# tft24T    V0.2 April 2015     Brian Lavery    TJCTM24024-SPI    2.4 inch Touch 320x240 SPI LCD

#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so.

# A demo of LCD/TFT SCREEN DISPLAY

#1 basic refresh and fetch
#2
#4 getting distances to work

import Image
import ImageDraw
import ImageFont
import pycurl
from StringIO import StringIO
import json
from urllib import urlencode
import time
import datetime
import os
import math
import ast 


from lib_tft24T import TFT24T
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

import spidev
from time import sleep

homelat=0
homelon=0
HOMEFILENAME = '/tmp/home.txt'
homeset=0
homefilemodified=""
strobe=0
def distance(origin, destination):
    lat1, lon1 = origin
    lat2, lon2 = destination
    print 'origin' + str(origin)
    print 'dest' + str(destination)
    radius = 6371*1000 # km

    dlat = math.radians(lat2-lat1)
    dlon = math.radians(lon2-lon1)
    a = math.sin(dlat/2) * math.sin(dlat/2) + math.cos(math.radians(lat1)) \
        * math.cos(math.radians(lat2)) * math.sin(dlon/2) * math.sin(dlon/2)
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1-a))
    d = radius * c

    return d

def getDevices():
    print "################"
    print "login"
    buffer = StringIO()
    c=pycurl.Curl()
    data="[\"rest\",\"rest\"]"
    #data2=urlencode(data)
    #data = {'user':'rest','password':'rest'}
    #postfields = urlencode(data)
    myurl = 'pitracker.local:8082/traccar/rest/login?payload='+data
    c.setopt(c.URL,myurl)
    c.setopt(c.COOKIEFILE,'')
    #c.setopt(c.POSTFIELDS,postfields)
    c.setopt(c.WRITEDATA,buffer)
    myloop=1
    while myloop==1:
        print ("trying to connect")
        try:
            c.perform()
        except pycurl.error, e:
            print ("not yet alive on 8082...")
            sleep(5)
            continue
        else:
            myloop=0
    
    c.close
    body=buffer.getvalue()
    #print(body)
    
    
    print "################"
    print "devices"
    myurl='pitracker.local:8082/traccar/rest/getDevices'
    c.setopt(c.URL,myurl)
    c.setopt(c.WRITEDATA,buffer)
    c.perform()
    c.close
    body=buffer.getvalue()
    #print(body)
    
    print "################"
    print "positions"
    # getpositions for all devices previously reecived
    myurl='pitracker.local:8082/traccar/rest/getLatestPositions'
    c.setopt(c.URL,myurl)
    c.setopt(c.WRITEDATA,buffer)
    #c.setopt(c.POSTFIELDS,body)
    c.perform()
    c.close
    body=buffer.getvalue()

    i=0
    #print(body)
    json_obs=body.split("][")
    for obj in json_obs:
        print 'item '+str(i)
        #print obj
        if (i==1):
            obj='['+obj
            #obj=obj+']'
            #print obj
            out=json.loads(obj)
            return out
        i=i+1
      
def getTime():
    mytime= time.strftime('%H:%M:%S',time.localtime(time.time()))
    return mytime

# Raspberry Pi configuration.
#For LCD TFT SCREEN:
DC = 25
RST = 23
LED = 18

#height of each device 
ROWHEIGHT=30
HEADERHEIGHT=52


#For PEN TOUCH:
#   (nothing)

# Create TFT LCD/TOUCH object:
TFT = TFT24T(spidev.SpiDev(), GPIO, landscape=False)
# If landscape=False or omitted, display defaults to portrait mode
# This demo can work in landscape or portrait


# Initialize display.
TFT.initLCD(DC, RST, LED)
# If rst is omitted then tie rst pin to +3.3V
# If led is omitted then tie led pin to +3.3V

# Get the PIL Draw object to start drawing on the display buffer.
draw = TFT.draw()

#get home position


