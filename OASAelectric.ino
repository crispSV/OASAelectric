/*
Project OASAelectric EV IOT LoRaWAN tranceiver

Modules used:
- Arduino UNO/Nano (ATMEGA328P)
- GPS NMEA serial Module (UBLOX 8M)
- LoRa EBYTE E220-900T22D (868 MHz)
- OLED and NFC reader to be implemented later

Copyright Bluedrive team DataHackathon2022 Christos Papathanasiou - Maria Oikonomopoulou
*/

// ========== GENERAL SETTINGS ========== 
#define NAME_LENGTH 12                   // the same value for all EV 
char MY_NAME[NAME_LENGTH] = "OASA_EV_1234";  //  Unique name of electric vehicle
#define CARD_ID_LENGTH 12				// ATH.ENA card ID length
int MY_ID[CARD_ID_LENGTH]				// Read from NFC customer ATH.ENA ID (to be implemented)
int ALARM=0								// initiate ALARM flag

// ========== SERIAL DEBUGGING ========== 
#define DEBUG_MODE false



// ============ GPS SETTINGS ============ 
#define GPS_PIN_RX 8
#define GPS_PIN_TX 7
#define GPS_PACKET_INTERVAL 20000 // milliseconds


// =========== LORA SETTINGS =========== 
#define LORA_PIN_RX 2
#define LORA_PIN_TX 3
#define LORA_PIN_M0 4
#define LORA_PIN_M1 5
#define LORA_PIN_AX 6
int loraChannel = 5;  // default LORA channel
// ======= END OF SETTINGS =============


#include <SoftwareSerial.h>  // standart library
#include <TinyGPSPlus.h>         // install from Arduino IDE
#include "EBYTE.h"           // Lora module library

// ========== GPS section ==========
TinyGPSPlus gps;
SoftwareSerial gps_ss(GPS_PIN_RX, GPS_PIN_TX);

// ========== LORA section ==========
struct DATA {
  float lat;
  float lon;
  unsigned char sat;

  unsigned char year; // the Year minus 1900
  unsigned char month;
  unsigned char day;

  unsigned char hour;
  unsigned char minute;
  unsigned char second;

  char id[NAME_LENGTH];
  char myid[MY_ID];
};

DATA loraDataPacket;
SoftwareSerial lora_ss(LORA_PIN_RX, LORA_PIN_TX);
EBYTE loraTransceiver(&lora_ss, LORA_PIN_M0, LORA_PIN_M1, LORA_PIN_AX);

unsigned long lastLoraPacketTime;


// ========== SETUP ==========  
void setup()
{
  Serial.begin(9600);



  // GPS initialisation
  gps_ss.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT); // GPS valid indicator
  if (DEBUG_MODE) displayGPSInfo();

  // LORA initialisation
  lora_ss.begin(9600);
  loraTransceiver.init();
  loraTransceiver.SetAddressH(0xFF);
  loraTransceiver.SetAddressL(0xFF);
  loraTransceiver.SetChannel(loraChannel);
  loraTransceiver.SaveParameters(PERMANENT);  // save the parameters to the unit

  if (DEBUG_MODE) {
    Serial.println("OASAelectric started...");
    Serial.println("Current LORA parameters:");
    loraTransceiver.PrintParameters();          // print all parameters for debugging
  }
}

// ========== MAIN LOOP ==========  
void loop()
{
  ListenLORA();  // listening and send to serial
  sendGPStoLORA(); 
}


