incomplete:

# Pre-requisites

Raspbian Jessie+

Python

https://github.com/mayeranalytics/pySX127x with adaptations to pinouts to work with Uputronics LoRa Shield for Hope RFM95

https://github.com/BLavery/lib_tft24T

Tilestache http://github.com/migurski/TileStache

Traccar.org https://traccar.org/

Traccar-web https://github.com/vitalidze/traccar-web

Flask: sudo apt-get install python3-flask


install individual components and confirm working, then copy the custom code on this github into the sub folders.

you can then test each individual component using the scripts at the root of this directory - for tilestache, flask, the LoRa receiver and TFT code etc, and of course Traccar and Traccar-web interface.
Once all working individually you can start them using the rc.local script to trigger each individual component.

You'll also need to set up the Pi as a wifi hotspot, DNS/DHCP server and captive portal.

The biggest job is editing Traccar and Traccar-web to serve maps 

I believe the latest version of Traccar post 2016 allows you to add custom maps, but the web interface on mobile devices is lacking, so I just butchered the Traccar-web interface and used this instead; letting Traccar server maintain the position database and accept telemetry.

Install Traccar, confirm working, then install Traccar-web, confirm working, then using an archive manager such as FileRoller on linux, you can directly edit traccar-web.war, specifically

traccar-web.war/m/index.html and any other references.

You'll also have to download and CSS resources and change references in the code for these 

edit /js/openlayers to replace references to openstreetmap with your own tilestache server

edit js/openlayers/openstreetmap to replace with url:port of your tilestache server


