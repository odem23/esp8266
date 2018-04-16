#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

/*
 * 
 * 
 * HOWTO:
 * 1. Compile and start board 
 * 2. Connect wifi client to specified network
 * 3. Open browser on client
 * 4. Request Commands via URL
 * 
 * 
 * 
 *  Command structure: 
 *    http://192.168.4.1/api?comp=COMPONENT&cmd=CMD&args=ARGUMENTS
 *  
 *  OPTIONS:
 *    COMPONENT   : SERIAL or RC
 *    CMD (SERIAL): print
 *    CMD (RC)    : up,down,left,right
 *    ARGUMENTS   : custom arguments
 *    
 */

/* --- Config --------------------------------------------------------------------*/
/* Set these to your desired credentials. */
const char *ssid = "ESP8266";
const char *password = "foo";

/* --- Globals --------------------------------------------------------------------*/
ESP8266WebServer server(80);

/* --- Definitions ----------------------------------------------------------------*/
enum ComponentType{
  CT_NONE,
  CT_RC,
  CT_SERIAL
  
};

struct HTTP_COMMAND{
  ComponentType type;
  String comp = String();
  String cmd = String();
  String arguments = String();
  
  HTTP_COMMAND(){
    this->type = ComponentType::CT_NONE;
    this->comp = "";
    this->cmd = "";
    this->arguments = "";
  }
  
  HTTP_COMMAND(String comp, String cmd, String arguments){
    this->type = this->parseType(comp);
    this->comp = comp;
    this->cmd = cmd;
    this->arguments = arguments;
  }

  ComponentType parseType(String comp){
    ComponentType result = CT_NONE;
    if(comp.equals("") == false){
      if(comp.equals("RC") == true){
        result = CT_RC;
      } else if(comp.equals("SERIAL") == true){
        result = CT_SERIAL;
      }else{
        result = CT_NONE;
      }
    }
    return result;
  }
  
  bool isValid(){
    bool result = false;
    if(this->comp.equals("") == false && this->cmd.equals("") == false){
      if(this->type != CT_NONE){
        result = true;
      }
    }
    return result;
  }

  bool hasArguments(){
    bool result = false;
    if(this->arguments.equals("") == false){
      result = true;
    }
    return result;
  }
};

struct HTTP_RESULT{
  HTTP_COMMAND cmd;
  String content;
  HTTP_RESULT(){
    this->cmd = HTTP_COMMAND();
    this->content = "";
  }
  HTTP_RESULT(HTTP_COMMAND cmd){
    this->cmd = cmd;
    this->content = "";
  }
  HTTP_RESULT(HTTP_COMMAND cmd, String content){
    this->cmd = cmd;
    this->content = content;
  }
};

/* --- Send Html responses --------------------------------------------------------------------*/

String generateHtmlResponse(String content){
  String result = "<!DOCTYPE HTML><html><body>\r\n" + 
                  content +
                  "</body></html>";
  return result;
}

bool sendRawResult(int code, String contentType, String content){
  bool result = false;
  if(code >= 0 && code <=1024){
    if(contentType.equals("") == false){
      server.send(code, contentType, content);
      result = true;
    }
  }
  if(result == false){
    Serial.print("Error on sending response!");
  }
  return result;
}

bool sendError(int code, String content){
  String contentType = "text/html";
  return sendRawResult(code, "text/html", content);
}

bool sendHtml(String content){
   return sendRawResult(200, "text/html", content);
}

bool sendCommandResult(HTTP_RESULT result){
   String content = generateHtmlResponse(result.content);
   return sendHtml(content);
}

/* --- Parse request --------------------------------------------------------------------*/
HTTP_COMMAND getCommand(){
  HTTP_COMMAND result = HTTP_COMMAND();
  String comp=server.arg("comp");
  String cmd=server.arg("cmd");
  String arguments=server.arg("args");
  if(comp.equals("") == false && cmd.equals("") == false && arguments.equals("") == false ){
    result = HTTP_COMMAND(comp, cmd, arguments);
  }
  return result;
}

/* --- Command handlers --------------------------------------------------------------------*/
HTTP_RESULT doActionError(HTTP_COMMAND cmd, String error){
   String msg = "ERROR:" + error + " => Component: "+cmd.comp.c_str()+", Command: "+cmd.comp.c_str()+", Arguments: "+cmd.comp.c_str();
   Serial.print(msg.c_str() );
   return HTTP_RESULT(cmd, msg);
}

HTTP_RESULT doActionSerial(HTTP_COMMAND cmd){
   String result = "";

   // is valid component?
   if(cmd.type == ComponentType::CT_SERIAL){

    // is valid command?
     if(cmd.cmd.equals("print") == true){

       // has required arguments?
       if(cmd.hasArguments() ){
          Serial.printf("%s", cmd.arguments.c_str() );
          result = "SERIAL command executed!";
       }
       else{
        result = "SERIAL command without argument";
        doActionError(cmd, result);
       }
     }
     else{
       result = "Invalid SERIAL command";
       doActionError(cmd, result);
     }
   }
   else{
     result = "Invalid component command";
     doActionError(cmd, result);
   }
   return HTTP_RESULT(cmd, result);
}

HTTP_RESULT doActionRc(HTTP_COMMAND cmd){
   String result = "";

   // is valid component?
   if(cmd.type == ComponentType::CT_RC){

     // is valid command?
     if(cmd.cmd.equals("up") == true 
          || cmd.cmd.equals("down") == true
          || cmd.cmd.equals("left") == true
          || cmd.cmd.equals("right") == true){

          // has required arguments?
          if(cmd.hasArguments() ){
            result =  "RC Command: '"  + cmd.cmd + "'<br />RC Arguments: " + cmd.arguments;
          }
          else{
            result = "RC command without argument";
            doActionError(cmd, result);
          }
     }
     else{
      result = "Invalid RC command";
      doActionError(cmd, result);
     }
   }
   else{
     result = "Invalid component command";
     doActionError(cmd, result);
   }
   
   return HTTP_RESULT(cmd, result);
}

/* --- Execute command --------------------------------------------------------------------*/
HTTP_RESULT doCommand(HTTP_COMMAND cmd){
  HTTP_RESULT result = HTTP_RESULT();
  if(cmd.isValid() == true){
    switch(cmd.type){
      case CT_NONE:
        result = doActionError(cmd, "No component registered");
        break;
      case CT_RC:
        result = doActionRc(cmd);
        break;
      case CT_SERIAL:
        result = doActionSerial(cmd);
        break;
      default:
        result = doActionError(cmd, "Unknown error on doCommand");
    }
  }
  return result;
}

/* --- HTTP handlers --------------------------------------------------------------------*/
void handleNotFound(){
  sendError(404, "<h1>Page not Found!</h1>");
}

void handleRoot() {
  sendHtml("<h1>This is my ESP8266 HTTP-Server</h1>");
}

void handleApi() {
  HTTP_COMMAND cmd = getCommand();
  HTTP_RESULT result = doCommand(cmd);
  sendCommandResult(result);
}



void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...\n");
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/api", handleApi);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
