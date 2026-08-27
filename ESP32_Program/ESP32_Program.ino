#include <WiFi.h>
#include <WiFiUDP.h>
#include <Adafruit_NeoPixel.h>
const char* ssid = "DigitalMediaLab_IoT"; //SSID Private (FRITZ!Box 7590 QM)
const char* password = "MZHe6.wlan!"; //PW PC Privat (80627705988204176528)
WiFiUDP udp;
IPAddress pc_IP(192, 168, 68, 70); //PC Adress here. Private PC IP(192, 168, 188, 36)
unsigned int localPort = 4210;//Port ESP32.
unsigned int unityPort = 5005;//Unity port.

#define LED_PIN 17
#define LED_COUNT 32
char incomingPacket[LED_COUNT * 3]; //LED_COUNT * 3 (RGB)
Adafruit_NeoPixel led_matrix(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

int delay_value = 500;

void setup() {
  Serial.begin(115200);
  //Function to scan available networks
  scanNetworks(); //Scan available networks.
  delay(100);//Small delay of 100ms.

  WiFi.begin(ssid, password); //Start WIFi connection.
  while (WiFi.status() != WL_CONNECTED) //While not connected to a WiFi print "." every 500ms.
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi CONNECTED!"); //If connected print "WiFi CONNECTED!"
  Serial.print("IP: "); 
  Serial.println(WiFi.localIP());

//UDP starten.
  udp.begin(localPort); //Begin UDP Communication listen on Port "localPort".
  delay(200);
  analogSetAttenuation(ADC_11db); //Set ADC (Analog Digital Converter) attenuation to 11db | Use Pin 34 for ADC1 because ADC2 causes issues while operating with WiFi.
 
 //NeoPixel setup.
  led_matrix.begin();
  led_matrix.setBrightness(10); //Dimn LED brightness to 10.
  led_matrix.clear(); //Clear all previous saved LED colours.
  led_matrix.show(); //show new colours (LED not on).
/*
  for (int i = 0; i < LED_COUNT; i++) //For loop for every LED in Matrix.
  { 
    led_matrix.setPixelColor(i, led_matrix.Color(255, 0, 0)); //Set all LEDs to red.
  }
led_matrix.show();
*/
}

void loop() {
//sum of 10 samples from the ADC to reduce noise of wron meassurements
int sum = 0;
for (int i = 0; i < 10 ; i++) //For loop 10 steps.
{
  int analogValue = analogRead(34); //Analog read pin 34 of the esp32.
  int anlogValueConstraint = constrain(analogValue, 180, 3000); // Constrain the measured Analog Voltage to min 180 (roughly 150mV) and max 3000 (roughly 2450 mV) due to linearity issues of the Aanalog Digital Converter of the ESP32 WROOM 32.
  sum += anlogValueConstraint; //Add anlogValueConstraint to the sum.
}
int adc_Average = sum / 10; //Average of the 10 Samples.
float voltage = adc_Average * (3.3 / 4095); //Determin the meassured volt using the adc_Average and current of the esp32 (3.3V).
long panel_Angle = map(adc_Average, 180, 3000, 0, 180); //Map the Value between 180 and 3000 due to non linearity of the esp32 in those voltage ranges.

udp.beginPacket(pc_IP, unityPort); //Send the determined angle to the Unity Client.
udp.println(panel_Angle);
udp.endPacket();
delay(20);

int packetSize = udp.parsePacket(); //Get LED-Colour matrix from Unity-Client
if(packetSize) //if packetSize > 0 the read package.
{
  int len = udp.read(incomingPacket, LED_COUNT * 3);
  if(len > 0) incomingPacket[len] = 0;
  Serial.print("Received: ");
  Serial.print(incomingPacket);
 
  for(int i = 0; i < LED_COUNT; i++) //For Length of LED_matrix. Set RGB Values of the LEDs.S
  {
    // LED 0 -> Index 0,1,2 | LED 1 -> Index 3,4,5 ....
    int r = incomingPacket[i*3];
    int g = incomingPacket[i * 3 + 1];
    int b = incomingPacket[i * 3 + 2];
    /*
    Serial.println("Color for LED: " + i);
    Serial.print("r: ");
    Serial.print(r);
    Serial.print("g: ");
    Serial.print(g);
    Serial.print("b: ");
    Serial.print(b);
    */
    //Set the LED colors of LED i
    led_matrix.setPixelColor(i, led_matrix.Color(r, g, b));
  }
  
  led_matrix.show(); //Display new LED-Matrix with parsed colours.
}
//else Serial.println("No UDP Packet received.");
//float panel_Angle = (adc_Average - 180) * 180 / (3000 - 180)
/*
Serial.print("Analog Value average is: ");
Serial.println(adc_Average);
Serial.print("Voltage is: ");
Serial.println( voltage);
Serial.print("Angle is: ");
Serial.println(panel_Angle);
*/
delay(20);
}
//Function to scan available networks
void scanNetworks()
{
int n = WiFi.scanNetworks();
  Serial.println("Gefundene WLANs:");

  for (int i = 0; i < n; i++) {
    Serial.println(WiFi.SSID(i));
  }
}
