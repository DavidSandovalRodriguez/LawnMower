
  
#include <Adafruit_CC3000.h>
#include <SPI.h>
#include "utility/debug.h"
#include "utility/socket.h"
#include <avr/wdt.h>

/* connect SPI Uno:
CLK to Digital 13
MISO to Digital 12
MOSI to Digital 11
CS to Digital 10
   connect SPI Mega

CLK to Digital 52
MISO to Digital 50
MOSI to Digital 51
CS to Digital 10
   connect SPI Due, you'll need to connect to the hardware SPI pins.
   You want to connect to the SCK, MISO, and MOSI pins on the small 6
   pin male header next to the Due's SAM3X8E processor:
CLK to SPI SCK
MISO to SPI MISO
MOSI to SPI MOSI
CS to Digital 10
*/

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11

Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed

#define WLAN_SSID       "**********"   // cannot be longer than 32 characters!
#define WLAN_PASS       "*******"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define LISTEN_PORT           80      // What TCP port to listen on for connections.  
                                      // The HTTP protocol uses port 80 by default.

#define MAX_ACTION            10      // Maximum length of the HTTP action that can be parsed.

#define MAX_PATH              80      // Maximum length of the HTTP request path that can be parsed.
                                      // There isn't much memory available so keep this short!

#define BUFFER_SIZE           MAX_ACTION + MAX_PATH + 20  // Size of buffer for incoming request data.
                                                          // Since only the first line is parsed this
                                                          // needs to be as large as the maximum action
                                                          // and path plus a little for whitespace and
                                                          // HTTP version.

#define TIMEOUT_MS            500    // Amount of time in milliseconds to wait for
                                     // an incoming request to finish.  Don't set this
                                     // too high or your server could be slow to respond.

Adafruit_CC3000_Server httpServer(LISTEN_PORT);
uint8_t buffer[BUFFER_SIZE+1];
int bufindex = 0;
char action[MAX_ACTION+1];
char path[MAX_PATH+1];

//============MI CODIGO===========================================================================================================================================
String latlon;


struct area {                         //LOS JARDINES SE GUARDAN AQUÍ
  String puntos[15];
};


struct area jardines[2];     //GUARDAR MEMORIA PARA 2 JARDINES



double Latitude(String coordenadas){
  int split=coordenadas.indexOf("_");
  double lat=coordenadas.substring(0, split).toDouble();
  return lat;
}

double Longitude(String coordenadas){
  int split=coordenadas.indexOf("_");
  double lon=coordenadas.substring(split+1).toDouble();
  return lon;
}

struct area CrearJardin(){
  
  struct area jardinNuevo;
  Serial.println("introduzca coordenadas");
  
  int i=0; 
  while (i<3){
    if (Serial.available()>0){  
      
  String prueba = Serial.readString();
  
  jardinNuevo.puntos[i]=prueba;
  Serial.println(jardinNuevo.puntos[i]);

  Serial.println(Latitude(jardinNuevo.puntos[i]),6);
  Serial.println(Longitude(jardinNuevo.puntos[i]),6);

  i++;
  }
  } 
  return jardinNuevo;
}
//============MI CODIGO=========================================================================================================================================================

