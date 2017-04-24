flask serves two purposes:
1) at initial boot and first client connect, the captive portal serves a simple page with a submit button. 

2) sets the Pi's 'home' lat long when the user selects one tracked devices icon in the web portal and clicks 'set as home'

On initial boot, a captive portal screen is shown, with a single button.

This button sends the viewing device's system time (e.g. iPhone) to the Pi and uses this to set the clock.

If the clock has been set, then it redirects all clients to the main tracker webpage.

There are two flask scripts started - one for localhost and one by IP - useful in testing. Probably a better way of getting it to work

