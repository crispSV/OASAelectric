/*
Project OASAelectric EV IOT LoRaWAN transmitter
 (code for receiver to be implemented)

Modules used:
- Arduino UNO/Nano (ATMEGA328P)
- GPS NMEA serial Module (UBLOX 6M)
- LoRa SX1278 module
- SSD1306 128×64 Oled i2c display Module

Copyright Bluedrive team DataHackathon2022
*/

// Transmitter
// needed Libraries
#include <SoftwareSerial.h>
#include <AltSoftSerial.h>
#include <TinyGPS++.h>
#include <SPI.h>              
#include <LoRa.h>
//--------------------------------------------------------------

#include <Adafruit_GFX.h>			// Arduino display library 
#include <Adafruit_SSD1306.h>		// OLED module library 
//-------------------------------
static const int RXPin = 8, TXPin = 9;
static const uint32_t GPSBaud = 9600;
 
// The TinyGPS++ object
TinyGPSPlus gps;
 
// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin); // for gps
/////////////////////////////////////////////////////

String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
byte destination = 0xFF;     
byte localAddress = 0xBB;
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends in ms 
///----------------------

//  Oled display init
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
///-------------------------------
String Mymessage = "";
//--------------------------------------------------------------

// Size of the geo fence (in meters)
const float maxDistance = 3000;			// Geofencing so user notified if is out of track limits

//--------------------------------------------------------------
float initialLatitude = 37.973653;		// GPS init somewhere in Syntagma SQ Athens Greece
float initialLongitude = 23.735321;

float latitude, longitude;

char buff[10];
String mylong = ""; // for storing the longittude value
String mylati = ""; // for storing the latitude value

//--------------------------------------------------------------

int msgstatus; 

int Sensor1;
int relay = 3;
int alarm = 0; // alarm on malfunction 0=functioning
int bat_volt = 13; //battery voltage 
float distance;

/*****************************************************************************************
 * setup() function
 *****************************************************************************************/
void setup()
{
  //--------------------------------------------------------------
  //Serial.println("Arduino serial initialize");
  Serial.begin(9600);
   display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  pinMode(relay, OUTPUT);
  //--------------------------------------------------------------
  //Serial.println("NEO6M serial initialize");
    ss.begin(GPSBaud);
  //--------------------------------------------------------------
  if (!LoRa.begin(433E6)) {       
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }
 
 Serial.println("LoRa init succeeded.");
}

/*****************************************************************************************
 * loop() function
 *****************************************************************************************/
void loop()
{

  while (ss.available() > 0)
    if  ( gps.encode(ss.read() ) )
    {
     displayInfo();    
  latitude = gps.location.lat(), 6 ;
  longitude = gps.location.lng(), 6 ;
  mylati = dtostrf(latitude, 3, 6, buff);
  mylong = dtostrf(longitude, 3, 6, buff);
  distance = getDistance(latitude, longitude, initialLatitude, initialLongitude);

    }
    
  //--------------------------------------------------------------
  if (millis() - lastSendTime > interval) {
  //displayInfo(); 

 // Serial.print("Latitude= "); Serial.println(latitude, 6);
  //Serial.print("Lngitude= "); Serial.println(longitude, 6);

    if(distance > maxDistance) {
      msgstatus =1;
    }
    
    if(distance < maxDistance) {
      msgstatus =0;
       
    }
      
    // Serial.print("Distance: ");
  //Serial.println(distance);
   Mymessage = Mymessage + mylati +"," + mylong+","+msgstatus + "," +distance +"," + bat_volt +"," + alarm;  //Message to be send 
     sendMessage(Mymessage);
     //Serial.println(Mymessage);
    delay(50);
    Mymessage = "";
  

  
      //Serial.println("Sending " + message);
    lastSendTime = millis();            // timestamp the message
   
  }
  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());


}

// Calculate distance between two points
float getDistance(float flat1, float flon1, float flat2, float flon2) {

  // Variables
  float dist_calc=0;
  float dist_calc2=0;
  float diflat=0;
  float diflon=0;

  // Calculations
  diflat  = radians(flat2-flat1);
  flat1 = radians(flat1);
  flat2 = radians(flat2);
  diflon = radians((flon2)-(flon1));

  dist_calc = (sin(diflat/2.0)*sin(diflat/2.0));
  dist_calc2 = cos(flat1);
  dist_calc2*=cos(flat2);
  dist_calc2*=sin(diflon/2.0);
  dist_calc2*=sin(diflon/2.0);
  dist_calc +=dist_calc2;

  dist_calc=(2*atan2(sqrt(dist_calc),sqrt(1.0-dist_calc)));
  
  dist_calc*=6371000.0; //Converting to meters

  return dist_calc;
}
void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}


// Receiving function 

void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {   // check length for error
    //Serial.println("error: message length does not match length");
    ;
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
   // Serial.println("This message is not for me.");
    ;
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  //Serial.println("Received from: 0x" + String(sender, HEX));
  //Serial.println("Sent to: 0x" + String(recipient, HEX));
 //Serial.println("Message ID: " + String(incomingMsgId));
 // Serial.println("Message length: " + String(incomingLength));
 // Serial.println("Message: " + incoming);
 //  Serial.println("RSSI: " + String(LoRa.packetRssi()));
 //  Serial.println("Snr: " + String(LoRa.packetSnr()));
 //  Serial.println();

 String q = getValue(incoming, ',', 0); // Latitude
 Serial.println(q);
 Sensor1 = q.toInt();  // latitude

 if ( Sensor1 == 1 )
 {
  digitalWrite(relay, HIGH);
 }
  if ( Sensor1 == 0 )
 {
  digitalWrite(relay, LOW);
 }
}

void displayInfo()
{
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
    Serial.print(" ");
    Serial.print(F("Speed:"));
    Serial.print(gps.speed.kmph());
  }
  else
  {
    Serial.print(F("INVALID"));
  }
 
  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }
 
  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }
 
  Serial.println();
 
}


String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;
 
    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