void setup(void)
{
  Serial.begin(9600);
  Serial.println(F("Hello, CC3000!\n")); 

  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  
  // Initialise the module
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  Serial.println(F("Request DHCP"));
#if !(defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__))
  Serial.println(F("Watchdog Timer Enabled"));
  wdt_enable(WDTO_8S);
#endif
 // Once your CC3000 has its profile for your AP I strongly suggest
 // you tell your router to set the same IP for this MAC each time
 // it connects and comment out the cc3000.checkDHCP() call. It won't
 // be needed at that point and it has a tendency to hang, so not
 // having to call it is faster and elminates a potenial lockup.
 // The Watchdog Timer call, wdt_enable(), will deal with the lockup,
 // but only works on the Uno (Atmega328 chip) and wastes a lot of
 // time by starting the initialization process again as well as
 // calling checkDHCP() again too.
 
  /*while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
   
  }*/
  
#if !(defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__))
  wdt_disable();
  Serial.println(F("Watchdog Timer Disabled"));
#endif

  // Display the IP address DNS, Gateway, etc.
  while (! displayConnectionDetails()) {
    delay(1000);
  }

  // ******************************************************
  // You can safely remove this to save some flash memory!
  // ******************************************************
  
  Serial.println(F("\r\nNOTE: This sketch may cause problems with other sketches"));
  Serial.println(F("since the .disconnect() function is never called, so the"));
  Serial.println(F("AP may refuse connection requests from the CC3000 until a"));
  Serial.println(F("timeout period passes.  This is normal behaviour since"));
  Serial.println(F("there isn't an obvious moment to disconnect with a server.\r\n"));
  
  // Start listening for connections
  httpServer.begin();
  
  Serial.println(F("Listening for connections..."));
}

// ==========================================bwk==============================================
// My modifications to parse incoming variables then set and get values to and from Arduino pins.
// These modifications are used for parsing the data of incoming GETS like:
// "?digital4=50&temp=&asensor16=" which writes a 5 to pin 4, displays the temperature and gets
// the value of pin 16.

/* parseGetRequest() is the main modification to the this sketch. It works like this:
 
   If there is a trailing digit as part of a variable's name like digital8 it will use that
   as the pin number, otherwise it uses the default pin number stored in the vars[].defaultPin
   array defined just below these comments.
   
   The asensor and dsensor defined variables are used to read a pin and don't use the values
   passed in with the GET, ie ?dsensor=SomeValue. The SomeValue is discarded and doesn't need
   to be included at all. However, you must include the = if there is another variable after
   that one. I.e, ?dsensor6=&temp=&digital8=1&desonsor17= is a perfectly acceptable GET query.
   
   The digital and analog defined variables are used to write to a pin. They use the value
   passed to them, so always include one. I.e, ?digital8=1 is acceptable, ?digital8= is not.
   
   The 'temp' defined variable is an example of any variables you would like to add and doesn't
   use the trailing name# to determine the pin. It only uses the pin set in vars.defaultPin.
   For the added variable to be useful you need to add code specifically for it in
   executeInstruction(). There is an example already there that uses 'temp'.
*/

#define DIGITAL     0   // Forces digitalRead/Write. Index into name, default pin in varlist.
#define ANALOG      1   // Forces analogRead/Write. Index into name, default pin in varlist.
#define ASENSOR     2   // Reads analog sensor. Index into name, default pin and return value.
#define DSENSOR     3   // Reads digital sensor. Index into name, default pin and return value.
#define TEMERATURE  4   // Increment numVars when adding vars! And add a new CONSTANT as well!

#define COORDS      5   //AÑADIDA POR MÍ PARA GUARDAR UNAS SUPUESTAS COORDENADAS RECIBIDAS EN UN GET

const int numVars = 6;  // Number of elements in the known variables struct varList, vars[numVars]

struct varList {        //  Predefined variables struct array.
  int len;
  char *var;
  int defaultPin;       // Default pin if not supplied as part of GET vars. Security feature of sorts.
  int rc;               // Return value from any sensors read.
};

struct varList vars[numVars] = // Predefined variables list.

// NameLen, Name, DefaultPin, ReturnValue (placeholder for it anyway)
  { {7, "digital", 8, -1},     // Any vars#=# passed as vars=# forces the defaultPin to be used.
    {6, "analog",  9, -1},     // Assumes write
    {7, "asensor", A0,-1},     // Assumes read
    {7, "dsensor", 1, -1},     // Assumes read, so does the following "temperature" element.
    {7, "temp", A3 ,-1},       // Assumes read. Use =-1 instead of =pin# for predefined sensors like this one.
    {6, "coorde", 8,-1}
  };
// NOTE: When adding new variables remember to increment numVars above first!
//       And #define another CONSTANT for it.
//       And the defaultPin cannot be overidden by the client like the first 4 vars.
// ==========================================bwk==============================================



