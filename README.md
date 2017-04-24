# LoRaFeatherTracker
Offline GPS tracker and map server based on Adafruit Feather M0 LoRa radios and Raspberry Pi. Location map is served to clients over wifi from cached map server, so requires no internet connectivity to work.


The code here is currently the Arduino code for the Feather M0, published solely as a favour for someone requesting a few hints. 

## Warning:
It is exceedingly rough, ready and poorly commented/written in a hurry. This is not how Arduino code should be written or presented!

Details on (https://hackaday.io/project/13013-off-grid-gps-race-tracker-client-and-server)

## Hardware:
**Mobile units/transmitters:** Adafruit Feather M0 LoRa + Adafruit Feather GPS Shield + 2x18650 LiPo

**Receiver/Map server:** Raspberry Pi 3 + HopeRF RFM95 Uputronics LoRa shield + 128x96 OLED display + USBPico UPS + Wifi USB dongle

## Software:
**Mobile units/transmitters:** 
  RadioHead RFM95 library for LoRa, TinyGPS++ library for GPS position and TDMA
  
  (http://www.airspayce.com/mikem/arduino/RadioHead/)
  (http://arduiniana.org/libraries/tinygpsplus/)

**Receiver/Map server:**
  Raspbian Jessie + python-based LoRa receiver code + Traccar server + Tilestache map tile server + Maperitive-produced OpenStreetmap .MB map tiles + traccar-web client portal +  flask for grabbing some client portal features

 (https://github.com/mayeranalytics/pySX127x)
  (https://www.traccar.org/)
  (https://github.com/vitalidze/traccar-web/)
  (http://tilestache.org/)
  (http://maperitive.net/)
  

It uses the RadioHead library for LoRa, communicating over SPI, and the TinyGPS++ library for GPS, communicating via Serial1





