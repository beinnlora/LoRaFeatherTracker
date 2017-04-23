#define DEVICE_ID 1
#define RF95_FREQ 434.0
#define debug 1

#define myled 13
#define ppsPin A5
#define sendtriggerpin A4
#include <TinyGPS++.h>

#include "dtostrf.c"

#include <TimeLib.h>
#include <TimeAlarms.h>

// Feather9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_RX
//01
//02
//03
//04 BASIC WORKING VERSION - validpos
//05 working on GPS time sync
//06 still working on gps time sync and cleaner structure
//07 - basic structure and sync timing done - need HW PPS link


#include <SPI.h>
#include <RH_RF95.h>

/* for feather32u4
  ##define RFM95_CS 8
  #define RFM95_RST 4
  #define RFM95_INT 7
*/
/* for feather m0  */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

// The TinyGPS++ object
TinyGPSPlus gps;

//feather batt monitor
#define VBATPIN A7
/* for shield
  #define RFM95_CS 10
  #define RFM95_RST 9
  #define RFM95_INT 7
*/



// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);


/////////////
// VARIABLES
////////////

String InputString = "";                     //used in LoRa transmissions
String Outputstring = "";
float Tlat, Tlon;

//THIS DEVICE ID
int devid = DEVICE_ID;
long setclockevery = 3600; //set clock from GPS every hours
long setclocklast = 0;

long getvoltsevery=5000;
long getvoltslast=0;
int16_t packetnum = 0;  // packet counter, we increment per xmission

long ticktocklast = 0;    //ticktock message timers for serial display to show working
long ticktockevery = 5000; // 5 second watchdog timer

long previousMillis = 0; //blink timer
byte validloc = 0;  //valid location fix
byte validtime = 0;    //valid time fix

float vbat = 0;
int ledstat = 1;

int interval = 500;
int flasduration = 100;
int flashcounter = 1;
int tempinterval = interval;

long ourslot = 0;
long oursec = 0;
long oursec2=0; //for testing every 30 second transmits
//for working out slot timings

//watchdog for PPS processing
long nexttick = 0;

//interrupt flag on pps
byte onppsflag=0;

void setup()
{
  pinMode(RFM95_RST, OUTPUT);
  pinMode(myled, OUTPUT);
  pinMode(VBATPIN, INPUT);
  pinMode(ppsPin,INPUT);
  pinMode(sendtriggerpin,OUTPUT);
  
  digitalWrite(RFM95_RST, HIGH);
  
  //attach interrupt for pps
  
  //wait for serials
  while (!Serial);

  while (!Serial1);//gps

  Serial.begin(9600);
  Serial1.begin(9600); //GPS
  delay(100);

  //CALCLULATE SLOT TIMINGS
  //transmit every 30 seconds
  oursec = (devid - 1) / 2;
  oursec2 = oursec+30 ;
  ourslot = ((devid - 1) % 2) * 500;
  

  Serial.print("Devid: "); Serial.println(devid);

  Serial.print("slot sec:millis: "); Serial.print(oursec); Serial.print(":"); Serial.println(ourslot);

  Serial.println("Feather LoRa TX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  //Serial.print ("Setting rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);");
  //Bw125Cr45Sf128
  Serial.print ("Setting rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);");
  rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096); //then overwritew
  ///////////////
  // MODEM PARAMS

  ////////

  //WE WANT
  //SF10
  //BW250
  //CR 4/8

  //cr 4/8  config1 = 20
  //bw 250 config1=20

  //sf9 512 config2 = 90
  // if sf12 then low data rate optimise = 01 (config3 = 01)

  //rf95.setModemConfig(RH_RF95::Bw125Cr48Sf512);
  //1d 1e 26


  const RH_RF95::ModemConfig cfg = {0x88, 0xA4, 0x00};
  rf95.setModemRegisters(&cfg);
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

  ////////////
  // SET CLOCK - pause until valid position
  // do some flashing of the LED to show we have no fix yet
  //while clock not set do
  ////////////

  while (validtime == 0)
  {
    //get time - 3 BLINK
    BlinkLed(myled, 2000, 3);
    while (Serial1.available() > 0) {
      if (gps.encode(Serial1.read()))
      {
        Serial.println ("Getting time");
        displayInfo();
        unsigned long age;
        int Year;
        byte Month, Day, Hour, Minute, Second;
        Year = gps.date.year();
        Month = gps.date.month();
        Day = gps.date.day();
        Hour = gps.time.hour();
        Minute = gps.time.minute();
        Second = gps.time.second();
        age = gps.time.age();

        if (age < 500 && gps.date.isValid() && gps.time.isValid()) {
          Serial.println ("Got time with age under 500");
          // set the Time to the latest GPS reading
          setTime(Hour, Minute, Second, Day, Month, Year);
          //  adjustTime(offset * SECS_PER_HOUR);
          validtime = 1;
        } else {
          Serial.println ("time not valid");
          validtime = 0;
        }
      }

    }
  }
  delay (2000);
  ///////////////////////
  ///AWAIT VALID LOCATOIN
  ///////////////////////
  while (validloc == 0)
  {
    BlinkLed(myled, 1500, 2);
    while (Serial1.available() > 0) {
      if (gps.encode(Serial1.read()))
      {
        Serial.println ("Getting pos");
        displayInfo();
        getvolts();
        Serial.print ("Vbat: "); Serial.println(vbat);
        if (gps.location.isValid())
        {
          validloc = 1;
          Serial.println ("Got Valid GPS Fix");
        } else {

          //DEBUG this is set to 1
          //validloc = 0;
          //if (debug){
         // validloc = 1;}
          Serial.println ("No GPS Fix");
        }


      }
    }
  }

  //OK we have everything

