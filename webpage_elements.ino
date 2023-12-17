// File:      webpage_elements.h
// Created:   2023 Dec 14
// By:        Brian Mathewson
// For:       WiFi project
//
// Purpose:   Routines that generate HTML code.
//

#include "WiFiS3.h"


void SendEventStreamHeader( WiFiClient *targetClient )
{
  targetClient->println("HTTP/1.1 200 OK");
  targetClient->println("Content-Type: text/event-stream");
  targetClient->println("Cache-Control: no-cache");
  targetClient->println();
}


void SendEventStreamData( WiFiClient *targetClient, int idValue )
{
  // Outputs
  int sensorReading = analogRead(0);
  targetClient->print("data: Analog input 0 :");
  targetClient->print(sensorReading);
  targetClient->print(" id=");
  targetClient->print(idValue/10);
  targetClient->print(".");
  targetClient->print(idValue % 10);

  // Two end-of-line characters mark the end of the data field.
  targetClient->print(".\n\n");

  Serial.print("Sending update id=");
  Serial.println( idValue );
}


void SendHtmlHeader( WiFiClient *targetClient )
{
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  targetClient->println("HTTP/1.1 200 OK");
  targetClient->println("Content-type:text/html");
  targetClient->println();
}


void SendWebPageTop( WiFiClient *targetClient )
{
  targetClient->println("<!DOCTYPE HTML>");
  targetClient->println("<html>");
  targetClient->println("<body>");

  targetClient->println("<p style=\"font-size:2vw;\">Robot Alliance - SSE Test 2</p>");
}


void SendWebPageSSEArea( WiFiClient *targetClient )
{
  targetClient->println("DIV[<div id=\"result\"></div>]DIV" );
  targetClient->println();
}


void SendWebPageScript( WiFiClient *targetClient )
{
  targetClient->println("<script>");
  targetClient->println("if(typeof(EventSource) !== \"undefined\") {");
  targetClient->println("  var source = new EventSource(\"getdata\");");
  targetClient->println("  source.onmessage = function(event) {");

  // This APPENDS to the data in the DIV.
  //  targetClient->println("    document.getElementById(\"result\").innerHTML += event.data + \"<br>\";");

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