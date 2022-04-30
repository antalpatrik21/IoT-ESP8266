// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h> 
#include <EEPROM.h>

// for timing the light

#include <TimeLib.h>
#include <WiFiUdp.h>

// integers are 4 bytes, so 32 bits, we calculate memory size for 25 integers 
#define EEPROM_SIZE 512

// Replace with your network credentials
const char* ssid     = "Telekom-fc19f0";
const char* password = "Q5ZKNC7EHTWH";

// Set web server port number to 80
WiFiServer server(80);

// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";

const int timeZone = 2;     // Central European Time

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

// Decode HTTP GET value
String redString = "0";
String greenString = "0";
String blueString = "0";
String whiteString = "0";
String mixString = "0";
String timerString = "0";
String durationString = "0";

// create an array for holding the addresses in memory
// each int is counting as 4 bytes 

const int num_addresses = 10;
int addresses[num_addresses];



int pos1 = 0;
int pos2 = 0;
int pos3 = 0;
int pos4 = 0;

// Variable to store the HTTP req  uest
String header;

// Red, green, and blue pins for PWM control
const int redPin = 13;     // 13 corresponds to GPIO13 -- D7
const int greenPin = 12;   // 12 corresponds to GPIO12 -- D6
const int bluePin = 14;    // 14 corresponds to GPIO14 -- D5
const int whitePin = 4;  // D2
const int mixPin =  5; // D1
const int erasePin = 15; //D8

// default settings of the light values

int red;
int blue;
int green;
int mix;
int white;

// integers representing the ON/OFF stages of the system
// 0 means OFF 1 means ON  


int read_save;
int timer;  // also holds the hour of starting the timer 
int duration; //also holds the number for the hours duration 
int dur; // has the field been written on EEPROM before
int ti;  //has the field been written on EEPROM before

// Setting PWM bit resolution
const int resolution = 256;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

unsigned long currentTime2 = millis();
unsigned long previousTime2 = 0;
const long timeoutTime2 = 5000;

unsigned long anHour = 3600;

/*
IPAddress staticIP(192, 168, 1, 40);
IPAddress gateway(188, 156, 224, 1);
IPAddress subnet(255, 255, 248, 0);
IPAddress dns(84, 2, 46, 1);
*/
const char* deviceName = "aquarium";

