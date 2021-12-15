#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <FirebaseArduino.h>
#include<math.h>

WiFiServer server(80);
ESP8266WebServer webserver(88);
IPAddress IP(192,168,4,15);
IPAddress mask = (255, 255, 255, 0);

#define LED_PIN D5
#define GND_PIN D7
#define FIREBASE_HOST "gps-app-cb9a0-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "9hq341OUB2rBtmemK6xj9vJGUaGpY2Fc657clOcQ"

//float dest_lat = 19.0750;
//float dest_longi = 72.8850;
float threshold = 0.9000;
float am_lat;
float am_longi;
float dest_lat;
float dest_longi;
float distance;
float m1;
float c1;
float m2;
float c2;
float p;
float q;
char one = '1';
char zero = '0';
int count = 2;

int node_data[2][2] = {{19.0710,72.8810},{19.0740,72.8840}};

String token = "000";

String slave_resp;

unsigned long current_time = 0;
unsigned long set_time = 0;
int period = 90000;

int track = 1;

void setup() {

  pinMode(LED_PIN,OUTPUT);
  pinMode(GND_PIN,OUTPUT);

  Serial.begin(9600);
  delay(5000);
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Wemos_AP", "Wemos_comm");
  WiFi.softAPConfig(IP, IP, mask);
  server.begin();

  Serial.println("WIfi Server started.");
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  webserver.on("/RPI",handleRPI);
  webserver.begin(); 

  set_time = millis();
  
}
void loop() {

  Serial.println("Start loop: " +String(track));
  Serial.println();

  current_time = millis();

  if (current_time - set_time > period) {
    
    set_time = millis();
    
    WiFi.mode(WIFI_OFF); 
    Serial.println("Wifi turned Off");
    Serial.println();

    WiFi.mode(WIFI_STA);
    WiFi.begin("TP-Link_D8A7", "97633549");
    Serial.println("Connecting to the internet");
    while(WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());

    Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);

    String path = "/Users/fMD7jJ86X5gaf3FdU0EYKY8fI9C3/GPSdata/";
    FirebaseObject object = Firebase.get(path);
  
    am_lat = object.getFloat("latitude");
    am_longi = object.getFloat("longitude");

    Serial.println("Ambulance location data: ");
    Serial.print("Latitude: ");
    Serial.println(am_lat,4);
    Serial.print("Longitude: ");
    Serial.println(am_longi,4);
    Serial.println();

    String path1 = "/Users/fMD7jJ86X5gaf3FdU0EYKY8fI9C3/Destination/";
    FirebaseObject object1 = Firebase.get(path1);
  
    dest_lat = object1.getFloat("latitude");
    dest_longi = object1.getFloat("longitude");

    Serial.println("Destination data: ");
    Serial.print("Latitude: ");
    Serial.println(dest_lat,4);
    Serial.print("Longitude: ");
    Serial.println(dest_longi,4);
    Serial.println();

    Serial.println("Hosting local wifi network again");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("Wemos_AP", "Wemos_comm");
    WiFi.softAPConfig(IP, IP, mask);
    server.begin();
  
    Serial.println("WIfi Server started.");
    Serial.print("IP: "); Serial.println(WiFi.softAPIP());
    Serial.println();
  
    webserver.on("/RPI",handleRPI);
    webserver.begin(); 

    m1 = float((dest_longi-am_longi)/(dest_lat-am_lat));
    c1 = float(dest_longi-(((dest_longi-am_longi)/(dest_lat-am_lat))*dest_lat));
    
    for (int i=0;i<count;i++) {
      node_checker(m1,c1,node_data[i][0],node_data[i][1], i);
      Serial.println("Token: ");
      Serial.print(token);
      Serial.println();
    }    

    track = 0;
  }

  webserver.handleClient();
  
  WiFiClient client = server.available();
  if (client) {
    Serial.println();
    Serial.println("Data sent to Slave: "+token);
    client.println(token + "\r");
    slave_resp = client.readStringUntil('\r');
    Serial.println("Response from the slave: " + slave_resp);
    client.flush();
    client.stop();

    Serial.println("End loop");
    Serial.println();
  
    track = track + 1;
    delay(1000);
    return;
  }
  else {
    Serial.println("Failed to send data to station");
    client.flush();
    client.stop();
  
    Serial.println("End loop");
    Serial.println();
  
    track = track + 1;
    delay(1000);
    return;
  }

}


void handleRPI() {
 
  if (webserver.hasArg("trigger")== false){
    webserver.send(200,"text/plain","Body not recieved");
    Serial.println("Body not found");
  }
  else{
    Serial.println("RPI request recieved");
    if (webserver.arg("trigger") == "1"){
      token.setCharAt(0,one);
      if (token[1]=='1') {
        digitalWrite(GND_PIN, LOW);
        digitalWrite(LED_PIN, HIGH);
        Serial.println("Master LEDs activated");
      }
      webserver.send(200,"text/plain","Message received");
    }
    else {
      token.setCharAt(0,zero);
      digitalWrite(LED_PIN, LOW);
      Serial.println("Master LEDs deactivated");
      webserver.send(200,"text/plain","Master D1 mini not triggered");
    }
  }
}

void node_checker(float M, float C, float X, float Y, int i) {
  m2 = float(-1/M);
  c2 = float(Y + (X/M));
  p = float((c2-C)/(M-m2));
  q = float(m2*p+c2);
  
  distance = float(sqrt(sq(p-X)+sq(q-Y)));

  if (distance <= threshold) {
    token.setCharAt(i+1,one);
  }
}
