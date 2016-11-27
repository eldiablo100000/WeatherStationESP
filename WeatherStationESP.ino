#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <Arduino.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <EEPROMforAuth.h>
#define NFIELDS 4 //Fields in the form of the standalone WebService
#define NMINSLEEP 20
#define TIMESLEEP 1000000 * 5 // in useconds
//#define TIMESLEEP 1000000 * 60 * NMINSLEEP
#define SERVER "http://184.106.153.149/update"
  ADC_MODE(ADC_VCC);
//DHT22
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#define DHTPIN         12         // Pin which is connected to the DHT sensor.
// Uncomment the type of sensor in use:
//#define DHTTYPE           DHT11     // DHT 11 
#define DHTTYPE           DHT22     // DHT 22 (AM2302)
//#define DHTTYPE           DHT21     // DHT 21 (AM2301)
DHT_Unified dht(DHTPIN, DHTTYPE);

//BMP280
#include <Wire.h>
#include <Adafruit_BMP085.h>
Adafruit_BMP085 bmp;


const char * ssid = "WeatherStationESP";
const char * password = "helloiot";//min. 8 characters
const int buttonPin = 2;

const int ledPinRed = 5;
const int ledPinYellow = 13;
int32_t rssi;
char networks[500]="";
int buttonState = 0; // current state of the button
int lastButtonState = 0; // previous state of the button
int startPressed = 0; // the time button was pressed
int endPressed = 0; // the time button was released
int timeHold = 0; // the time button is hold
int counter = 0; // counter loop
bool closeAP = 0;
EEPROMforAuth eep;
//ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
uint32_t getVcc;


//  ███████╗███████╗████████╗██╗   ██╗██████╗        ██╗       ██╗      ██████╗  ██████╗ ██████╗ 
//  ██╔════╝██╔════╝╚══██╔══╝██║   ██║██╔══██╗       ██║       ██║     ██╔═══██╗██╔═══██╗██╔══██╗
//  ███████╗█████╗     ██║   ██║   ██║██████╔╝    ████████╗    ██║     ██║   ██║██║   ██║██████╔╝
//  ╚════██║██╔══╝     ██║   ██║   ██║██╔═══╝     ██╔═██╔═╝    ██║     ██║   ██║██║   ██║██╔═══╝ 
//  ███████║███████╗   ██║   ╚██████╔╝██║         ██████║      ███████╗╚██████╔╝╚██████╔╝██║     
//  ╚══════╝╚══════╝   ╚═╝    ╚═════╝ ╚═╝         ╚═════╝      ╚══════╝ ╚═════╝  ╚═════╝ ╚═╝     
//  

/*long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}*/
void setup(void) {
  pinMode(buttonPin, INPUT);
  pinMode(ledPinRed, OUTPUT);
  pinMode(ledPinYellow, OUTPUT);
  digitalWrite(ledPinRed, LOW);
  digitalWrite(ledPinYellow, LOW);
  Serial.begin(115200);
  String res;


    getVcc = ESP.getVcc();
  Serial.println("Battery Level:");
  Serial.println(getVcc);
  //long int vcc=readVcc();
  //Serial.println(vcc);

  //strcpy(networks,scanNetworks());
  //res=connectStandAlone(ssid,password);
  eep= EEPROMforAuth();
  //SDA 3, SCL 14
  Wire.begin(0,14);
  String psw = eep.readFromEEPROM("PSW");
  String essid = eep.readFromEEPROM("ESSID");
  String key = eep.readFromEEPROM("KEY");
  Serial.println();
  Serial.println("[SETUP] essid : " + essid);
  Serial.println("[SETUP] key : " + key);
  Serial.println("[SETUP] psw : " + psw);

  delay(1000);

  connectToNetwork(essid.c_str(), psw.c_str(), key.c_str());
}
void loop(void) {

  digitalWrite(ledPinRed, HIGH);
  Serial.println("[LOOP] Press the button and wait until led turn off to connect standalone");
  delay(3000);
  bool setup = false;
  int now;
  buttonState = digitalRead(buttonPin);
  //Setup possible only at launch of loop 
  while (buttonState == LOW) {
  counter++;

  if (counter == 1) startPressed = millis();
  now = millis();

  timeHold = now - startPressed;
  Serial.printf("[LOOP] TimeHold down %i \n", timeHold);
    if (timeHold >= 2000) {
    setup = true;
    digitalWrite(ledPinRed,LOW);
    digitalWrite(ledPinYellow,HIGH);
    
    }
  delay(100);
  buttonState = digitalRead(buttonPin);

  }
  if (buttonState == HIGH && setup) {
  endPressed = millis();

  timeHold = endPressed - startPressed;
  //  if(counter==1) timeHold=0;
  Serial.printf("[LOOP] TimeHold high %i \n", timeHold);

  strcpy(networks,scanNetworks());
  blinkLed(ledPinYellow,3);
  Serial.println("[LOOP] Scatta la routine di registrazione in apmode");
  connectStandAlone(ssid, password);
  Serial.println("[LOOP] Restarting");

  ESP.reset();
  delay(1000);

  

  }else {
  //startPressed = millis();
  
  blinkLed(ledPinRed,3);

  }

   float bmpSens[2];
  if(initializeBMP280()){
    bmpSens[0]=readBMP280(1);
    bmpSens[1]=readBMP280(2);

  }
 

  initializeDHT22();
  float temperature=readDHT22(1);
  float humidity=readDHT22(2);

  float dhtSens[2] = {
  float(temperature),
  float(humidity)
  };


  String key = eep.readFromEEPROM("KEY");
  int result = postToServer(key,dhtSens[0], dhtSens[1], bmpSens[0], bmpSens[1]);
  if (result == 1) {
  Serial.println("Deepsleep!...");
  delay(1000);
  ESP.deepSleep(TIMESLEEP);
  }
}

