# Arduino-R4-WiFi-Server-Side-Events-Demonstration-Program

Updated 2024 April 5 - Brian Mathewson

Demonstrates how to make the Uno R4 WiFi serve both a web page and service Server-Side Events (SSE).

To run this program both the Uno and a WiFi-enabled device (mobile phone or PC) must be connected to the same WiFi access point.
You must configure the SSID (WiFi name) and password in an arduino_secrets.h file that you have to create (not included in the upload; see Arduino Uno R4 WiFi example code).

To know the IP address of the Uno R4 you must look at the Serial Monitor for the IP address, at least initially.
IMPORTANT: 
   Be sure to set the baud rate (bits/second) of the Serial Monitor to 115200 rather than the default 9600.

The Uno R4 acts as a web server, sending a web page.
Then javascript on the web page request a server-sent event stream from the Uno R4, 
which responds with an analog reading and a time value in seconds so you can see it incrementing.

NEXT STEPS

o Update the HTML web page content as you please.

o Also update the content and format of the data the server sends out.
The javascript could parse the data and send it to different elements of the web page.

o The program closes connections after 20 seconds. Most Web browsers will attempt to reopen the connection but you will notice a delay of a few seconds without updates. Update the time to whatever makes sense. 

o You can also test increasing the number of supported connections if desired. It is only 3 to make it easier for testing what happens when more clients attempt to access the device than the number of available channels. 

--- End ---
