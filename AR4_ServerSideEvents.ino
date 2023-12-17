// File:        AR4_ServerSideEvents.ino

// Purpose:     Demonstrate how an Arduino R4 WiFi can implement server-side events (SSE).
//              SSE is a method for a server to continually send updated data to a client.
//              In this case the Uno R4 acts as the server.
//              The client is any web browser.
//              Both must be connected by WiFi to the same WiFi router, 
//              although router settings may be modified to allow connections from
//              clients beyond this.
//
// Created:     2023 Dec 11
// By:          Brian Mathewson
//
// How it works
//
//              The Arduino Uno R4 WiFi first connects to the WiFi router yous specify
//              in the separate arduino_secrets.h file. In this file enter the SSID and password
//              for your particular WiFi access point. As an example it could be like
//                #define SECRET_SSID "SantaClauseHouseWiFi"     (THESE ARE EXAMPLES ONLY!!!)
//                #define SECRET_PASS "MySecret8-ReindeerPassword"
//
//              The code takes about 10 seconds to connect and will show you the IP address
//              for connecting to the Uno R4, such as 192.168.6.120 (it could be almost any
//              four numbers from 0 to 255 separated by dots).
//
//              Important: After downloading the code, open the Serial Monitor and change
//              the speed from 9600 baud (bits/s) to 115200 baud. 
//
//              The above all occurs in the setup() part of the code.
//
//              In the loop() code it waits for a connection from a client.
//              So, in a web browser enter:
//                http://192.168.6.120     or whatever address you see in the Serial Monitor.
//
//              The code will identify the request for a default web page by seeing
//                GET / HTTP/1.1
//              where it only shows the "/" with no name after it, since the web page you
//              specified has nothing after the 120, for instance.
//
//              The Uno then sends the text of the web page as a reply to the client.
//              The Uno then closes the connection since it's done. 
//              This also frees up a connection for SSE below...
//
//              The reply starts with a header identifying it as an HTML file.
//              The web page text includes a short Javascript code area that, when it runs,
//              sends a request back to the server indicating an event-stream request.
//
//              When the Uno detects the request for an event-stream, which indicates it's
//              a Server-Side Event connection, it sends a message back to the client.
//              The SSE message to the client may include any sequence of text lines,
//              followed by an extra blank line to indicate the end of the SSE message.
//              An example of what the Uno R4 server sends to the client (web browser):
//
//                | id: 123
//                | data: Analog input 0 :562 id=1479.6.
//                |
//
//              These three lines (ignore the initial "| ") are sent by this demo program.
//              The id is not necessary. More data lines can be added.
//              Only UTF-8 characters are allowed, no binary since it could include 
//              a double-newline character that incorrectly indicates the end of the message.
//
//              After a given timeout the Uno will close the connection, just as a 
//              feature if needed.
//
//              More work needs to be done to handle multiple requests from multiple clients.
//              So this just shows you how the basic SSE mechanism works.
//
//              This example is written for a network using WPA encryption. For
//              WEP or WPA, change the WiFi.begin() call accordingly.
//
// Connections  Connect a variable voltage source, such as from a variable resistor,
//              to the analog input pin A0 to see live updates of the reading. 
//
//   Find the full UNO R4 WiFi Network documentation here:
//   https://docs.arduino.cc/tutorials/uno-r4-wifi/wifi-examples#wi-fi-web-server
//
//   Documentation on the WiFiS3 library is here:
//   https://www.arduino.cc/reference/en/libraries/wifinina
//

#include "WiFiS3.h"

// 
// WIRELESS GATEWAY CONNECTION
//
#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;        // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key index number (needed only for WEP)

WiFiServer server(80);

//
// CLIENT CONNECTION HANDLING
//
String pageRequest;               // Holds text of the latest page request
int wifiStatus = WL_IDLE_STATUS;

unsigned long timeOfFirstUpdate;
unsigned long timeOfLastUpdate;

// The program displays on the Serial Monitor a one-line message on the client status at the specified intervals.
const unsigned long PRINT_CLIENT_STATUS_INTERVAL_MILLISECONDS = 4000;

