// File:      webpage_elements.h
// Created:   2023 Dec 14
// By:        Brian Mathewson
// For:       Arduino Server Side Events demonstration project
//
// Purpose:   Routines that generate HTML code.
//

#include "WiFiS3.h"


//
// This is sent to the client to initiate an event-stream data connection.
//
void SendEventStreamHeader( WiFiClient *targetClient )
{
  targetClient->println("HTTP/1.1 200 OK");
  targetClient->println("Content-Type: text/event-stream");
  targetClient->println("Cache-Control: no-cache");
  targetClient->println();
}


// Send the Server Side Event (SSE) data to the client.
//
// The idValue is customary but optional. 
// In this demo program the ID is the time since the program started.
//
// The data is a single line of text but multiple data: lines may be sent.
// You may customize this to send any data, including multiple lines,
// however do not send blank lines since this marks the end of the data.
//
void SendEventStreamData( WiFiClient *targetClient, int timeInTenthsOfASecond )
{
  // Outputs
  int sensorReading = analogRead(0);
  targetClient->print("data: Analog input 0 :");
  targetClient->print(sensorReading);
  targetClient->print(" at t=");
  targetClient->print(timeInTenthsOfASecond/10);
  targetClient->print(".");
  targetClient->print(timeInTenthsOfASecond % 10);

  // Two end-of-line characters mark the end of the data field.
  targetClient->print(".\n\n");

  Serial.print("Sending update t=");
  Serial.println( timeInTenthsOfASecond );
}


// =====================================================================
// Web Page Sent to Clients
// 

void SendHtmlHeader( WiFiClient *targetClient )
{
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  targetClient->println("HTTP/1.1 200 OK");
  targetClient->println("Content-type:text/html");
  targetClient->println();
}


//
// Customize this
//
void SendWebPageTop( WiFiClient *targetClient )
{
  targetClient->println("<!DOCTYPE HTML>");
  targetClient->println("<html>");
  // The following is supposed to suppress request of the favicon icon.
  targetClient->println("<head><link rel=\"icon\" href=\"data:,\"></head>");

  targetClient->println("<body>");

  targetClient->println("<p style=\"font-size:2vw;\">Arduino UNO SSE Test</p>");
}


// The web page could do anything with the SSE data it receives.
//   Here the data is presented between the characters "DIV[" and "]"
//   so you can see clearly the extent of the data.
//
// The division ID is named "result".
// This name is used in the web page script to insert data into this div.
//
void SendWebPageSSEArea( WiFiClient *targetClient )
{
  targetClient->println("DIV[<div id=\"result\"></div>]DIV" );
  targetClient->println();
}


// This script is part of the web page sent to the client.
// The client web browser runs the script which initiates a request 
//   for data from the Uno.
// The script also defines a function to be called whenever an event source
//   data message is received. This function sets the web page element 
//   called "result" with the contents of the received data. 
//
void SendWebPageScript( WiFiClient *targetClient )
{
  targetClient->println("<script>");
  targetClient->println("if(typeof(EventSource) !== \"undefined\") {");
  targetClient->println("  var source = new EventSource(\"getdata\");");
  targetClient->println("  source.onmessage = function(event) {");

  // This REPLACES the data in the DIV.
  targetClient->println("    document.getElementById(\"result\").innerHTML = event.data;");

  targetClient->println("  };");
  targetClient->println("} else {");
  targetClient->println("  document.getElementById(\"result\").innerHTML = \"Sorry, your browser does not support server-sent events...\";");
  targetClient->println("}");
  targetClient->println("</script>");
  targetClient->println();
}


void SendWebPageFooter( WiFiClient *targetClient )
{
  targetClient->println("<p style=\"font-size:2vw;\">More coming soon.</p>");

  targetClient->println("  </body>");
  targetClient->println("</html>");
}

// End of file.
