# Raspberry Pi 3 based tracker server that requires no internet/WAN connection.

Traccar (https://traccar.org) server running on Raspberry Pi
Serves maps

Raspberry Pi set up as wifi hotspot and captive portal redirects connected clients to self-hosted OpenStreetMap tracking portal 
using https://traccar.org and https://github.com/vitalidze/traccar-web interface

Map tiles are served from local cache using Tilestache server, so no internet connection needed.

Positions are received from clients via LoRa radio (HopeRF RF_RM95) connected over SPI to the Pi, and Python script uploads position reports and telemetry data (battery voltage) to the Traccer srever