//========================MI CÓDIGO===================


//====================================================

void loop(void)
{
  // Try to get a client which is connected.
  Adafruit_CC3000_ClientRef client = httpServer.available();
  if (client) {
    Serial.println(F("Client connected."));
    // Process this request until it completes or times out.
    // Note that this is explicitly limited to handling one request at a time!

    // Clear the incoming data buffer and point to the beginning of it.
    bufindex = 0;
    memset(&buffer, 0, sizeof(buffer));
    
    // Clear action and path strings.
    memset(&action, 0, sizeof(action));
    memset(&path,   0, sizeof(path));

    // Set a timeout for reading all the incoming data.
    unsigned long endtime = millis() + TIMEOUT_MS;
    
    // Read all the incoming data until it can be parsed or the timeout expires.
    bool parsed = false;
    while (!parsed && (millis() < endtime) && (bufindex < BUFFER_SIZE)) {
      if (client.available()) {
        buffer[bufindex++] = client.read();
      }
      parsed = parseRequest(buffer, bufindex, action, path);
    }

    // Handle the request if it was parsed.
    if (parsed) {
      Serial.println(F("Processing request"));
      Serial.print(F("Action: ")); Serial.println(action);
      Serial.print(F("Path: ")); Serial.println(path);
      // Check the action to see if it was a GET request.
      if (strcmp(action, "GET") == 0) {
        // Respond with the path that was accessed.
        // First send the success response code.
        client.fastrprintln(F("HTTP/1.1 200 OK"));
        // Then send a few headers to identify the type of data returned and that
        // the connection will not be held open.

        client.fastrprintln(F("Content-Type: text/html\r\n"));
        client.fastrprintln(F(""));
        //client.fastrprint(F("<!DOCTYPE HTML><html><body bgcolor = \"c8e6f7\"><center><h1>Wireless Controller via Wi-Fi</h1><p>by Nexys</p></center></body></html>"));
        
        client.fastrprint(F("<html><head><title>"));   
        client.fastrprint(F("GET request example"));
        client.fastrprintln(F("</title><body>"));
        
        client.fastrprint(F("<b>Request:</h3> </b>"));
        client.fastrprint(path);
        
        client.fastrprint(F("<p /><b>Sensor Values:</h3> </b>"));
        client.fastrprint((char *)buffer);

        client.fastrprintln(F("<br /><h2>Click buttons to turn pin 8 on or off</h2>"));
        
        client.fastrprint(F("<form action='/' method='GET'><p>"));
        //client.fastrprint(F("<input type='hidden' name='digital9' value='0'>")); //este 1 de aquí es el que enciende
        client.fastrprint(F("<input type='hidden' name='digital8' value='0'>"));
        //client.fastrprint(F("<input type='hidden' name='temp' value='50'>"));
        client.fastrprintln(F("<input type='submit' value='Off Pin8'/></form>"));
        
        client.fastrprint(F("<form action='/' method='GET'><p>"));
        client.fastrprint(F("<input type='hidden' name='digital9' value='0'>")); //este 1 de aquí es el que enciende
        //client.fastrprint(F("<input type='hidden' name='digital8' value='0'>"));
        //client.fastrprint(F("<input type='hidden' name='temp' value='50'>"));
        client.fastrprintln(F("<input type='submit' value='Off Pin9'/></form>"));

                
        client.fastrprint(F("<form action='/' method='GET'><p>"));
        //client.fastrprint(F("<<input type='hidden' name='digital9'>"));
        //client.fastrprint(F("<input type='hidden' name='digital9' value='1'>"));
        client.fastrprint(F("<input type='hidden' name='digital8' value='1'>"));
        //client.fastrprint(F("<input type='hidden' name='temp' value=''>"));
        client.fastrprint(F("<input type='submit' value='ON Pin8'/></form>"));

        client.fastrprint(F("<form action='/' method='GET'><p>"));
        //client.fastrprint(F("<<input type='hidden' name='digital9'>"));
        client.fastrprint(F("<input type='hidden' name='digital9' value='1'>"));
        //client.fastrprint(F("<input type='hidden' name='digital8' value='1'>"));
        //client.fastrprint(F("<input type='hidden' name='temp' value=''>"));
        client.fastrprint(F("<input type='submit' value='ON Pin9'/></form>"));
        

        client.fastrprint(F("<form action='/' method='GET'><p>"));
        client.fastrprint(F("<input type='hidden' name='temp' value=''>"));
        client.fastrprint(F("<input type='submit' value='TEXTITO'/></form>"));
                
        client.fastrprint(F("</body>"));
        client.fastrprint(F("</html>"));
        client.fastrprintln(F(""));
        client.fastrprintln(F(""));

      }
      else {
        // Unsupported action, respond with an HTTP 405 method not allowed error.
        client.fastrprintln(F("HTTP/1.1 405 Method Not Allowed"));
        client.fastrprintln(F(""));
      }
    }

    // Wait a short period to make sure the response had time to send before
    // the connection is closed (the CC3000 sends data asyncronously).
    delay(100);

    // Close the connection when done.
    Serial.println(F("Client disconnected"));
    client.close();
  }
}

