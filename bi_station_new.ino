#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#define LED_PIN D5
#define GND_PIN D7

unsigned long current_time = 0;
unsigned long set_time = 0;
int period = 10000;

WiFiClient client;

String post_resp;
String pre_resp;
unsigned char number_client;

char AP_ssid[] = "Wemos_AP"; // SSID of preceding AP
char AP_pass[] = "Wemos_comm"; // password of preceding AP
IPAddress AP_server(192,168,4,15); // IP address of the preceding AP

char ssid[] = "Wemos_A1"; // SSID of AP
char pass[] = "Wemos_comm1"; // password of AP
IPAddress IP(192,168,3,15);
IPAddress mask = (255, 255, 255, 0);
WiFiServer server(81);

String token = "000";

void setup() {
  Serial.begin(9600);
  delay(5000);

  pinMode(LED_PIN,OUTPUT);
  pinMode(GND_PIN,OUTPUT);

  Serial.println();
  Serial.println("Connecting to the preceding AP");
  WiFi.mode(WIFI_STA);
  WiFi.begin(AP_ssid, AP_pass); // connects to the WiFi of preceding AP
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected to the preceding AP. Assigned IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.println("Connecting to the preceding AP");
    WiFi.begin(AP_ssid, AP_pass); // connects to the WiFi of preceding AP
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
    }
  }

  client.connect(AP_server, 80);
  if (client) {
    pre_resp = client.readStringUntil('\r');
    client.println("Token received");
    token = pre_resp;
    Serial.print("Data received from preceding node: "); 
    Serial.println(token);
    
  }
  else {
    Serial.println("Data not received from preceding node"); 
  }
  if (token[0]=='1'&&token[2]=='1') {
    digitalWrite(GND_PIN, LOW);
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Node LEDs activated");
    //Serial.println();
  }
  else{
    digitalWrite(LED_PIN, LOW);
    Serial.println("Node LEDs deactivated");
    //Serial.println();
  }

  client.flush();
  client.stop();
  delay(1000);

  // for succeding node

  WiFi.mode(WIFI_OFF); 
  Serial.println("Wifi turned Off");
  Serial.println();
  
  Serial.println("Hosting wifi network");
  WiFi.softAP(ssid, pass);
  WiFi.softAPConfig(IP, IP, mask);
  server.begin();
  Serial.print("Hosted wifi network. AP IP: ");
  Serial.println(WiFi.softAPIP());

  Serial.println("Waiting for the succeding AP to connect");
  number_client = wifi_softap_get_station_num();
  while (number_client==0){
    number_client = wifi_softap_get_station_num();
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Succeding AP connected");
  
  client = server.available();
  if (client) {
    client.println(token+"\r");
    post_resp = client.readStringUntil('\r');
    Serial.print("Data sent to succeding node: "); 
    Serial.println(token);
    Serial.print("Response from succeding node: ");
    Serial.println(post_resp);
    
  }
  else {
    Serial.println("Data not sent to succeding node"); 
    Serial.println();
  }
  client.flush();
  client.stop();
  delay(1000);

}
