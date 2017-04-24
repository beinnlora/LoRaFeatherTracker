//Code for Adafruit Feather M0 LoRa (RFM_RF95) + GPS featherWing shield
//transmits device position over LoRa radio every 30 seconds, with GPS-disciplined TDMA to accommodate 60+ devices on one radio channel with 30 second position updates

//GPL Stephen Wilson April 2016


//This tracker's device ID. Change for each individual tracker. This determins the transmission slot timing.
#define DEVICE_ID 1
//Transmit frequency - adjust as required. Code should work with 868MHz units too.
#define RF95_FREQ 434.0
//Flag for enabling some debug outputs that may affect slot timings / battery life.
#define debug 1

//LED pin for basic status report
#define myled 13
//input pin for receiving pulse-per-second output from GPS board
#define ppsPin A5
// output pin to time LoRa transmission code execution speed via oscilloscope. pin goes high at start of transmission function.
#define sendtriggerpin A4

//libraries - TinyGPS++, RadioHead. Some of the Time alarms are redundant 
#include <TinyGPS++.h>
#include "dtostrf.c"
#include <TimeLib.h>
#include <TimeAlarms.h>
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

/* for shield
  #define RFM95_CS 10
  #define RFM95_RST 9
  #define RFM95_INT 7
*/

// The TinyGPS++ object
TinyGPSPlus gps;

//feather batt monitor
#define VBATPIN A7

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

/////////////
// VARIABLES
////////////

String InputString = "";                      //used in LoRa transmissions
String Outputstring = "";                     //used in LoRa transmissions
float Tlat, Tlon;                             //used in formatting position report

int devid = DEVICE_ID;                        //THIS DEVICE ID

long setclockevery = 3600;                    // TODO set clock from GPS every x seconds
long setclocklast = 0;                        //system millis count when clock was last set

long getvoltsevery=5000;                      //get battery voltage every x hours
long getvoltslast=0;                          //system millis count when the battery was last checked

int16_t packetnum = 0;                        // packet counter, we increment per xmission

long ticktockevery = 5000;                    // 5 second watchdog timer fpr debug message
long ticktocklast = 0;                        //ticktock message timers for serial display to show working


long previousMillis = 0;                      //blink timer
byte validloc = 0;                            //flag for valid location fix
byte validtime = 0;                           //valid time fix

float vbat = 0;                               //current battery voltage
int ledstat = 1;                              //LED status

int interval = 500;                           //timing between LED flashes    
int flasduration = 100;                       //how long one flash is (ms)
int flashcounter = 1;                         //how many LED flashes counter
int tempinterval = interval;                  //helper variable for LED flashes

//for working out slot timings
long ourslot = 0;                             //which millisecond slot to transmit in
long oursec = 0;                              //how many seconds past the minute is our slot
long oursec2=0;                               //for testing every 30 second transmits

long nexttick = 0;                            //watchdog for PPS processing
byte onppsflag=0;                             //interrupt flag on pps

