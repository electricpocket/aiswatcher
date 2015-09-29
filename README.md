aiswatcher
==========
Displays live ship positions on a google map from ships' AIS vhf radio broadcasts.

AisWatcher builds and runs on most linux platfroms including BeagleBone, Raspberry Pi and Mac together with a RTL2832U (RTL SDR usb stick).

See http://www.rtl-sdr.com/buy-rtl-sdr-dvb-t-dongles/ for the radio usb stick.

AisWatcher decodes the radio data and converts it to AIS ascii data strings. You can dump the strings to the console and also forward them to the <a href="http://boatbeaconapp.com">BoatBeaconApp.com</a> server to display on a live map view and in the free <a href="http://boatwatchapp.com">Boat Watch apps</a>, available for iPhone, iPad, Mac and Android.

This code is based on a rework of aisdecoder from aishub.net (http://forum.aishub.net/ais-decoder/ais-decoder-beta-release/) 
 to include rtl-sdr (http://sdr.osmocom.org/trac/wiki/rtl-sdr) to run as a single command and diplay ships live on a google map.

Pre-requisites
==============

Clone and build the rtl-sdr library. see http://sdr.osmocom.org/trac/wiki/rtl-sdr

Make sure rtl_fm runs and it is in your path. (e.g. $PATH).

Build instructions
==================

git clone https://github.com/pocketmariner/aiswatcher.git

cd aiswatcher
<p>mkdir build
<p>cd build
<p>cmake ../ -DCMAKE_BUILD_TYPE=RELEASE
<p>make

To run:-

./aiswatcher -h boatbeaconapp.com -p 7028 -t 1 -d -l -f /tmp/aisdata -P 54 -D 0

You can then view the live ships on this web page:-

http://boatbeaconapp.com/station/7028

Please email us at "support at pocketmariner.com" if you would like your own port and dedicated web page to view
data from your setup.

AisWatcher defaults to using the lower AIS radio frequency - 161.975MHz. Pass "-F 162025000" to use the alternative AIS band.
Fine tune gain parameter (-G )  and most important set the ppm offset error ( -P ). You can use the gqrx app to find out what the ppm needs to be set to by tuning in to a known VHF radio station by adjusting the ppm value on gqrx.

http://gqrx.dk/

./aiswatcher -H to see all the options.

We have also successfully developed a dual band AIS receiver for BeagleBone using gnu-radio.
http://pocketmariner.com/ais-ship-tracking/cover-your-area/pocket-mariner-ais-dual-channel-receiver-for-99/