// Return true if the buffer contains an HTTP request.  Also returns the request
// path and action strings if the request was parsed.  This does not attempt to
// parse any HTTP headers because there really isn't enough memory to process
// them all.
// HTTP request looks like:
//  [method] [path] [version] \r\n
//  Header_key_1: Header_value_1 \r\n
//  ...
//  Header_key_n: Header_value_n \r\n
//  \r\n

bool parseRequest(uint8_t* buf, int bufSize, char* action, char* path) {
  // Check if the request ends with \r\n to signal end of first line.
  char bwkBuf[MAX_PATH+1];
  char *p = 0;
  if (bufSize < 2)
    return false;
  if (buf[bufSize-2] == '\r' && buf[bufSize-1] == '\n') {
    parseFirstLine((char*)buf, action, path);
                                            // Bruce Knipe Addition 
    p =  strstr (path,"?");                 // Find our HTML variables.
    if( p > 0 and strlen(p+1) <= MAX_PATH) {// and if they're small enough for our buffer
     strcpy(bwkBuf, p+1);                   // remove ? prefix from string.
     strcpy(path, bwkBuf);
     memset(&buffer, 0, sizeof(buffer));
     parseGetRequest( bwkBuf );       // Parse and execute variables.
     
     
    }                          // And inform ignore to calling function.
    
    return true;
  }
  return false;
}

// Parse the action and path from the first line of an HTTP request.
void parseFirstLine(char* line, char* action, char* path) {
  // Parse first word up to whitespace as action.
  char* lineaction = strtok(line, " ");
  if (lineaction != NULL)
    strncpy(action, lineaction, MAX_ACTION);
  // Parse second word up to whitespace as path.
  char* linepath = strtok(NULL, " ");
  if (linepath != NULL)
    strncpy(path, linepath, MAX_PATH);
}

// ==========================================bwk==============================================

