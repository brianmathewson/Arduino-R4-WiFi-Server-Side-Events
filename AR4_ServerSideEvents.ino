// File:        ServerSideEvents.ino
//
// Platform:    Arduino R4 WiFi
//              Firmware version 4.1 or later
//              GitHub
//
// Purpose:     Demonstrate how an Arduino R4 WiFi can implement server-side events (SSE).
//              SSE is a method for a server to continually send updated data to a client.
//              In this case the Uno R4 acts as the server.
//              The client is any web browser.
//              Both must be connected by WiFi to the same WiFi router, 
//              although router settings may be modified to allow connections from
//              clients beyond this (see Port Forwarding).
//
// Copyright © 2024 Brian B. Mathewson
//
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the “Software”), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
// copies of the Software, and to permit persons to whom the Software is 
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
// SOFTWARE.
//
// Updated:     2024 March 23
// By:          Brian Mathewson
// Updates:     Supports multiple concurrent client connections when used with 
//              Uno 4 WiFi firmware 0.4.1 or later. May not support multiple
//              concurrent connections with 0.3.x or earlier.
//
// How it works
//
//              The Arduino Uno R4 WiFi first connects to the WiFi router you specify
//              in the separate arduino_secrets.h file. In this file enter the SSID and password
//              for your particular WiFi access point. As an example it could be like
//                #define SECRET_SSID "SantaClauseHouseWiFi"     (THESE ARE EXAMPLES ONLY!!!)
//                #define SECRET_PASS "MySecret8-ReindeerPassword"
//
//              The code takes about 10 seconds to connect.
//              The IP address for connecting to the Uno R4, such as 192.168.6.120 (it could be almost
//              any four numbers from 0 to 255 separated by dots), is shown in the Serial Monitor
//              when the Uno restarts.
//
//              Important: Open the Serial Monitor and change the speed from 9600 baud (bits/s) to 115200 baud. 
//
//              The above all occurs in the setup() part of the code.
//
//              The loop() code waits for a connection from a client.
//              So, in a web browser enter:
//                http://192.168.6.120     or whatever address you see in the Serial Monitor.
//
//              The SSE code will identify the request for a default web page by seeing
//                GET / HTTP/1.1
//              where it only shows the "/" with no name after it, since the web page you
//              specified has nothing after the 120, for instance.
//
//              The Uno, acting as a web server, then sends the text of the web page 
//              as a reply to the client. 
//              The Uno then closes the connection since it's done sending the web page. 
//              This also frees up a connection for Server Side Events.
//
//              The Uno sends a web page with a header identifying it as an HTML file.
//              The web page text includes a short Javascript code area that, when it runs
//              on the user's browser, sends a request back to the server (Uno) indicating 
//              an event-stream request.
//
//              When the Uno detects the request for an event-stream, which indicates it's
//              a Server-Side Event connection, the Uno sends a data message back to the client.
//              Each data message to the client may include any sequence of text lines,
//              followed by an extra blank line to indicate the end of the SSE message.
//              An example of what the Uno server sends to the client (web browser):
//
//                | id: 14796
//                | data: Analog input 0 :562 at t=1479.6.
//                |
//
//              These three lines (indicated by the initial "| ") are sent by this demo program.
//              The id is customary but not necessary. More data lines can be added.
//              Only UTF-8 characters are allowed, no binary since it could include 
//              a double-newline character that incorrectly indicates the end of the message.
//
//              Code on the Uno cycles through its list of open client connections, 
//              and it checks if it's time to send an update. This is a configurable interval.
//              If so it sends the SSE message to the client and updates the time the
//              last message was sent so it knows when to send the next one.
//            
//              After a configurable session timeout (by default just 20 seconds)
//              the Uno will close the connection. This helps clean up old connections. 
//              Generally if the client's web browser is still open then it will
//              attempt to reconnect with the Uno, typically if 5 seconds has passed
//              without an update. 
//              A short (5 second) disruption in communication is usually observable. 
//              You can modify the session timeout for your application. 
//
//              This example is written for a WiFi network using WPA encryption. 
//              For WEP or WPA, change the WiFi.begin() call accordingly.
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

using namespace std;

// =================================================================================
// WIRELESS GATEWAY CONNECTION
//
#include "arduino_secrets.h" 
///////please enter your sensitive data in the tab arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;        // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key index number (needed only for WEP)

WiFiServer server(80);