///////////////////////////////////////
// SETUP
///////////////////////////////////////
void setup()
{
  //define hardware pins
  pinMode(RFM95_RST, OUTPUT);     //LoRa reset pin
  pinMode(myled, OUTPUT);         //status LED
  pinMode(VBATPIN, INPUT);        //voltage divider for battery level
  pinMode(ppsPin,INPUT);          //pulse per second input from GPS
  pinMode(sendtriggerpin,OUTPUT); //hardware flag for start of LoRa transmission function. For timing loops with 'scope
  
  // assert LoRa modem
  digitalWrite(RFM95_RST, HIGH);
     
  //wait for serials - onboard Serial for debugging. Serial1 for GPS shield
  while (!Serial);
  while (!Serial1);//gps
  
  //start serials
  Serial.begin(9600);
  Serial1.begin(9600); //GPS
  delay(100);

  //CALCLULATE SLOT TIMINGS
  //transmit every 30 seconds
  oursec = (devid - 1) / 2;   //determine how many seconds past the minute our device should transmit at
  oursec2 = oursec+30 ;       //add additional 'seconds' slot for transmission, 30 seconds after our first 
  ourslot = ((devid - 1) % 2) * 500;  //how many milliseconds past the 'second' timeslot should we transmit for this device
  
  //for debug
  Serial.print("Devid: "); Serial.println(devid);
  Serial.print("slot sec:millis: "); Serial.print(oursec); Serial.print(":"); Serial.println(ourslot);
  Serial.println("Feather LoRa TX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  //initialise LoRa modem
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM. Check communications.
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  
  //Set LoRa parameters temporarily - just check the board is compatible first: TODO: remove this section
  //Serial.print ("Setting rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);");
  //Bw125Cr45Sf128
  Serial.print ("Setting rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);");
  rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096); //then overwrite with other values later on
  
  ///////////////
  // LoRa MODEM PARAMS
  ///////////////
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
  // SET CLOCK - loop until valid position is received.
  // do some flashing of the LED to show we have no fix yet
  //while clock not set do:
  ////////////

  while (validtime == 0)
  {
    //get time - LEd should do 3 BLINKs to show it is in SET CLOCK mode
    BlinkLed(myled, 2000, 3);
    //read from GPS when bytes available
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
  ///AWAIT VALID LOCATION - loop until obtained
  ///////////////////////
  while (validloc == 0)
  {
    //LED flash mode now two quick flashes = awaiting valid fix:
    BlinkLed(myled, 1500, 2);
    //get bytes from GPS 
    while (Serial1.available() > 0) {
      if (gps.encode(Serial1.read()))
      {
        Serial.println ("Getting pos");
        displayInfo();
        //might as well show the battery voltage, should this be causing the issue of no fix
        getvolts();
        Serial.print ("Vbat: "); Serial.println(vbat);
        
        //if we get a valid position, flag as such
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
  //some flashes to show ready TODO: tidy up you lazy s*d.

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

////////////////
// LOOP
////////////////
void loop()
{
//if we get a pps interrupt trigger, it sets the onpps flag, so service it
if( onppsflag==1){
  //Received a PPS signal from GPS - transmit position if our device's timeslot allows (set in ONPPS funtion)
  
  Serial.print ("ONPPS!");
  //
  //compare device timeslot to current time, and transmit if needed
  onpps();
  //reset flag
  onppsflag=0;
}
  /////////////////////
  //Not servicing a PPS interrupt - do other tasks
  /////////////////////
  // service the LED blinking requirements - single flash is standby mode - all hunky dory.
  if(onppsflag==0){
    BlinkLed(myled, 1000, 1);
  }
  
  //////////////////////
  //UPDATE GPS object with latest position if we have data on the GPS serial port.
  // abort if we receive a PPS interrupt, so that we may service it as fast as possible.
  /////////////////////
  while ((Serial1.available() > 0) &&onppsflag==0){
    if ((gps.encode(Serial1.read())) && onppsflag==0)
    {
      displayInfo();

    }
  }

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
//quick serial output of GPS params
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
/////
void getvolts() 
//get the battery voltage from ADC and scale accordingly. NB this will depend on your voltage divider network.
{
  vbat  = analogRead(VBATPIN);
  vbat *= 2;
  vbat *= 3.3;
  vbat /= 1024;
}

void addtostring(double lFloat, byte lmin, byte lprecision, String Stuff)
//construct a LoRa suitable output string.
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
  //blink our LED to show current status
  
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
//go and send a LoRa packet
{
  digitalWrite(sendtriggerpin,HIGH); //output pin high to time function execution speed via oscilloscope
  Serial.println("Sendpacket function"); //TODO: move into DEBUG only to speed up execution if required
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
  //add GPS info if valid
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
    
    addtostring(0, 0, 0, "*"); //append * to end for ease of reception. TODO possible checksum a-la /NMEA
  
  //////////////////////
  //TRANSMIT
  /////////////////////
  Serial.print ("Sending: "); Serial.println(Outputstring);
  //vbat  = analogRead(VBATPIN);
  //construct array for holding LoRa payload
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
  //what to do when we receive a pulse-per-second trigger from GPS
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

  //GET current GPS object SECOND timestamp (how many seconds past the minute)
  //add 1 to it to give us the real time now (we have received 1pps but the gps object is now 1s out of date)
  int seconds = gps.time.second();
  Serial.print ("current seconds: "); Serial.println (seconds);//TODO: shift all serial prints to debug only
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
  //interrupy handler for receiving pps signal: just set a flag
  
{
  onppsflag=1;
}





