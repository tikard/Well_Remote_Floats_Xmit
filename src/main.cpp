/*      WELL REMOTE FLOAT TRANSMITTER MODULE
  Uses LORA to send float msgs from a remote float sender unit
  

  REV 0.0  Initial Version   5/4/21

  5/4/22  Working version:   Tested with Receiver side all works

  5/5/22  Pushed code up to GITHUB


*/

#include <Arduino.h>
#include "heltec.h"
#include "images.h"
#include "HardwareSerial.h"
#include "ArduinoJson-v6.18.5.h"
#include <U8x8lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

const int WEB_RADIO_ID = 101;

// OLED screen setup
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

// HELTEC LORA 
#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6

#define debugMode true

String rssi = "RSSI --";
String packSize = "--";
String packet;

unsigned int counter = 0;

bool receiveflag = false;   // software flag for LoRa receiver, received data makes it true.

uint64_t chipid;

//************************************ Well Stuff **************************************************  

const int EmptyPin = 32;  // empty from FLOAT   low = true
const int FullPin  = 33;  // full from FLOAT    low = true

int radioID   = 0;
int wellFULL  = 0;
int wellEMPTY = 0;

const long ONESECONDINTERVAL = 1000;   // one second counter
const long TWOSECONDINTERVAL = 2000;   // two second counter
const long DEFAULTINTERVAL   = 15000;   // Default Interval

long previousMillis = 0;        // will store last time LED was updated

StaticJsonDocument<512> doc;

// Helpers to send output to right port
void debugPrint(String s){ Serial.print(s);}
void debugPrint(int s){ Serial.print(s);}
void debugPrintln(String s){ Serial.println(s);}
void debugPrintln(int s){ Serial.println(s);}

// OLED Functions
void pre(void)
{
  u8x8.setFont(u8x8_font_chroma48medium8_r); 
  u8x8.clear();
  u8x8.setCursor(0,0);
}

void draw_ascii_row(uint8_t r, int start)
{
  int a;
  uint8_t c;
  for( c = 0; c < u8x8.getCols(); c++ )
  {
    u8x8.setCursor(c,r);
    a = start + c;
    if ( a <= 255 )
      u8x8.write(a);
  }
}

void idleScreen(){
  pre();
  //u8x8.setFont(u8x8_font_px437wyse700b_2x2_r);
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.println("ID = " + String(radioID));

  u8x8.println("");
  if(wellEMPTY == 1)
    u8x8.println("EMPTY = TRUE");
  else
    u8x8.println("EMPTY = FALSE");

  if(wellFULL == 1)
    u8x8.println("FULL  = TRUE");
  else
    u8x8.println("FULL  = FALSE");

  u8x8.println("");


  u8x8.println("");
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.println("Last " + rssi);
}

void receiveLORAData(String payload){
  //Serial.println("payload from relay: " + payload);

  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.println(error.c_str()); 
    return;
  }

  // store data in globals
  //radioID   = doc["RID"];
  //wellFULL  = doc["WF"];
  //wellEMPTY = doc["WE"];

  //Serial.println("Radio ID is " + radioID);
}


// *************************************************** LORA Functions

void interrupt_GPIO0()
{
  delay(10);
  // user button pressed
}

void onLORAReceive(int packetSize)  //LoRa receiver interrupt service
{
  packet = "";
  packSize = String(packetSize,DEC);

  while (LoRa.available()){
    packet += (char) LoRa.read();
  }

  rssi = "RSSI: " + String(LoRa.packetRssi(), DEC);    
  receiveflag = true;    
}


// *****************************************   1 time SETUP CODE  ************************************
void setup() {
  Serial.begin(115200);
  u8x8.begin();

  pinMode(EmptyPin,INPUT_PULLUP);  // pull down to zero 
  pinMode(FullPin,INPUT_PULLUP);  // pull down to zero 
  

  debugPrintln("Well Remote Float Transmitter - Testing");

  // Setup the LORA board for communications
  Heltec.begin(false /*DisplayEnable Enable*/, true /*LoRa Enable*/, true /*Serial Enable*/, true /*LoRa use PABOOST*/, BAND /*LoRa RF working band*/);
  
  LoRa.setTxPowerMax(20);
  
  
  idleScreen();
	delay(300);

  attachInterrupt(0,interrupt_GPIO0,FALLING);
  LoRa.onReceive(onLORAReceive);
  LoRa.receive();

}

void send(String msg, String debugMsg)
{
    //LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
    //LoRa.setTxPower(5,RF_PACONFIG_PASELECT_PABOOST); 
    LoRa.beginPacket();
    LoRa.setTxPower(5,RF_PACONFIG_PASELECT_PABOOST); 
    LoRa.print(msg); 
    LoRa.endPacket();
    delay(50);
    LoRa.receive();
    debugPrintln(debugMsg);
}

void pushFloatData(){  // Send out whole msg to Display units which are listening
  String requestBody = "";
  doc.clear();
  doc["RID"]  = radioID;    // Add values in the document
  doc["WF"]   = wellFULL;   // Add values in the document
  doc["WE"]   = wellEMPTY;  // Add values in the document
  serializeJson(doc, requestBody);

  debugPrintln("Sending Float Data = " + requestBody);

  send(requestBody, "Sending Float Status via LORA");
}

void loop() {
  unsigned long currentMillis = millis();

  if(receiveflag){  // Process any data
    receiveLORAData(packet);  // got data over LORA from someone
    receiveflag = false;
    LoRa.receive();
    delay(50);
  }else{
  
  }

  wellEMPTY  = digitalRead(EmptyPin);
  wellFULL  = !digitalRead(FullPin);   

  debugPrintln("Well Empty = " + String(wellEMPTY) + "  Full  = " + String(wellFULL));
  //debugPrintln("Well Full  = " + String(wellFULL));
  

  if(long(currentMillis - previousMillis) > DEFAULTINTERVAL) {
    previousMillis = currentMillis;  
    
    pushFloatData();

    u8x8.begin();  // fixes a bug in display lib
    idleScreen();

    //debugPrintln("Well Remote Float Receiver - Testing");

    LoRa.receive();
  }

  delay(TWOSECONDINTERVAL);

}