//
// SERVER-SENT DATA HANDLING
//
// You can experiment with these values.
const unsigned long CONNECTION_TIME_LIMIT_SECONDS = 20;
const unsigned long CONNECTION_UPDATE_RATE_MILLISECONDS = 400;


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while ( wifiStatus != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    wifiStatus = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();
  // you're connected now, so print out the status:
  PrintWifiStatus();
}


unsigned long printUpdateTime;

void PrintClientStatus( WiFiClient *pStreamClient )
{
  Serial.print("At t=");
  Serial.print( millis()) ;
  Serial.print(" con= ");
  Serial.print( pStreamClient->connected() );
  Serial.print(" avail= ");
  Serial.print( pStreamClient->available() );
  Serial.print(" status= ");
  Serial.println(pStreamClient->getTimeout() );
}


void LogClientStatusAtIntervals( WiFiClient *pStreamClient )
{
  if( (millis() - printUpdateTime) > PRINT_CLIENT_STATUS_INTERVAL_MILLISECONDS )
  {
    printUpdateTime = millis();
    PrintClientStatus( pStreamClient );
  }
}



// Try to update the client with stream data at the given interval.
void UpdateEventStreamAtStreamInterval( WiFiClient *pStreamClient, unsigned long millisecondUpdateTime, unsigned long connectionTimeLimitSeconds )
{
  if( pStreamClient->connected() )
  {
    if( (millis() - timeOfLastUpdate) > millisecondUpdateTime )
    {
      timeOfLastUpdate = millis();
      SendEventStreamData( pStreamClient, (timeOfLastUpdate / 100) );
    }

    // Close after 1 minute
    if( (millis() - timeOfFirstUpdate) > (connectionTimeLimitSeconds*1000) )
    {
      // Close connection.
      pStreamClient->stop();
      Serial.print("Server disconnected connection after ");
      Serial.print( connectionTimeLimitSeconds );
      Serial.println(" seconds.");
    }
  }
}


void loop() {

  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {

    // an HTTP request ends with a blank line
    boolean currentLineIsBlank = true;

    if( client.connected() )
    {
      Serial.println("*** Client connect ***");
    }

    while ( client.connected() )
    {
      if (client.available() )  
      {
        char c = client.read();

        // Accumulate the full page request text.
        pageRequest = pageRequest + c;

        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the HTTP request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) 
        {
          // Print the page request.
          Serial.println("<<< Page request start >>>");
          Serial.print(pageRequest);
          Serial.println("<<< Page request end >>>");

          // If a GET / HTTP/1.1 page request
          if( pageRequest.indexOf( "GET / HTTP" ) > -1 )
          {
            Serial.println("*** Sending web page. ***");

            // Send a standard HTTP response header
            SendHtmlHeader( &client );

            // Send the web page
            SendWebPageTop( &client );
            SendWebPageScript( &client );
            SendWebPageSSEArea( &client );
            SendWebPageFooter( &client );

            // Close connection.
            PrintClientStatus( &client );
            client.stop();
            Serial.println("HTTP client disconnect.");
            PrintClientStatus( &client );

            pageRequest = "";
          }

          // If an event stream page request
          if( pageRequest.indexOf( "event-stream" ) > -1 )
          {
            Serial.println("*** SSE event-stream starting. ***");

            SendEventStreamHeader( &client );

            SendEventStreamData( &client, 1 );
            timeOfFirstUpdate = millis();
            timeOfLastUpdate = timeOfFirstUpdate;

            pageRequest = "";
          }

          // Because we saw the end of a page request from the client,
          // break out of while() loop.
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      } else
      {
        // We have client.available() but no characters received from the client (no requests in process), 
        // so send updates via SSE at the specified rate.
        UpdateEventStreamAtStreamInterval( &client, CONNECTION_UPDATE_RATE_MILLISECONDS, CONNECTION_TIME_LIMIT_SECONDS );
      }

      // This logs the status while a connection is established.
      LogClientStatusAtIntervals( &client );
    }
  }
  // This logs the status when there is no connection.
  LogClientStatusAtIntervals( &client );
}


void PrintWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// End of program.