// =================================================================================
// SERIAL MONITOR OUTPUT CONFIGURATION
//

// Set to 1 to show the full text of each HTTP message request from clients, 0 otherwise.
int showPageRequests = 0;

int activityOutputsEnabled = 0;
int activityOutputsEnabledTrigger = 20;
int freeClientIndex = 0;


// =================================================================================
// CLIENT CONNECTION HANDLING
//
int wifiStatus = WL_IDLE_STATUS;

// =================================================================================
// CLIENT SSE CONNECTION HANDLING
//
// Support multiple simultaneous SSE clients.
#define MAX_SSE_DATA_CLIENTS   3

// Holds a WiFi connection and associated data so the code can serve multiple connections
// simultaneously.
struct SseDataClient
{
  WiFiClient client;
  unsigned long timeOfFirstUpdate;
  unsigned long timeOfLastUpdate;
  String pageRequest;               // Holds text of the latest page request
};

struct SseDataClient dataClient[MAX_SSE_DATA_CLIENTS];

// =================================================================================
// CLIENT STATUS
//
// The program displays on the Serial Monitor a one-line message on the client status at the specified intervals.
const unsigned long PRINT_CLIENT_STATUS_INTERVAL_MILLISECONDS = 250;

// =================================================================================
// SERVER-SENT DATA HANDLING
//
// You can experiment with these values.
const unsigned long CONNECTION_TIME_LIMIT_SECONDS = 20;
const unsigned long CONNECTION_UPDATE_RATE_MILLISECONDS = 100;


// =================================================================================
// DATA CLIENT HANDLING ROUTINES
//


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
  Serial.print("Firmware version: ");
  Serial.println(fv);
  Serial.print("Firmware latest version: ");
  Serial.println( WIFI_FIRMWARE_LATEST_VERSION );

  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  byte mac[6];                     // the MAC address of your Wifi shield

  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[5],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.println(mac[0],HEX);


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



void PrintClientStatus( SseDataClient *pStreamClient, int cIndex )
{
  Serial.print("\t[");
  Serial.print(cIndex);
  Serial.print(":");
  Serial.print( pStreamClient->timeOfFirstUpdate );
  //  Serial.print(" upd= ");
  //  Serial.print( pStreamClient->timeOfLastUpdate );
  //  Serial.print(" avail= ");
  //  Serial.println( pStreamClient->client.available() );
  Serial.print("]");
}



// The Uno 4 WiFi USB bridge firmware typically communicates with the ESP32
// using a serial communications protocol, called a modem protocol.
// This subroutine demonstrates how it can also be used at the application
// level though it's not recommended.
// The main reason to use this is if the ESP32 modem protocol supports something
// that isn't supported by function calls using the WiFi bridge firmware.
//
void TryModem()
{
  uint8_t rv = 0;
  int socketNumber;
  string res = "";

  Serial.print("<modem>");
  modem.begin();

  Serial.print("<modem.write>");
  if(modem.write(string(PROMPT(_AVAILABLE)),res, "%s%d\r\n" , CMD_WRITE(_AVAILABLE), socketNumber )) {
    rv = atoi(res.c_str());
    if (rv < 0) {
        Serial.println("Avail was 0");
    }
    Serial.print("Avail was");
    Serial.println(rv);
  }
  Serial.println("</modem>");
}


//
// Shows the list of clients and whether any of them are active connections
// and, if so, if there is pending data from the client available.
//
void LogDataClientStatusAtIntervals( int showAllNow = 0 )
{
  static unsigned long logDataClientUpdateTime;
  int i;

  if( ( (millis() - logDataClientUpdateTime) > PRINT_CLIENT_STATUS_INTERVAL_MILLISECONDS ) || (showAllNow > 0)  )
  {
    Serial.print("At t=");
    Serial.print( millis() );
    Serial.print(" ");
    
    logDataClientUpdateTime = millis();
    for(i=0; i<MAX_SSE_DATA_CLIENTS; i++)
    {
      PrintClientStatus( &dataClient[i], i );
    }

    Serial.println();

    // TryModem();

  }
}



