# Arduino-R4-WiFi-Server-Side-Events-Demonstration-Program
#
# Updated 2023 December 16 - Brian Mathewson
#
Demonstrates how to make the Uno R4 WiFi serve both a web page and service Server-Side Events (SSE).

To run this program both the Uno and a WiFi-enabled device (mobile phone or PC) must be connected to the same WiFi access point.
You must configure the SSID (WiFi name) and password in an arduino_secrets.h file that you have to create (not included in the upload; see Arduino Uno R4 WiFi example code).

To know the IP address of the Uno R4 you must look at the Serial Monitor for the IP address, at least initially.
Be sure to set the baud rate (bits/second) of the Serial Monitor to 115200 rather than the default 9600.

The Uno R4 acts as a web server, sending a web page.
Then javascript on the web page request a server-sent event stream from the Uno R4, 
which responds with an analog reading and a time value in seconds so you can see it incrementing.

NEXT STEPS

o Update the HTML web page as you please.

o Also update the content and format of the data the server sends out.
The javascript could parse the data and send it to different elements of the web page.

o Develop a version which supports multiple simultaneous users.

--- End ---