// =========================================== Listen LORA =====================================
void ListenLORA() {
  lora_ss.listen();     //switch to lora software serial
  for (unsigned long start = millis(); millis() - start < GPS_PACKET_INTERVAL;) {
    if (lora_ss.available()) {
      loraTransceiver.GetStruct(&loraDataPacket, sizeof(loraDataPacket));
      // if you got data, update the checker
      lastLoraPacketTime  = millis();

      // DEBUG_MODE
      if (DEBUG_MODE){      // dump out what was just received
        Serial.print("NEW LORA DATA RECIVED. ID: "); Serial.print(loraDataPacket.id);
        Serial.print(" LAT: "); Serial.print(loraDataPacket.lat, 6);
        Serial.print(" LON: "); Serial.print(loraDataPacket.lon, 6);
        Serial.print(" SAT: "); Serial.print(loraDataPacket.sat);

        Serial.print(" Date: "); Serial.print(loraDataPacket.year);
        Serial.print("/"); Serial.print(loraDataPacket.month);
        Serial.print("/"); Serial.print(loraDataPacket.day);

        Serial.print(" Time: "); Serial.print(loraDataPacket.hour);
        Serial.print(":"); Serial.print(loraDataPacket.minute);
        Serial.print(":"); Serial.println(loraDataPacket.second);

        
      }

      getNewData(loraDataPacket);
      

     
    }
    else {
      // if the time checker is over some prescribed amount
      // let the user know there is no incoming data
      
        lastLoraPacketTime = millis();
      }
    }
  }
}


// =======------------------=== READ GPS and SEND TO LORA ==========
void sendGPStoLORA() {
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;
  // For one second we parse GPS data and report some key values
  gps_ss.listen();     //switch to gps software serial
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (gps_ss.available() > 0)
    if (gps.encode(gps_ss.read())){
      newData = true;
    }
  }

  digitalWrite(LED_BUILTIN, LOW);
  
  if (newData && gps.location.isValid() && gps.date.isValid() && gps.time.isValid())
  {
    digitalWrite(LED_BUILTIN, HIGH); // GPS is valid
    sendData( gps.location.lat(), gps.location.lng(), gps.satellites.value(), 
              gps.date.year(), gps.date.month(), gps.date.day(), 
              gps.time.hour(), gps.time.minute(), gps.time.second());
    if (DEBUG_MODE) displayGPSInfo();
  } else {
    //sendData(0, 0, 0);
  }
  
  if (gps.charsProcessed() < 10)
    if (DEBUG_MODE) Serial.println("** GPS ERROR **");
}


// ========== SEND LORA DATA ==========  
void sendData(float lat, float lon, unsigned short sat, 
              unsigned char year, unsigned char month, unsigned char day, 
              unsigned char hour, unsigned char minute, unsigned char second){
  
  // data set
  loraDataPacket.lat = lat;
  loraDataPacket.lon = lon;
  loraDataPacket.sat = sat;

  loraDataPacket.year = year;
  loraDataPacket.month = month;
  loraDataPacket.day = day;

  loraDataPacket.hour = hour;
  loraDataPacket.minute = minute;
  loraDataPacket.second = second;

  strcpy(loraDataPacket.id, MY_NAME);
  strcpy(loraDataPacket.id, MY_ID);


  // send
  lora_ss.listen();
  loraTransceiver.SendStruct(&loraDataPacket, sizeof(loraDataPacket));
  // debug
  if (DEBUG_MODE) Serial.println(F("SEND LORA DATE"));
}


// =================================== GPS ===========================================
void displayGPSInfo() {
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.print("MY GPS POSITION HAVE BEEN UPDATED: ");
  Serial.print("Satellites in view: ");
  Serial.println(gps.satellites.value()); 
  
  Serial.print(F("Location: ")); 
  
  if (gps.location.isValid())
  {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print(F("INVALID "));
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
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




// =============================== DATA STORAGE ========================================
#define STORAGE_SIZE 5
DATA storage[STORAGE_SIZE];
char storageCounter = 0;

// ADD NEW DATA TO STORAGE
void getNewData(DATA newData) {
  bool isExist = false;
  String newId = newData.id;

  for(int i = 1; i <= storageCounter; i++){
    String id = storage[i].id;

    if( id == newId ) {
      storage[i] = newData;
      isExist = true;
      
    }
  }

  if( !isExist ) {
    storageCounter++;
    if( storageCounter > STORAGE_SIZE ) storageCounter = STORAGE_SIZE;
    storage[storageCounter] = newData;
  }
}