//
// Update the client with stream data at the given interval.
//
// Note: The connectionIndex parameter is just for diagnosis.
//
void UpdateEventStreamAtStreamInterval( struct SseDataClient *pDataClient, unsigned long millisecondUpdateTime, unsigned long connectionTimeLimitSeconds, int connectionIndex )
{
  if( pDataClient->client.connected() )
  {
    // Check if it's time to send a new update.
    if( (millis() - pDataClient->timeOfLastUpdate) > millisecondUpdateTime )
    {
      // Refresh last update time
      pDataClient->timeOfLastUpdate = millis();

      SendEventStreamData( &pDataClient->client, (pDataClient->timeOfLastUpdate / 100) );
    }

    // Stop the client after the specified connection time limit.
    if( (millis() - pDataClient->timeOfFirstUpdate) > (connectionTimeLimitSeconds*1000) )
    {
      // Close connection.
      pDataClient->client.stop();
      // Diagnostic output.
      Serial.print("Server disconnected connection [");
      Serial.print( connectionIndex );
      Serial.print("] at ");
      Serial.print( pDataClient->timeOfFirstUpdate );
      Serial.print( " after " );
      Serial.print( connectionTimeLimitSeconds );
      Serial.println(" seconds");
      // Zero these for consistency.
      pDataClient->timeOfFirstUpdate = 0;
      pDataClient->timeOfLastUpdate = 0;
    }
  }
}


int SelectNextClient()
{
  WiFiClient nextClient;
  int i;
  int selectedClientIndex = -1;
  unsigned long oldestConnectionTime;
  int oldestConnectionIndex;

  oldestConnectionTime = millis();
  oldestConnectionIndex = 0;

  // Find an open connection OR the oldest connection
  for(i=0; i<MAX_SSE_DATA_CLIENTS; i++)
  {
    if( dataClient[i].client.connected() )
    {
      // Find the oldest connection of all the connected clients
      if( (dataClient[i].timeOfFirstUpdate < oldestConnectionTime) ) 
      {
        oldestConnectionTime = dataClient[i].timeOfFirstUpdate;
        oldestConnectionIndex = i;
      }
    } else
    {
      // No connection, use this channel.
      selectedClientIndex = i;
    }
  }

  // If nothing's selected, pick the oldest one.
  if( selectedClientIndex == -1 )
  {
    Serial.print( "[oldest cIdx=" );
    Serial.print( oldestConnectionIndex );
    Serial.println( "] " );
    selectedClientIndex = oldestConnectionIndex;
  }

  // Check if any new data has arrived
  nextClient = server.available();

  // New data has arrived and/or a new client.  
  if( nextClient ) 
  {
    // See if it matches an existing connected client.
    for(i=0; i<MAX_SSE_DATA_CLIENTS; i++)
    {
      if( nextClient == dataClient[i].client ) 
      {
        selectedClientIndex = i;
        break;    // break out of for() loop
      }
    }
    // TODO
    //  I wonder if selectedClientIndex will always be correct here?
    //  If it finds a match just above, then it doesn't need to do this, right?  But maybe it's benign, just a second copy?
    //  But if it uses selectedClientIndex from earlier (an open connection or the oldest)
    //  then this step is fine.
    dataClient[selectedClientIndex].client = nextClient;
  }

  if( activityOutputsEnabled )
  {
    Serial.print("sCI:");
    Serial.print(selectedClientIndex);
    Serial.print(" nc=");
    Serial.println( nextClient );
  }

  return selectedClientIndex;
}



// Selects any open clients that are currently waiting for server-side events.
// Returns:
//  -1: no SSE channels open; do nothing
//  0..(MAX_SSE_DATA_CLIENTS-1) : the channel index is open for SSE
//
// On repeated calls, cycles through all channels open for SSE.
//
int SelectCurrentClient()
{
  static int connectedClientIndex = 0;
  int selectedClientIndex = -1;
  int i;

  // This looks for the next channel with an active connection. 
  // If it finds one it exits the for loop. 
  //
  for( i=0; i<MAX_SSE_DATA_CLIENTS; i++ )
  {
    // Advance to the next index, in round-robin fasion.
    connectedClientIndex = (connectedClientIndex + 1) % MAX_SSE_DATA_CLIENTS;
  
    // If this client channel indicates it's open for SSE, select it.
    if( dataClient[connectedClientIndex].client.connected() && (dataClient[connectedClientIndex].timeOfFirstUpdate > 0) ) 
    {
      selectedClientIndex = connectedClientIndex;

      // Exit the for() loop.
      break;
    }
  }

  return selectedClientIndex;
}