TFT.clear()
font = ImageFont.truetype('/home/pi/fonts/FreeSerifItalic.ttf', 25)
font_time = ImageFont.truetype('/home/pi/fonts/FreeSerifItalic.ttf', 16)
font_header = ImageFont.truetype('/home/pi/fonts/FreeMonoBold.ttf',16)
#oneoff time update
draw.text((170,12),getTime(),font=font_time,fill="WHITE")
#oneoff header set and get pos
def setHome():
    global homelat
    global homelon
    global homeset
    if (os.path.isfile(HOMEFILENAME)):
        #home now set - change header
        print 'file exists'
        homeset=1
        #TFT.textdirect((55, 5), ('Tracking...'), font=font, fill="YELLOW")   # signature !
        homefile=open(HOMEFILENAME)
        pos=homefile.read()
        homefile.close()
        homepos=pos.split(',')
        statbuf=os.stat(HOMEFILENAME)
        homefilemodified=statbuf.st_mtime
        homelat=homepos[0]
        homelon=homepos[1]
        print 'HOME LAT:' + str(homelat)
        print 'HOME LON:' + str(homelon)

        
    
    else:
        homeset=0
    return
        
   ###################
def drawHeader():
    #draw horizontal line     
    draw.line((0, 35, 239, 35), fill="white")
    #headers
    draw.text((15,37),'Name',font=font_header,fill="RED")
    draw.text((105,37),'Dist(m)',font=font_header,fill="RED")
    draw.text((180,37),'Age(s)',font=font_header,fill="RED")
    #hor
    draw.line((0, HEADERHEIGHT, 239, HEADERHEIGHT), fill="white")
    #vert1
    draw.line((100,HEADERHEIGHT-15,100,319),fill="WHITE")
    #vert2
    draw.line((170,HEADERHEIGHT-15,170,319),fill="WHITE")
    if (homeset==0):
        draw.text((2, 5), 'HOME NOT SET', font=font, fill="YELLOW")   # signature !
    else:
        draw.text((55, 5), ('Tracking...'), font=font, fill="YELLOW")   # signature !
    devs=getDevices()
    yoff=0
    for people in devs:
        draw.line((0, ROWHEIGHT+HEADERHEIGHT+yoff, 239, ROWHEIGHT+HEADERHEIGHT+yoff), fill="grey")
        yoff+=30
#set grid

print 'drawing header'
drawHeader()
TFT.display()
print 'done header'

doneredraw=0
while 1:
    print 'LOOP'
    if (homeset==1):
        print 'home exists'
        if (doneredraw==0):
            TFT.clear()
            drawHeader()
            draw.text((55, 5), ('Tracking...'), font=font, fill="YELLOW")   # signature !
            TFT.display()
            doneredraw=1
            
        #check for change
        statbuf=os.stat(HOMEFILENAME)
        if (homefilemodified!=statbuf.st_mtime):
            print 'RELOADING HOME'
            setHome()
            print homefilemodified
            print statbuf.st_mtime
            homefilemodified=statbuf.st_mtime
            
    else:
        print 'home not exist in while'
        setHome()
    
        
   ###################

    #TFT.clear((255, 0, 0))

    # Alternatively can clear to a black screen by simply calling:
    
    dev=getDevices()
    print dev[0]
    #GET HOME LOCATION
    #if file exists:
        #open file and read lat long
    #lat,long=readfile etc
        #homeisset=True
    #else: homeisset=False

########################
 #HEADING
    
    
##GET ALL DEVICES FROM TRACCAR
    ##LOGIN - get cookie id
    
    
            #returns JSON payload
    