Serial.println ("Got time and fix. Proceeding");

  delay(2000);
  //some flashes to show ready

  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(100);
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(100);
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(100);
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(100);
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(100);

  delay(2000);
  //activate PPS interrupt
  attachInterrupt(ppsPin,onmyinterrupt,RISING);
} //end setup


void loop()
{
if( onppsflag==1){
  //ON PPS
  
  Serial.print ("ONPPS!");
  //
  onpps();
  onppsflag=0;
}
  ///blinking
  if(onppsflag==0){
    BlinkLed(myled, 1000, 1);
  }
  //1PPS FLASH WHEN RUNNING IN LOOP
  //////////////////////
  //UPDATE GPS
  /////////////////////
  while ((Serial1.available() > 0) &&onppsflag==0){
    if ((gps.encode(Serial1.read())) && onppsflag==0)
    {
      displayInfo();

    }
  }


  //////////////////////
  //ACTIVITY CYCLE - 

  //GET BATTERY LEVEL EVERY 5 seconds
nexttick=getvoltslast+getvoltsevery;
 if ((millis() > nexttick) && onppsflag==0)
 {
 getvolts();
 getvoltslast=millis();
 }
 
 
 
//////////////////////
    //WATCHDOG if no PPS received
    //if we haven't had a PPS for over 10 seconds, send a packet alarm
    /////////////////////

  nexttick = ticktocklast + ticktockevery;
  if ((millis() > nexttick) && onppsflag==0)
  {
    
    ticktocklast = millis();
    rf95.setHeaderFlags(1);
    Serial.println ("Timeout waiting for PPS");
    sendpacket();
    rf95.setHeaderFlags(0);


  }
} //end void loop
///////////////
/// FUNCTIONS
///////////////
void displayInfo()
{
  Serial.print(F("Location: "));
  if (gps.location.isValid())
  {
    // validloc = 1;
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
    //  validloc = 0;
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
    //  validtime = 1;
  }
  else
  {
    Serial.print(F("INVALID"));
    //  validtime = 0;

  }
Serial.print (" ");    Serial.print(F("SATELLITES Fix Age="));
    Serial.print(gps.satellites.age());
    Serial.print(F("ms Value="));
    Serial.println(gps.satellites.value());
 // Serial.println();
}

void getvolts() {
  vbat  = analogRead(VBATPIN);
  vbat *= 2;
  vbat *= 3.3;
  vbat /= 1024;
}
void addtostring(double lFloat, byte lmin, byte lprecision, String Stuff)
{
           //clear array
  InputString = "";
  
  if (lmin==0 && lprecision==0 && lFloat==0)
  {
      Outputstring = Outputstring  + Stuff;
  } else {
    char charVal[10];                              //temporarily holds data from vals
  memset(charVal, 0, sizeof(charVal));  
  dtostrf(lFloat, lmin, lprecision, charVal);    //lmin is mininum width, lprecision is precision
  for (int i = 0; i < sizeof(charVal); i++)
  {
    if  (charVal[i] == 0)
    {
      break;
    }
    InputString += charVal[i];
  }
  Outputstring = Outputstring  +InputString+ Stuff;
  }
}