void parseGetRequest( char *p) {
  
  Serial.print(F("Parsing string:"));
  int varsFound = 0;
  int curVar = 0;
  int pin = -1;
  int value = -1;
  char *variable = 0;
  char *delmin = "=&?";            // List of the delmiters.
  int maxDelim = strlen(delmin);
  int len = strlen(p);
  char *name = p;               // Set 'name' to point to the first char of the vars.
  int d = 0;
  Serial.println(p);

  // Locate the delimiters ?, =. &.
  for(int i=0; i<len; i++) {      // Loop through variables string.
   for(d=0; d<maxDelim; d++) {    // Loop through delimitors structure
    if( *p == delmin[d] ) {       // If a match is found to a delimiter?
      if(d > 0) {                 // If either & or ? just skip over it and continue;
        p++;                      // Increment string variables pointer.
        name = p;               // Reset 'name/ to point to the first char of next var.
        d = maxDelim;             // Force break from delimiter loop.
        continue;                 // Continue looking through the variables string.
      }
     *p = 0;                      // If we get this far set *p overwritting the delimiter.
     break;     
    }
   }
   p++;                           // Increment string of variables pointer.
   if( d < maxDelim ) {           // If we have found a variable lets process it.
      value = atoi(p);           // Get variables string value and convert to int.
      latlon = String(p);              // MI CODIGO      Look for known variable name so we can get its defaults.  
      
      
      for(curVar=0;curVar<numVars;curVar++) { // vars[numVars] = { {3, "led"}, {5, "motor"}, {6, "sensor"} };
        // Set execution() call parameters.
        if( strncmp(name, vars[curVar].var, vars[curVar].len) == 0) {
          varsFound++;                  // Increment variables found counter.
          variable = vars[curVar].var;  // Set variable pointer to variables name.
          pin = vars[curVar].defaultPin;//Set default pin in case one isn't provided as part of var name.
          
          if(strlen(name) > vars[curVar].len) { // If variable has trailing digits..
             pin = atoi(name+vars[curVar].len);    // Get trailing digits from string variable's name and convert to int.
          }
          while(*p != delmin[1] && *p != NULL) // Find next & or EOL
              p++;
          if( *p != 0 )          // Likely an &, so
              *p++ = 0;          // erase it and skip over it.    
          // Debug display info.

          /*
          Serial.print(F("Variable: "));
          Serial.print(variable);
          Serial.print(F(" Pin: "));
          Serial.print(pin);
          Serial.print(F(" Value: "));
          Serial.println(value);
          */
          
          vars[curVar].rc = executeInstruction(pin, value, curVar, variable);
          name = p; // Set our string pointer to next potential var name;
          break;
        }
      }
    }
      continue;
  
  }
}
int executeInstruction(int pin, int value, int type, char *variable) {
 int rc = -1;

 if( type == TEMERATURE ) {
    // User predefined variable, temp.
    // When you add another variable to the vars[] array you must create
    // a constant like TEMPERATURE and increment numVars. Then add another
    // if(type==YOUR_CONSTANT) { your code } in this function.
    // Use the 'temp' variable as a guide. It is defined in the vars[] array
    // which is created just above the loop() function.
    
    Serial.println(F("Reading user defined variable 'temp'"));
    rc = 80; 
    // Put your code to read the temperature here..
    // The commented out digitalRead() call below is only there to give you
    // an example of the vars[] struct array being used to reference a user
    // defined variable's default pin#.
    // rc =  digitalRead(vars[TEMERATURE].defaultPin);
    // It could also be written as vars[type].defaultPin and the result
    // would be exactly the same, only not as clear to anyone reading the sketch.
  }

 if ( type == COORDS ) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin,HIGH);
  Serial.print("Coordenada recibida:");
  Serial.println(latlon);
  delay(100);
  digitalWrite(pin,LOW);
  
 } 
 
 if( type == DSENSOR || type == ASENSOR )
    pinMode(pin, INPUT); 
 else
    pinMode(pin, OUTPUT); 
 switch (type ) {
  case ANALOG:  //Serial.println(F("AnaloglWrite()")); 
                analogWrite(pin, value); break;
  case DIGITAL: //Serial.println(F("digitalWrite()"));
                digitalWrite(pin,value); break;
  case ASENSOR: //Serial.println(F("Reading analogRead()"));
                rc = analogRead(pin); break;
  case DSENSOR: //Serial.println(F("Reading digitalRead()"));
                rc = digitalRead(pin); break;
  
 }
 
 vars[type].rc = rc; // Store the return value in our vars[] array.
 
 if( rc >= 0) {
  if(strlen((char *)buffer) > 0)
     strcat((char *)buffer," -- ");
  sprintf((char *)buffer + strlen((char*)buffer), "%s = %i", variable, rc);
}
return rc;
 
}
// ==========================================bwk==============================================

// Tries to read the IP address and other connection details
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}
