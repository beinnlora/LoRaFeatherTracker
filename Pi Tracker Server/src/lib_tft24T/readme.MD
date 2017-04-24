using Adafruit TFT shield, this code queries the traccar server for tracked device names and positions, and displays their distance from 'home' and time since last heard.

'Home' is set by clicking on one tracked device on the web portal, then clicking 'set as home'. This stores that device's current location as the tracker server's 'home' location for calculating distances.

the 'Home' button addon is added to the Traccar library, and the actual URL behind the Home button is served via Flask