bool lightSet = false;
bool timerFirst = true;
bool trigger = true;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//SWITCHING THE LED STRIP 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void switchON(){
  

  delay(1000);
  
  analogWriteRange(resolution);
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
  analogWrite(whitePin, white);
  analogWrite(mixPin, mix);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//SWITCHING THE LED STRIP 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void switchOFF(){
    
  analogWrite(redPin, 0);
  analogWrite(greenPin, 0);
  analogWrite(bluePin, 0);
  analogWrite(whitePin, 0);
  analogWrite(mixPin, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//ERASE EEPROM AND RESTART THE ESP8266 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void eraseReset(){
    // write a -1 to all 512 bytes of the EEPROM
  for (int i = 0; i < 512; i++) {
    EEPROM.put(i, -1);
  }
  EEPROM.commit();
  EEPROM.end();
  Serial.println("Memory has been erased.");
  Serial.println("Now restarting the board.");
  ESP.restart();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//ERROR FUNCTION
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void errorFunction1(){
  if(WiFi.status() != WL_CONNECTED){
    analogWrite(redPin, 255);
    analogWrite(mix, 0);
    analogWrite(white, 0);
    analogWrite(bluePin, 0);
    analogWrite(greenPin, 0);
  }
  }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CONNECTED TO WIFI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void connectedWiFi(){
    analogWrite(redPin, 0);
    analogWrite(mix, 0);
    analogWrite(white, 0);
    analogWrite(bluePin, 0);
    analogWrite(greenPin, 255);  
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TRACKING THE TIME 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DATA FROMATING FOR DISPLAY
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//MAKING AND MAINTAINING THE NTP CONNECTION
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//SENDING NTP  request to a time server on a specified IP address
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void setup() {

  pinMode(erasePin, INPUT);


  
  int k = 0;
  int i = 0;
  for(i = 0; i<(num_addresses*4); i = i + 4){
      addresses[k] = i;
      k++;
    }

  Serial.begin(115200);
  delay(10);
  
  EEPROM.begin(EEPROM_SIZE);

  EEPROM.get(addresses[0], read_save);
  EEPROM.get(addresses[6], timer);
  EEPROM.get(addresses[7], duration);
  EEPROM.get(addresses[8], ti);
  EEPROM.get(addresses[9], dur);
  if(ti != 1 ){
    EEPROM.put(addresses[8], 0);
    EEPROM.get(addresses[8], ti);
  }
  if(dur != 1){
    EEPROM.put(addresses[9], 0);
    EEPROM.get(addresses[9], dur);
  }

  EEPROM.commit();
  
  Serial.println("The initial value");
  Serial.println(read_save);

  Serial.print("The ti variable:");
  Serial.println(ti);

  Serial.print("The dur variable:");
  Serial.println(dur);
  
  Serial.print("The timer is set to: ");
  Serial.println(timer);
  Serial.print("Duration: ");
  Serial.println(duration);

  WiFi.begin(ssid, password);

  WiFi.disconnect();

  WiFi.hostname(deviceName);

  WiFi.begin(ssid, password);

  
  // configure LED PWM resolution/range and set pins to LOW
  analogWriteRange(resolution);
  analogWrite(redPin, 0);
  analogWrite(greenPin, 0);
  analogWrite(bluePin, 0);
  analogWrite(whitePin, 0);
  analogWrite(mixPin, 0);

   Serial.println();
   Serial.print("MAC: ");
   Serial.println(WiFi.macAddress());
  
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
/*  Serial.println(ssid);
  WiFi.begin(ssid, password); */
  while (WiFi.status() != WL_CONNECTED) {
    errorFunction1();
    delay(500);
    Serial.print(".");
  }

  connectedWiFi();
  Serial.println("Connected to:");
  Serial.println(ssid);
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Subnet Mask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway IP: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS 1: ");
  Serial.println(WiFi.dnsIP(0));
  Serial.print("DNS 2: ");
  Serial.println(WiFi.dnsIP(1));
  Serial.println(sizeof(1));
  for(int b=0; b <num_addresses; b++){
      Serial.println(addresses[b]);
  }
  bool success = Ping.ping("www.google.com", 3);
  
  if(!success){
    Serial.println("\nPing failed");
    return;
  }
  
  Serial.println("\nPing successful.");



  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  
  server.begin();
}

time_t prevDisplay = 0; // when the digital clock was displayed
time_t timerStart = 0;
time_t timerStop = 0;


void loop(){

if(digitalRead(erasePin) == HIGH){
  Serial.println("READ HIGH");
  eraseReset();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //refreshing the time 
    if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();
    }
  }


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  currentTime2 = millis();
  if(currentTime2 - previousTime2 >= timeoutTime2){
    previousTime2 = currentTime2;
    EEPROM.get(addresses[0], read_save);
    if(read_save != 1){
    read_save = 0;
    }
     Serial.println("READ");
     Serial.println(digitalRead(erasePin));
     Serial.print("Read saved eeprom variable");
     Serial.println(read_save);
     Serial.print("The timer is set to: ");
     Serial.println(timer);
     Serial.print("Duration: ");
     Serial.println(duration);


    EEPROM.get(addresses[1], red);
    EEPROM.get(addresses[2], green);
    EEPROM.get(addresses[3], blue);
    EEPROM.get(addresses[4], white);
    EEPROM.get(addresses[5], mix);

    Serial.print("The color values:");
    Serial.println(red);
    Serial.println(green);
    Serial.println(blue);
    Serial.println(white);
    Serial.println(mix);
  }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  

  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {            // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
                   
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css\">");
            client.println("<script src=\"https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js\"></script>");
            client.println("</head><body><div class=\"container\"><div class=\"row\"><h1>ESP Color Picker</h1></div>");
            client.println("<a class=\"btn btn-primary btn-lg\" href=\"#\" id=\"change_color\" role=\"button\">Change Color</a> ");
            client.println("<input class=\"jscolor {onFineChange:'update(this)'}\" id=\"rgb\"></div>");
            client.println("<script>function update(picker) {document.getElementById('rgb').innerHTML = Math.round(picker.rgb[0]) + ', ' +  Math.round(picker.rgb[1]) + ', ' + Math.round(picker.rgb[2]);");
            client.println("document.getElementById(\"change_color\").href=\"?r\" + Math.round(picker.rgb[0]) + \"g\" +  Math.round(picker.rgb[1]) + \"b\" + Math.round(picker.rgb[2]) + \"&\";}</script></body></html>");
            // The HTTP response ends with another blank line
            client.println();

            // Request sample: /?r201g32b255&
            // Red = 201 | Green = 32 | Blue = 255
            if(header.indexOf("GET /?r") >= 0) {
              pos1 = header.indexOf('r');
              pos2 = header.indexOf('g');
              pos3 = header.indexOf('b');
              pos4 = header.indexOf('&');
              redString = header.substring(pos1+1, pos2);
              greenString = header.substring(pos2+1, pos3);
              blueString = header.substring(pos3+1, pos4);
              /*Serial.println(redString.toInt());
              Serial.println(greenString.toInt());
              Serial.println(blueString.toInt());*/
              red  = redString.toInt();
              green = greenString.toInt();
              blue = blueString.toInt();
              EEPROM.put(addresses[0], 1);
              EEPROM.put(addresses[1], red);
              EEPROM.put(addresses[2], green);
              EEPROM.put(addresses[3], blue);
              
            }
            else if(header.indexOf("GET /?w") >= 0) {
              pos1 = header.indexOf('w');
              pos2 = header.indexOf('&');
              whiteString = header.substring(pos1 +1, pos2);
              white = whiteString.toInt();
              EEPROM.put(addresses[4], white);
              
            }
            else if(header.indexOf("GET /?m") >= 0)  {
              pos1 = header.indexOf('m');
              pos2 = header.indexOf('&');
              mixString = header.substring(pos1 +1, pos2);
              mix = mixString.toInt();
              EEPROM.put(addresses[5], mix);
              
            }

            else if(header.indexOf("GET /?settimehour") >= 0)  {
              pos1 = header.indexOf('r');
              pos2 = header.indexOf('&');
              timerString = header.substring(pos1 + 1, pos2);
              timer = timerString.toInt();
              switchOFF();
              if( 1 <= timer <= 23 ){
                EEPROM.put(addresses[6], timer); 
                ti = 1;
                EEPROM.put(addresses[8], ti); 
              }else{
                Serial.println("Timer Error, out of bounds ");
              }
            }


            else if(header.indexOf("GET /?deltime") >= 0)  {
              timer = 0 ;
              switchOFF();
              EEPROM.put(addresses[6], timer); 
              ti = 1;
              EEPROM.put(addresses[8], ti); 
            }
           
            else if(header.indexOf("GET /?setdur") >= 0)  {
              switchOFF();
              pos1 = header.indexOf('r');
              pos2 = header.indexOf('&');
              durationString = header.substring(pos1 + 1, pos2);
              duration = durationString.toInt();
              if( 1 <= timer <= 23 ){
                EEPROM.put(addresses[7], duration); 
                dur = 1;
                EEPROM.put(addresses[9], dur);
              
              }else{
                Serial.println("Duration Error, out of bounds ");
              }
            }


            else if(header.indexOf("GET /?deltime") >= 0)  {
              duration = 0 ;
              EEPROM.put(addresses[7], duration); 
              switchOFF();
              dur = 1;
              EEPROM.put(addresses[9], dur);
            }
            
            else if(header.indexOf("GET /?available"))
            // Break out of the while loop
            EEPROM.commit();
             
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
        EEPROM.commit();
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CONTROL
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

if(read_save){
  //mode 3 -- when there is no timing, turn the lights  on until there is power
  if ((dur == 0)&&(ti == 0)){
    switchON();
    //Serial.println("Hey I have been saved ");
  }
  //mode 1 -- from the time we push reset or power off switch the led on until the period ends 
  
  else if(dur == 1 && ti == 0){
    if(timerFirst){
      timerStart = now();
      timerStop = timerStart + (duration * anHour);
      switchON();
      timerFirst = false;
    }
    
    if(now() >= timerStop){
      switchOFF();
    }
    
  }

// MODE 4 after the given time switch on, than turn down 

  else if(dur == 0 && ti == 1){
    if(hour(now()) >=  timer){
      switchON();
    }
    else{
      switchOFF();
    }
  }

 else if(dur == 1 && ti == 1){
  if(hour(now()) == timer && trigger){
    trigger = false;
    switchON();
    timerStart = now();
    timerStop = timerStart + (anHour * duration); 
  }
  if(!trigger && now() > timerStop){
    trigger == true;
    switchOFF();
    
  }
 }


  
}
  

  
}