//  █████╗ ██╗  ██╗██╗  ██╗
//  ██╔══██╗██║ ██║╚██╗██╔╝
//  ███████║██║ ██║ ╚███╔╝ 
//  ██╔══██║██║ ██║ ██╔██╗ 
//  ██║  ██║╚██████╔╝██╔╝ ██╗
//  ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝
//  
int32_t getRSSI(const char* target_ssid) {
  WiFi.mode(WIFI_STA);
 WiFi.disconnect();
  byte available_networks = WiFi.scanNetworks();

  for (int network = 0; network < available_networks; network++) {
    if (strcmp(WiFi.SSID(network).c_str(), target_ssid) == 0) {
    
      return WiFi.RSSI(network);
    }
  }
  return 0;
}
char* scanNetworks(){
  char networks[500];
  digitalWrite(ledPinRed, HIGH);
  digitalWrite(ledPinYellow, HIGH);
  Serial.println("[SCAN] Scan start");

  // WiFi.scanNetworks will return the number of networks found
  WiFi.mode(WIFI_STA);
 WiFi.disconnect();
  int n = WiFi.scanNetworks();
  Serial.println("[SCAN] Scan done");
  if (n == 0)
    Serial.println("[SCAN] No networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    strcpy(networks,"<select name=essid>");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
  strcat(networks,"<option value=\"");

  strcat(networks,WiFi.SSID(i).c_str());
  strcat(networks,"\">");
  strcat(networks,WiFi.SSID(i).c_str());
  strcat(networks,"</option>");

      delay(10);
    }
    strcat(networks,"</select>");
  }
  Serial.println("");
  return networks;
}
void blinkLed(int pin,int repeat){
  if(repeat>0){
  for(int i=0;i<repeat;i++){
  digitalWrite(pin,LOW);
  delay(500);
  digitalWrite(pin, HIGH);
  delay(500);
  digitalWrite(pin, LOW);
  delay(500);

  }
  }
}
void blink2Led(int pin1,int pin2,int repeat){
  if(repeat>0){
  for(int i=0;i<repeat;i++){
  digitalWrite(pin1,LOW);
  digitalWrite(pin2,LOW);
  delay(500);
  digitalWrite(pin1, HIGH);
  digitalWrite(pin2,HIGH);
  delay(500);
  digitalWrite(pin1, LOW);
  digitalWrite(pin2,LOW);
  delay(500);

  }
  }
}

//   ██████╗ ██████╗ ███╗   ██╗███╗   ██╗███████╗ ██████╗████████╗██╗ ██████╗ ███╗   ██╗███████╗
//  ██╔════╝██╔═══██╗████╗  ██║████╗  ██║██╔════╝██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║██╔════╝
//  ██║     ██║   ██║██╔██╗ ██║██╔██╗ ██║█████╗  ██║        ██║   ██║██║   ██║██╔██╗ ██║███████╗
//  ██║     ██║   ██║██║╚██╗██║██║╚██╗██║██╔══╝  ██║        ██║   ██║██║   ██║██║╚██╗██║╚════██║
//  ╚██████╗╚██████╔╝██║ ╚████║██║ ╚████║███████╗╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║███████║
//   ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═══╝╚══════╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝
//  

