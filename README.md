aiswatcher
==========
Displays live ship positions on a map from ships' AIS vhf radio broadcasts.

AisWatcher builds and runs on most linux platfroms including BeagleBone, Raspberry Pi and Mac for use 
with RTL2832U (RTL SDR usb sticks).

see http://www.rtl-sdr.com/buy-rtl-sdr-dvb-t-dongles/ for the radio dongles.

AisWatcher decodes the radio data and converts it to AIS ascii data strings. 
You can dump the strings to the console and also forward them to the BoatBeaconApp.com server to display on a live map view.

This code is based on a rework of aisdecoder from aishub.net (http://forum.aishub.net/ais-decoder/ais-decoder-beta-release/) 
 to include rtl-sdr (http://sdr.osmocom.org/trac/wiki/rtl-sdr) to run as one command and add displaying ships on a live map web view.

Build instructions
==================

git clone https://github.com/electricpocket/aiswatcher.git

cd aiswatcher
<p>mkdir build
<p>cd build
<p>cmake ../ -DCMAKE_BUILD_TYPE=RELEASE
<p>make

To run:-

./aiswatcher -h boatbeaconapp.com -p 7004 -t 1 -d -l -f /tmp/aisdata -P 54 -D 0

You can then view the live ships on this web page:-

http://boatbeaconapp.com/station/7004

Please email us at "support at pocketmariner.com" if you would like your own port and dedicated web page to view
data from your setup.

AisWatcher defaults to using the lower AIS radio frequency - 161.975MHz. Pass "-F 162025000" to use the alternative AIS band.
Fine tune gain parameter (-G )  and most important ppm error ( -P ) 

./aiswatcher -H to see all the options.