#FOR EACH DEVINCE
    #get position
    # if homeisset
    #write name
    #advance yreg by lineheight

    ##now have devices in dictionary  OUT
    yreg=1
    for person in dev:
        
        #print NAME
        TFT.textdirect((0,(yreg+HEADERHEIGHT)),"                ",font=font,fill="WHITE")
                    
        TFT.textdirect((0,(yreg+HEADERHEIGHT)),person['device']['name'],font=font,fill="WHITE")
        #print 'int lat' + str(int(person['longitude']))
        print person['device']['name']
        print 'done draw one person'
        
        
        #AGE
        
        lastupdate=person['device']['lastUpdate']
        
        
        #print 'lastupdate' + lastupdate
        #local time
        tlocal=time.localtime(time.time())
        #print 'tlocal: ' + str(tlocal)
        sformat="%a, %d %b %Y %H:%M:%S"
        #print lastupdate[0:25]
        dt1=datetime.datetime.strptime(lastupdate[0:25],sformat)
        dt2=time.strftime(sformat,time.localtime(time.time()))
        dt3=datetime.datetime.strptime(dt2[0:25],sformat)
        #print 'dt1: ' + str(dt1)
        #print 'dt2: ' + str(dt2)
        #print 'dt3: ' + str(dt3)
        mdiff= dt3-dt1
        
        print 'diff ' + str(int(mdiff.total_seconds()))
        if (mdiff.total_seconds()>300 or mdiff.total_seconds() < 0 ):
            mdiff='---'
        else:
            mdiff=str(int(mdiff.total_seconds()))
            #TODO pad with spaces to blank out previous characters
        TFT.textdirect((175,yreg+HEADERHEIGHT),"       ",font=font,fill="WHITE")
        TFT.textdirect((175,yreg+HEADERHEIGHT),mdiff,font=font,fill="WHITE")
        
        #Get person's lat/lon
        #DISTANCE
        if (homeset==0 or int(person['longitude'])==0 or int(person['latitude'])==0 or mdiff=='---'):
            dist='---'
            TFT.textdirect((105,yreg+HEADERHEIGHT),"          ",font=font,fill="WHITE")
            TFT.textdirect((105,yreg+HEADERHEIGHT),dist,font=font,fill="WHITE")
        else:
            #haversine
            lat1=person['latitude']
            lon1=person['longitude']
            lat2=float(homelat)
            lon2=float(homelon)
            print 'lat2:' + str(lat2)
            print 'lon2:' + str(lon2)
            dist=distance((lat1,lon1),(lat2,lon2))
            TFT.textdirect((105,yreg+HEADERHEIGHT),"        ",font=font,fill="WHITE")
            TFT.textdirect((105,yreg+HEADERHEIGHT),str(int(dist)),font=font,fill="WHITE")
        print 'DIST: ' + str(dist)
 #FOR TESTING made !=
        if (mdiff=='---'):
            pass
        else:
            #HOME strobing
        
            z1= person['other']
            print z1
            z2=ast.literal_eval(z1)
            print z2
            hdop=999
            try:
                hdop= z2['hdop']
            except:
                pass
            print 'hdop' + str(hdop)
            if (hdop!=0.0):
                #HOME IS SET - strobe
                if (strobe==1):
                    strobe=0
                    TFT.textdirect((0,(yreg+HEADERHEIGHT)),"                ",font=font,fill="WHITE")
                    TFT.textdirect((0,(yreg+HEADERHEIGHT)),person['device']['name'],font=font,fill="WHITE")
                else:
                    strobe=1
                    TFT.textdirect((0,(yreg+HEADERHEIGHT)),"                ",font=font,fill="WHITE")
                    TFT.textdirect((0,(yreg+HEADERHEIGHT)),"HOME",font=font,fill="WHITE")
                

        
        
        
        
        #move to next row
        yreg+=ROWHEIGHT
    
    #update clock
    TFT.textdirect((170,12),getTime(),font=font_time,fill="WHITE")
    sleep(1)
    yreg=0
    

#        All colours may be any notation (exc for clear() function):
#        (255,0,0)  =red    (R, G, B) - a tuple
#        0x0000FF   =red    BBGGRR   - note colour order
#        "#FF0000"  =red    RRGGBB   - html style
#        "red"      =red    html colour names, insensitive