String connectStandAlone(const char * essid,const char * pwd) {
    closeAP=0;

  WiFi.softAP(essid, pwd);
  Serial.println();
  
  // Wait for connection
  
  Serial.println();
  Serial.print("[CONN. STANDALONE] Connected to ");
  Serial.println(essid);
  Serial.print("[CONN. STANDALONE] IP address: ");
  Serial.println(WiFi.softAPIP());
  
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  
  server.on("/", handleRoot);
  server.on("/RegisterWStation", handlePost);
  server.on("/IoT", handleIoT);
  server.on("/About", handleAbout);
  server.on("/Contact", handleContact);
  server.on("/Tools", handleTools);
  server.on("/Exit", handleExit);
  server.on("/Reset", handleReset);
  
  server.onNotFound(handleNotFound);
  server.begin();
  
  Serial.println("[CONN. STANDALONE] HTTP server started");
  while (true&&!closeAP) {
    server.handleClient();
  
  }
  Serial.println("[CONN. STANDALONE] Server closed!Exit !...");
  
  char buf[250];
   char network[68];
  if(server.arg(2).length()>0){
    strcpy(network,server.arg(2).c_str());

  }
  else{
    strcpy(network,server.arg(1).c_str());

  }
  sprintf(buf, "Fullname:%s, ESSID:%s, Password:%s, Write Key:%s", server.arg(0).c_str(), network, server.arg(3).c_str(), server.arg(4).c_str());
  WiFi.disconnect();
  
  return buf; 
}
void connectToNetwork(const char essid[],const char password[],const char string[]) {

  Serial.println();
  Serial.println();
  Serial.println();
  Serial.printf("[CONN. NETWORK] Connecting to %s with pass %s\n", essid, password);
  rssi = getRSSI(essid);

    WiFi.begin(essid,password);
 // WiFiMulti.addAP(ssid, password);
}
int postToServer(String api_key,float f1, float f2, float f3,float f4) {
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
   
  HTTPClient http;
  Serial.print("[POST2SERVER] begin...\n");
  // configure traged server and url
  //http.begin("https://192.168.1.12/test.html", "7a 9c f4 db 40 d3 62 5a 6e 21 bc 5c cc 66 c8 3e a1 45 59 38"); //HTTPS
  http.begin(SERVER); //HTTP

  Serial.print("[POST2SERVER] POST...\n");
  // start connection and send HTTP header
  getVcc = ESP.getVcc();
  Serial.println("[POST2SERVER] Battery Level:");
  Serial.println(getVcc);
  int httpCode = http.POST("api_key="+api_key+"&field1=" + f1 + "&field2=" + f2 + "&field3=" + f3+"&field4=" + f4+"&field5="+rssi+"&field6="+getVcc);

  // httpCode will be negative on error
  if (httpCode > 0) {
  // HTTP header has been send and Server response header has been handledPinRed
  Serial.printf("[POST2SERVER] POST... code: %d\n", httpCode);

  // file found at server
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println(payload);
    return 1;
  }
  } else {
  Serial.printf("[POST2SERVER] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());

  }
  delay(5000);
  http.end();
  return 0;
  } else {
  Serial.println("Not connected");
  }
}


//  ██╗    ██╗███████╗██████╗     ███████╗███████╗██████╗ ██╗   ██╗██╗ ██████╗███████╗
//  ██║    ██║██╔════╝██╔══██╗    ██╔════╝██╔════╝██╔══██╗██║   ██║██║██╔════╝██╔════╝
//  ██║ █╗ ██║█████╗  ██████╔╝    ███████╗█████╗  ██████╔╝██║   ██║██║██║     █████╗  
//  ██║███╗██║██╔══╝  ██╔══██╗    ╚════██║██╔══╝  ██╔══██╗╚██╗ ██╔╝██║██║     ██╔══╝  
//  ╚███╔███╔╝███████╗██████╔╝    ███████║███████╗██║  ██║ ╚████╔╝ ██║╚██████╗███████╗
//   ╚══╝╚══╝ ╚══════╝╚═════╝     ╚══════╝╚══════╝╚═╝  ╚═╝  ╚═══╝  ╚═╝ ╚═════╝╚══════╝
//                                                                                    