void BlinkLed (int led, int interval, int flashes) {
  //Serial.println("inblink");
  //(long) can be omitted if you dont plan to blink led for very long time I think
  if (millis() > (previousMillis + tempinterval)) {

    previousMillis = millis(); //stores the millis value in the selected array
    // Serial.println (interval);

    if (ledstat == 0) {
      //light is off, turn it on
      ledstat = 1;
      digitalWrite(led, ledstat); //changes led state
      ledstat = 1;
      //do our x quick flashes
      tempinterval = flasduration;
      //flascounter


    } else {
      //light is on, turn it off
      ledstat = 0;
      digitalWrite(led, ledstat); //changes led state
      //we've finished a flash
      flashcounter--;
      //if we are doing short flashes, temporarily override interval
      if (flashcounter != 0) {
        tempinterval = flasduration;
      } else


      {
        //have done our requisite flashes, delay set to supplie
        tempinterval = interval;
        flashcounter = flashes;
  //for debugging, when flashes = 1 then dummy this as pps
  //if (flashes == 1) {
  //  onpps();
 // }

      }
    }

  }


}

void sendpacket()
{
  digitalWrite(sendtriggerpin,HIGH);
  Serial.println("Sendpacket function");
  /////////////////////

  // GET VOLTAGE - already got in void loop every x seconds
  //////////////////////
  
 // getvolts();
  if(debug){Serial.print ("Vbat: "); Serial.println(vbat);}
  //////////////////////
  //HEADER
  /////////////////////
  //valid=1;
  rf95.setHeaderTo(255);    //TO = 255 (broadcast defined by us
  rf95.setHeaderFrom(devid); //FROM= device ID
  rf95.setHeaderId(validloc);  // header ID  0 = no fix  1 = fix  2 = panic, no fix, panic, 3 = fix, panic
  //rf95.setHeaderFlags(0);


  //////////////////////
  //PAYLOAD
  /////////////////////
  Outputstring = "";
  //add vBat
    addtostring(vbat, 3, 2, ",");
  //add /GPS info if valid
  if (gps.location.isValid()) {         // if location is not validloc fix, then only send voltage
  //lat
    addtostring(gps.location.lat(), 5, 5, ",");
    //lon
    addtostring(gps.location.lng(), 5, 5, ",");
    //heading
    addtostring(gps.course.deg(),3,0,",");
    //speed
    addtostring(gps.speed.kmph(),2,1,",");
    //altitude
    addtostring(gps.altitude.meters(), 3, 0, ",");
    //fix age
    //addtostring(gps.location.age(),3,0,",");
    //sats
    addtostring(gps.satellites.value(),2,0,",");
    
    
    
    
    
  } 
    
    addtostring(0, 0, 0, "*");
  
  //////////////////////
  //SEND
  /////////////////////
  Serial.print ("Sending: "); Serial.println(Outputstring);
  //vbat  = analogRead(VBATPIN);
  uint8_t data[] = "v.vv,ddd.ddddd,ddd.ddddd,ccc.c,ss.s,aaa.a,aaa*";
  //uint8_t data[]="";
  for (int i = 0; i < Outputstring.length(); i++) {
    data[i] = Outputstring[i];
    Serial.print(data[i]);
  }
  //Outputstring.toCharArray(data, 20);
  Serial.print (Outputstring);

  //Serial.print("Sending "); Serial.println(data);
  //delay(100);
  rf95.send(data, Outputstring.length());

  // for (int i=0;i<21;i++){
  //Serial.print(rp2[i]);
  //  }
  Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
//END OF SEND PACKET
 digitalWrite(sendtriggerpin,LOW);
}



void onpps()
{
  Serial.println("onpps");
  //interrupt driven
  //we assume we have valid time currently stored in the GPS object
  //this received PPS is therefore one second AFTER the time given in PPS
  //device 1 transmits on the minute
  //device 2 transmits 500ms after the minute
  //device 3 etc...
  //  long ourslot = (devid-1%2)*500;
  // long oursec = devid/2

  //reset our pps watchdog
  ticktocklast = millis();

  //GET current GPS object SECOND timer
  //add 1 to it to give us the real time now (we have received 1pps but the gps object is 1s slow)
  int seconds = gps.time.second();
  Serial.print ("current seconds: "); Serial.println (seconds);
  seconds++;
  int nextsec = seconds % 60;
  Serial.print("next seconds: "); Serial.println(nextsec);
  Serial.print("Trigger on  ");Serial.print(oursec);Serial.print("and: ");Serial.println(oursec2);
  if (oursec == nextsec  || oursec2 ==nextsec) {
    //do something this second
    //if we are subsecond slot, then wait:
    Serial.print("sleeping ");Serial.println(ourslot);
    delay(ourslot);
   
    Serial.println ("DOING IT");
  sendpacket();
    //wait until the second has passed before resuming - could even sleep 30 seconds?
    while (((gps.time.second()+1)%60) == nextsec) {
      while (Serial1.available() > 0) {

        gps.encode(Serial1.read());
        displayInfo();

      }
    }
  }
//when all done, send our trigger pin low again for debug

}

void onmyinterrupt()
{
  onppsflag=1;
}