void loop() {
  int i;
  int serverUpdateChannelIndex;
  
  if( activityOutputsEnabledTrigger > 0 )
  {
    if( activityOutputsEnabledTrigger == 1 )
    {
      activityOutputsEnabled = 0;
    } 
    else
    {
      activityOutputsEnabled = 1;
    }
    activityOutputsEnabledTrigger -= 1;    // Can trigger multiple rounds
  }

  // Select a client channel for receiving new connection requests from a client.
  freeClientIndex = SelectNextClient();

  // If the client has a valid socket
  if (dataClient[freeClientIndex].client) 
  {
    // An HTTP request ends with a blank line.
    boolean currentLineIsBlank = true;

    while ( dataClient[freeClientIndex].client.connected() )
    {

      if (dataClient[freeClientIndex].client.available() )  
      {
        char c = dataClient[freeClientIndex].client.read();

        // Accumulate the full page request text.
        dataClient[freeClientIndex].pageRequest = dataClient[freeClientIndex].pageRequest + c;

        // If you've gotten to the end of the line (received a newline
        // character) and the line is blank, the HTTP request has ended,
        // so you can send a reply.
        if (c == '\n' && currentLineIsBlank) 
        {
          // ProcessPageRequest( dataClient[freeClientIndex] );

          // Print which client this is for.
//          Serial.print("<<< Received msg cIdx=");
//          Serial.print( freeClientIndex );
//          Serial.println( " >>>");

          // Print the page request.
          Serial.println("<<< Page request start >>>");
          Serial.print(dataClient[freeClientIndex].pageRequest);
          Serial.println("<<< Page request end >>>");

          // If a GET / HTTP/1.1 page request
          if( dataClient[freeClientIndex].pageRequest.indexOf( "GET / HTTP" ) > -1 )
          {
            Serial.println("*** Sending web page. ***");

            // Send a standard HTTP response header
            SendHtmlHeader( &dataClient[freeClientIndex].client );

            // Send the web page
            SendWebPageTop( &dataClient[freeClientIndex].client );
            SendWebPageScript( &dataClient[freeClientIndex].client );
            SendWebPageSSEArea( &dataClient[freeClientIndex].client );
            SendWebPageFooter( &dataClient[freeClientIndex].client );

            // Close connection.
            // PrintClientStatus( &dataClient[freeClientIndex].client );
            LogDataClientStatusAtIntervals( 1 );
            dataClient[freeClientIndex].client.stop();
            Serial.println("*** HTTP client disconnect ***");
            // For consistency
            dataClient[freeClientIndex].timeOfFirstUpdate = 0;
            dataClient[freeClientIndex].timeOfLastUpdate = 0;
            // PrintClientStatus( &dataClient[freeClientIndex].client );
            LogDataClientStatusAtIntervals( 1 );

            dataClient[freeClientIndex].pageRequest = "";
          }

          // If an event stream page request
          if( dataClient[freeClientIndex].pageRequest.indexOf( "event-stream" ) > -1 )
          {
            Serial.println("SSE event-stream starting.");

            SendEventStreamHeader( &dataClient[freeClientIndex].client );

            SendEventStreamData( &dataClient[freeClientIndex].client, 0 );

            dataClient[freeClientIndex].timeOfFirstUpdate = millis();
            dataClient[freeClientIndex].timeOfLastUpdate = dataClient[freeClientIndex].timeOfFirstUpdate;

            dataClient[freeClientIndex].pageRequest = "";

            // After a page request, turn on activity for a short time.
//          activityOutputsEnabledTrigger = 5;
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
        break;
      }
    }
  }

  // 
  // Send updates to active channels
  //
  serverUpdateChannelIndex = SelectCurrentClient();
  if( serverUpdateChannelIndex >= 0 )
  {
    UpdateEventStreamAtStreamInterval( &dataClient[serverUpdateChannelIndex], CONNECTION_UPDATE_RATE_MILLISECONDS, CONNECTION_TIME_LIMIT_SECONDS, serverUpdateChannelIndex );
  }

  // This logs the status while a connection is established.
  LogDataClientStatusAtIntervals();

  //
  // User commands from the serial port
  //
  char userCommand;
  userCommand = Serial.read();
  if( userCommand == 'd' )
  {
    // Triggers full activity output for a time.
    activityOutputsEnabledTrigger = 100;
  }
  if( userCommand == 'm' )
  {
    TryModem();
  }

  // Slow the logging down for debugging
  delay(50);
}

// End of program.