//CSS DECLARATION
  char css[540]="body {\
    background-color: LightGreen;\
  }\
  h1 {\
    color: DarkGreen;\
  }\
   h2 {\
    color: ForestGreen;\
  }\
  ul.bar {\
    list-style-type: none;\
    margin: 0;\
    padding: 0;\
    overflow: hidden;\
    background-color: #333;\
  }\
  li.bar {\
    float: left;\
  }\
  li.bar a {\
    display: block;\
    color: white;\
    text-align: center;\
    padding: 14px 16px;\
    text-decoration: none;\
  }\
  li.bar a:hover:not(.active) {\
    background-color: #111;\
  }\
  .active {\
    background-color: #4CAF50;\
  }";
void handlePost() {
  digitalWrite(ledPinYellow,HIGH);
  int lenHTML = 2500;
  char network[68];
  if(server.arg(2).length()>0){
    strcpy(network,server.arg(2).c_str());

  }
  else{
    strcpy(network,server.arg(1).c_str());

  }
  char temp[lenHTML];
  snprintf(temp, lenHTML,"<html><head>\
  <style>%s\
  .btn {\
    background: #75f734;\
    background-image: linear-gradient(to bottom, #75f734, #3d8c23);\
    -webkit-border-radius: 30;\
    -moz-border-radius: 30;\
    border-radius: 30px;\
    font-family: Courier New;\
    color: #040504;\
    font-size: 20px;\
    padding: 10px 20px 10px 20px;\
    text-decoration: none;\
    }\
    .btn:hover {\
    background: #3cb0fd;\
    background-image: linear-gradient(to bottom, #3cb0fd, #3498db);\
    text-decoration: none;\
    }\
    </style>\
   </head>\
   <body>\
   <ul class=\"bar\">\
  <li class=\"bar\"><a class=\"active\" href=\"/\">Home</a></li>\
  <li class=\"bar\"><a href=\"/IoT\" >IoT</a></li>\
  <li class=\"bar\"><a href=\"/About\">About</a></li>\
  <li class=\"bar\"><a href=\"/Contact\">Contact</a></li>\
  <li class=\"bar\"><a href=\"/Tools\">Tools</a></li>\
  </ul>\
   <h1>Crowdsending Data</h1>\
  <h2>Registrationg Confirmed</h2>\
  <p>Congratulations! Now you can start to crowdsensing<br/>\
  <br/>Close the server or click the reset button to start! \
  <form action=/Exit>\
  <button class=\"btn\"  type=\"submit\" >Close server!</button>\
  </form>\
  <br/>\
  Your name: %s<br/>Your ESSID: %s<br/>Your password: %s<br/>Your Write Key: %s</p>\
  </body></html>",css,server.arg(0).c_str(),network, server.arg(3).c_str(), server.arg(4).c_str());

  //Serial.println("----->"+out);
  server.send(200, "text/html", temp);
  delay(1000);
  digitalWrite(ledPinYellow,LOW);
  eep.writeToEEPROM(network, server.arg(3).c_str(), server.arg(4).c_str());
strcpy(temp,"");
}
void handleReset() {
  blinkLed(ledPinRed,3);
  String temp;
  temp="<html><head>\
  <style>";
  temp+=css;
  temp+="</style>\
   </head>\
   <body>\
  <ul class=\"bar\">\
  <li class=\"bar\"><a href=\"/\">Home</a></li>\
  <li class=\"bar\"><a href=\"/IoT\" >IoT</a></li>\
  <li class=\"bar\"><a href=\"/About\">About</a></li>\
  <li class=\"bar\"><a href=\"/Contact\">Contact</a></li>\
  <li class=\"bar\"><a class=\"active\" href=\"/Tools\">Tools</a></li>\
  </ul>\
   <h1>Crowdsending Data</h1>\
  <h2>Resetting</h2>\
  <p>Your Weather Station was cleared from every old data.\
  <br/>Click Home to register from zero.</p>\
  </body></html>";

  Serial.println("----->Reset");
  server.send(200, "text/html", temp);
  eep.clearEEPROM();
}
void handleExit() {
  blinkLed(ledPinRed,3);
  String temp;
  temp="<html><head>\
  <style>";
  temp+=css;
  temp+="</style>\
   </head>\
   <body>\
  <ul class=\"bar\">\
  <li class=\"bar\"><a href=\"/\">Home</a></li>\
  <li class=\"bar\"><a href=\"/IoT\" >IoT</a></li>\
  <li class=\"bar\"><a href=\"/About\">About</a></li>\
  <li class=\"bar\"><a href=\"/Contact\">Contact</a></li>\
  <li class=\"bar\"><a class=\"active\" href=\"/Tools\">Tools</a></li>\
  </ul>\
   <h1>Crowdsending Data</h1>\
  <h2>Stopping server</h2>\
  <p>Your AccessPoint was closed: <br/> you can start crowdsensing\
  (if you registred your Weather Station) <br/> you must reset the Weather Station otherwise.</p>\
  </body></html>";
  Serial.println("----->Exit");
  server.send(200, "text/html", temp);
  WiFi.disconnect();
  server.close();
  closeAP=1;
}
void handleNotFound() {
  digitalWrite(ledPinRed, HIGH);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
  message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(ledPinRed, LOW);
}
void handleIoT() {
  digitalWrite(ledPinYellow, HIGH);
  int lenHTML = 2900;
  String temp;
  temp="<html>\
  <head>\
  <style>";
  temp+=css;
  temp+="</style>\
   </head>\
   <body>  \
   <ul class=\"bar\">\
  <li class=\"bar\"><a href=\"/\">Home</a></li>\
  <li class=\"bar\"><a class=\"active\" href=\"/IoT\" >IoT</a></li>\
  <li class=\"bar\"><a href=\"/About\">About</a></li>\
  <li class=\"bar\"><a href=\"/Contact\">Contact</a></li>\
  <li class=\"bar\"><a href=\"/Tools\">Tools</a></li>\
  </ul>\
  <h1>Crowdsending Data</h1>\
  <h2>IoT?</h2>\
  <h3>Wikipedia:</h3>\
  <p>The Internet of things (stylised Internet of Things or IoT) is the \
  internetworking of physical devices, vehicles (also referred to as \"connected devices\" and \"smart devices\"),\
   buildings and other items-embedded with electronics, software, sensors, actuators, and network \
   connectivity that enable these objects to collect and exchange data. <br/>In 2013 the Global Standards \
   Initiative on Internet of Things (IoT-GSI) defined the IoT as \"the infrastructure of the information society.\"\
   The IoT allows objects to be sensed  and/or controlled remotely across existing network infrastructure,\
   creating opportunities for more direct integration of the physical world into computer\-based systems, \
   and resulting in improved efficiency, accuracy and economic benefit.</p>\
  <h2>What is IoT?</h2>\
     <p>\
  Today the IoT is heavily present in:\
  <ul>\
   <li>Home Automation</li>\
   <li>Industry</li>\
   <li>Telematics</li>\
   <li>WSN (Wireless Sensor Network)</li>\
   <li>Security / Surveillance</li>\
   <li>Robotics</li>\
   <li>Environmental monitoring</li>\
  </ul>\
  <br/>While the sectors in which the IoT world is channeling its attention are:\
  <ul>\
   <li>Biomedical engineering</li>\
   <li>Safety (intrinsic) device adapted to the IoT world</li>\
   <li>Scientific research</li>\
  </ul>\
  <br/>Just starting from the idea of ​​IoT, through a business model like crowdsourcing will dock right\
   in one of the areas where the IoT is focusing: collaborative IoT, a system to collect measurements\
    from devices heterogeneous  in a single location, to manage and provide methods to multiple\
     users to write and read what can be considered interesting: in short, the collaborative IoT is\
      the new frontier of sharing information .</p>\
    </body> \
  </html>";

  server.send(200, "text/html",temp);
  delay(5000);
  digitalWrite(ledPinYellow, LOW);
  //free(temp);
}
void handleTools() {
  digitalWrite(ledPinYellow, HIGH);
  String temp;
  temp="<html>\
  <head>\
  <style>";
  temp+=css;
  temp+="\
  .btn {\
    background: #75f734;\
    background-image: -webkit-linear-gradient(top, #75f734, #3d8c23);\
    background-image: -moz-linear-gradient(top, #75f734, #3d8c23);\
    background-image: -ms-linear-gradient(top, #75f734, #3d8c23);\
    background-image: -o-linear-gradient(top, #75f734, #3d8c23);\
    background-image: linear-gradient(to bottom, #75f734, #3d8c23);\
    -webkit-border-radius: 30;\
    -moz-border-radius: 30;\
    border-radius: 30px;\
    font-family: Courier New;\
    color: #040504;\
    font-size: 20px;\
    padding: 10px 20px 10px 20px;\
    text-decoration: none;\
    }\
    .btn:hover {\
    background: #3cb0fd;\
    background-image: -webkit-linear-gradient(top, #3cb0fd, #3498db);\
    background-image: -moz-linear-gradient(top, #3cb0fd, #3498db);\
    background-image: -ms-linear-gradient(top, #3cb0fd, #3498db);\
    background-image: -o-linear-gradient(top, #3cb0fd, #3498db);\
    background-image: linear-gradient(to bottom, #3cb0fd, #3498db);\
    text-decoration: none;\
    }\
  </style>\
   </head>\
   <body>  \
   <ul class=\"bar\">\
  <li class=\"bar\"><a href=\"/\">Home</a></li>\
  <li class=\"bar\"><a href=\"/IoT\" >IoT</a></li>\
  <li class=\"bar\"><a href=\"/About\">About</a></li>\
  <li class=\"bar\"><a href=\"/Contact\">Contact</a></li>\
  <li class=\"bar\"><a class=\"active\" href=\"/Tools\">Tools</a></li>\
  </ul>\
  <h1>Crowdsending Data</h1>\
  <h2>Explore your Weather Station: </h2>\
  <form action=/Reset>\
  <button class=\"btn\"  type=\"submit\" >Reset credentials!</button>\
  </form>\
  <form action=/Exit>\
  <button class=\"btn\"  type=\"submit\" >Close server!</button>\
  </form>\
  </body> \
  </html> ";
  server.send(200, "text/html", temp);
  delay(5000);
  digitalWrite(ledPinYellow, LOW);
}
void handleAbout() {
  digitalWrite(ledPinYellow, HIGH);
  String temp;
  temp="<html>\
  <head>\
  <style>";
  temp+=css;
  temp+="</style>\
   </head>\
   <body>  \
   <ul class=\"bar\">\
  <li class=\"bar\"><a href=\"/\">Home</a></li>\
  <li class=\"bar\"><a  href=\"/IoT\" >IoT</a></li>\
  <li class=\"bar\"><a class=\"active\" href=\"/About\">About</a></li>\
  <li class=\"bar\"><a href=\"/Contact\">Contact</a></li>\
  <li class=\"bar\"><a href=\"/Tools\">Tools</a></li>\
  </ul>\
  <h1>Crowdsending Data</h1>\
  <h2>About the Work</h2>\
  <p>The project consists of the design and construction of an inexpensive device to be used as a weather station.\
  The entire stage design and software development were performed on esp modules, \
  and then be integrated in the release phase, in a standalone box (battery powered \
  with lithium batteries and sotware designed to make everything low-power) with \
  two sensors: BMP280, pressure and temperature, and DHT22, humidity and temperature.\
  <br/></p>\
  <h2>About Crowdsensing</h2>\
  <p>Born as an offshoot of crowdsourcing, the crowdsensing is a business model was born in the last few \
  years, in which several devices are exploited, as well as the purposes for which they were designed \
  (call, connect to the Internet, manage multimedia files), and also to communicate share measurements ​​\
  detected by sensors on a few data collection platform.<br/>\
  In analogy, ideas, suggestions, opinions that, in the process of outsourcing were the subject of the \
  exchange in a crowdsourcing model in crowdsensensing are replaced with sensor measurements ​​and / or information related to environmental measurements or processes.\
  We distinguish two types of crowdsensing, according to the effort that the user takes to activate \
  the data communication procedure:\
  <ul>\
  <li>participatory crowdsensing in which the user must enable the detection of a sensor, or its initialization, implies the maximum effort among the possible cases;\
  Eg. SenSquare, a service that collects measurements ​​from sensors (Environmental Crowdsensing) sent by minimal \
  weather stations of users and provides an interface to the possible stakeholders to access these surveys.</li>\
   <li>opportunistic Crowdsensing, in which the user automatically sends the measurements, probably because they\
    are initialized sensors without user input (but not a power), minimum effort.\
  Eg. Google or Facebook or telephone operators, and in general the devices that lend themselves to the \
  Mobile Crowdsensing, holding, ie, traces of sensors such as light, noise (noise), location (GPS) and \
  movement (accelerometer) automatically without a explicit user action if you do not install the \
  application (and the granting of the necessary permits, as in the case of Google apps) and / or to \
  subscribe to the service (as for the operators, who can read the measurements ​​related to quality our data \
  line or to our mobile location).</li></ul></p>\
  </body> \
  </html>";
  server.send(200, "text/html", temp);
  delay(5000);
  digitalWrite(ledPinYellow, LOW);
}
void handleContact() {
  digitalWrite(ledPinYellow, HIGH);
 String temp;
 temp="<html>\
  <head>\
  <style>";
  temp+=css;
  temp+="</style>\
   </head>\
   <body>  \
   <ul class=\"bar\">\
  <li class=\"bar\"><a href=\"/\">Home</a></li>\
  <li class=\"bar\"><a href=\"/IoT\" >IoT</a></li>\
  <li class=\"bar\"><a href=\"/About\">About</a></li>\
  <li class=\"bar\"><a class=\"active\" href=\"/Contact\">Contact</a></li>\
  <li class=\"bar\"><a href=\"/Tools\">Tools</a></li>\
  </ul>\
  <h1>Crowdsending Data</h1>\
  <h2>Contact</h2>\
  <p>This project was designed and implemented by Natale Vadala', under the supervision of Prof. Luciano Bononi and Dr. Luca Bedogni at the University of Bologna.</p>\
    <p>Email: nato.vada@gmail.com</p>\
  <p>Institutional Email: natale.vadala@studio.unibo.it</p>\
  <p>Channel GitHub <a href=\"https://github.com/eldiablo100000\"> github.com/eldiablo100000</a></p>\
  <p>Link to Weather Station's Sketch <a href=\"https://github.com/eldiablo100000/WeatherStationESP\"> github.com/eldiablo100000/WeatherStationESP</a> </p>\
  </body> \
  </html> ";
  server.send(200, "text/html", temp);
  delay(5000);

  digitalWrite(ledPinYellow, LOW);
}
void handleRoot() {
  int lenHTML = 2000;
  //blinkLed(ledPinYellow,2);
  String temp;

  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  temp="<html>\
  <head>\
  <style>";
  temp+=css;
  temp+="</style>\
   </head>\
   <body>  \
   <ul class=\"bar\">\
  <li class=\"bar\"><a class=\"active\" href=\"/\">Home</a></li>\
  <li class=\"bar\"><a href=\"/IoT\" >IoT</a></li>\
  <li class=\"bar\"><a href=\"/About\">About</a></li>\
  <li class=\"bar\"><a href=\"/Contact\">Contact</a></li>\
  <li class=\"bar\"><a href=\"/Tools\">Tools</a></li>\
  </ul>\
  <h1>Crowdsending Data</h1>\
  <h2>Weather Station based on Arduino</h2>\
  <h2>Hello from ESP8266!</h2>\
  <p>Uptime: ";
  temp+=hr;
  temp+=":";
  temp+=min%60;
  temp+=":";
  temp+=sec%60;
    temp+="</p>\
  <fieldset>\
  <legend>Sign in:</legend>\
  <form action=/RegisterWStation method=POST>\
    <p><label>Name: <input type=\"text\" name=\"fullname\" placeholder=\"Alan Turing\"></label></p>\
    <p><label>ESSID: ";
    temp+=networks;
    temp+="</label></p>\
    <p><label> or hidden ESSID: <input type=\"text\" name=\"essid\" placeholder=\"Fastweb-9876527\"></label></p>\
    <p><label>Password: <input type=\"password\" name=\"password\"></label></p>\
    <p><label>Write Key: <input type=\"text\" name=\"key\" placeholder=\"lachiave\"></label></p>\
    <button type=”submit”>Write!</button>\
  </form>\
  </fieldset>  \
  </body> \
  </html> ";
  server.send(200, "text/html", temp);
  delay(5000);
  blink2Led(ledPinYellow,ledPinRed, 3);
}



//  ███████╗███████╗███╗   ██╗███████╗ ██████╗ ██████╗ ███████╗
//  ██╔════╝██╔════╝████╗  ██║██╔════╝██╔═══██╗██╔══██╗██╔════╝
//  ███████╗█████╗  ██╔██╗ ██║███████╗██║   ██║██████╔╝███████╗
//  ╚════██║██╔══╝  ██║╚██╗██║╚════██║██║   ██║██╔══██╗╚════██║
//  ███████║███████╗██║ ╚████║███████║╚██████╔╝██║  ██║███████║
//  ╚══════╝╚══════╝╚═╝  ╚═══╝╚══════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝
//                                                             

void initializeDHT22(){
  // Initialize device.

  dht.begin();
  Serial.println("[INIT. DHT] DHTxx Unified Sensor Example");
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  //Serial.println("------------------------------------");
  //Serial.println("Temperature");
  //Serial.print  ("[INIT. DHT] Sensor:       "); 
  //Serial.println(sensor.name);
  //Serial.print  ("[INIT. DHT] Driver Ver:   "); 
  //Serial.println(sensor.version);
  //Serial.print  ("[INIT. DHT] Unique ID:    "); 
  //Serial.println(sensor.sensor_id);
  //Serial.print  ("[INIT. DHT] Max Value:    "); 
  //Serial.print(sensor.max_value); 
  //Serial.println(" *C");
  //Serial.print  ("[INIT. DHT] Min Value:    "); 
  //Serial.print(sensor.min_value); 
  //Serial.println(" *C");
  //Serial.print  ("[INIT. DHT] Resolution:   "); 
  //Serial.print(sensor.resolution); 
  //Serial.println(" *C");  
  //Serial.println("------------------------------------");
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  //Serial.println("------------------------------------");
  //Serial.println("Humidity");
  //Serial.print  ("[INIT. DHT] Sensor:       "); 
  //Serial.println(sensor.name);
  //Serial.print  ("[INIT. DHT] Driver Ver:   "); 
  //Serial.println(sensor.version);
  //Serial.print  ("[INIT. DHT] Unique ID:    "); 
  //Serial.println(sensor.sensor_id);
  //Serial.print  ("[INIT. DHT] Max Value:    "); 
  //Serial.print(sensor.max_value); 
  //Serial.println("%");
  //Serial.print  ("[INIT. DHT] Min Value:    "); 
  //Serial.print(sensor.min_value); 
  //Serial.println("%");
  //Serial.print  ("[INIT. DHT] Resolution:   "); 
  //Serial.print(sensor.resolution); 
  //Serial.println("%");  
  //Serial.println("------------------------------------");
}
float readDHT22( int sens){
  // Get temperature event and print its value.
  sensors_event_t event;  
  if(sens==1){
      dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
  Serial.println("[READ DHT] Error reading temperature!");
  }
  else {
  float temp=event.temperature;
  Serial.print("[READ DHT] Temperature: ");
  Serial.print(temp);
  Serial.println(" *C");
  return temp;
  }
  }
  else if(sens==2){
    // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
  Serial.println("[READ DHT] Error reading humidity!");
  }
  else {
  Serial.print("[READ DHT] Humidity: ");
  float hum=event.relative_humidity;
  Serial.print(hum);
  Serial.println("%");
  return hum;
  }
  }
  else{
    return 0;
  }
  
}
bool initializeBMP280( ){


  if (!bmp.begin()) {
  Serial.println("[INIT. BMP] Could not find a valid BMP085 sensor, check wiring!");
  return false;
  }
  return true;
}
float readBMP280(int sens){
  float buf[2];
  if(sens==1){
    Serial.print("[READ BMP] Temperature = ");
    buf[0]=bmp.readTemperature();
    Serial.print(buf[0]);
    Serial.println(" *C");
    return buf[0];
  }

  else if(sens==2) {
    buf[1]=bmp.readPressure();
    Serial.print("[READ BMP] Pressure = ");
    Serial.print(buf[1]);
    Serial.println(" Pa");
    return buf[1];
  }
 
    else return 0;

  // Calculate altitude assuming 'standard' barometric
  // pressure of 1013.25 millibar = 101325 Pascal
  //Serial.print("[READ BMP] Altitude = ");
  //Serial.print(bmp.readAltitude());
  //Serial.println(" meters");

  //Serial.print("[READ BMP] Pressure at sealevel (calculated) = ");
  //Serial.print(bmp.readSealevelPressure());
  //Serial.println(" Pa");

  // you can get a more precise measurement of altitude
  // if you know the current sea level pressure which will
  // vary with weather and such. If it is 1015 millibars
  // that is equal to 101500 Pascals.
  //Serial.print("[READ BMP] Real altitude = ");
  //Serial.print(bmp.readAltitude(101500));
  //Serial.println(" meters");
  
  //Serial.println();
}
