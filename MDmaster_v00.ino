/*
 * Maggiordomotic (www.maggiordomotic.it)
 * 
 *          MDmaster_v01
 * 
 * This sketch is for ESP8266 chip
 *
 * See our website for more information
 * 
 * This code is in the public domain.
 * modified 11 October 2016
 * by Maggiordomotic
 */

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/*
 *    Change the labels (between " " ) according to your NodeRed settings :
 */
/**/  const char* esid =             "...your ssid....";    //SSID
/**/  const char* epass =            "...your pswd....";    //SSID's PWD
/**/  const char* mqttServerIP =     ".your IP server.";    //MQTT server IP
/**/
/**/  const char* mqttESPname =      "MDtest";              //ESP name
/**/  const char* mqttBuiltinLed =   "/MDtest-builtinled";  //builtin led
/**/  const char* mqttDigOutput =    "/MDtest-relays";      //output to relays 
/**/  const char* mqttDigInput =     "/MDtest-digitalIn";   //digital input request from NR
/**/  const char* nrDigInput =       "/MDtest-digitalInNr"; //digital input answer to NR
/**/  const char* mqttAnalogA0 =     "/MDtest-analogA0";    //analog input request from NR
/**/  const char* nrAnalogA0 =       "/MDtest-analogA0nr";  //analog input answer to NR
/**/  const char* mqttSerialData =   "/MDtest-read-data";   //string from NR to UART
/**/  const char* nrSerialData =     "/MDtest-send-data";   //string from UART to NR
/**/  const char* mqttDHT22 =        "/MDtest-dht22";       //DHT request from NR
/**/  const char* nrTemperature =    "/MDtest-temperature"; //DHT temperature answer to NR
/**/  const char* nrHumidity =       "/MDtest-humidity";    //DHT humidity answer to NR
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/*
 * PIN mapping (change pin # according to your settings)
 * 
 * pin #4 e #5 can be used for i2c devices (default on wire.h)
 * pin #0 is huzzah hw switch & led used for reset (not change)
 * warning: pin #2 #15 #16 check ESP datasheet before use it !
*/
/**/  #define DHTPIN  14
/**/  #define pinDigInput 12
/**/  #define pinDigOutput 13
/**/  #define pinAnalogA0 A0
/**/  #define pinPushLedRed 0 //led red (all board) & internal switch (only huzzah)
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/*
 * Library
 */
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <MQTTClient.h>
#include <DHT.h>
#define DHTTYPE DHT22
WiFiClient net;
MQTTClient client;
DHT dht(DHTPIN, DHTTYPE, 22);
/*
 * Predefine for setup 
 */
void connect();
/*
 * Global variables
 */
float humidity, temp_f, temp_c;
unsigned long lastMillis = 0;
boolean stato = false;
unsigned long previousMillis = 0;
const long interval = 2000;
int sensorValue = 0;

/*
 * SETUP
 */

void setup() {

  Serial.begin(9600);

  pinMode(pinDigOutput, OUTPUT);
  pinMode(pinDigInput, OUTPUT); 
  pinMode(pinPushLedRed, OUTPUT); 
  delay(10);
  Serial.print("Wifi begin...");
  WiFi.begin(esid, epass);
  Serial.println("done!");
  Serial.print("Client begin...");
  client.begin(mqttServerIP, net);
  Serial.println("done!");
  connect();
  
}//setup


void connect() {
  Serial.print("Connecting wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("done!");

  Serial.print("Connecting MQTT...");
  while (!client.connect(mqttESPname, "", "")) { //"arduino", "mqttuser", "mqttppassword"
    Serial.print(".");
    }
  Serial.println("done!");
  
  Serial.print("Subscribe feed...");
  client.subscribe(mqttBuiltinLed);
  client.subscribe(mqttDigOutput);
  client.subscribe(mqttAnalogA0);
  client.subscribe(mqttDigInput);
  client.subscribe(mqttDHT22);
  Serial.println("done!");

}//connect

/*
 * VOID LOOP
 */
void loop() {
  client.loop();
  delay(10); // <- fixes some issues with WiFi stability

  if(!client.connected()) {
    connect();
  }

  // every 5 minutes
  if(millis() - lastMillis > 300000) {
    lastMillis = millis();
      /*
       * do something if needed
       */
  }

}//loop

void gettemperature() {
  // Wait at least 2 seconds seconds between measurements.
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor 
    previousMillis = currentMillis;   

    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();          // Read humidity (percent)
    temp_f = dht.readTemperature(true);     // Read temperature as Fahrenheit
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temp_f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
  }
}//gettemperature


void messageReceived(String topic, String payload, char * bytes, unsigned int length) {

  //debug incoming topic
  Serial.print("incoming: ");
  Serial.print(topic);
  Serial.print(" - ");
  Serial.print(payload);
  Serial.println();

  if(topic == mqttBuiltinLed){
    stato =!stato;
    digitalWrite(pinPushLedRed, stato);
  }

  if(topic == mqttDigOutput){
     digitalWrite(pinDigOutput, payload == "1" ? HIGH : LOW);    
       Serial.print("Relay: ");
       Serial.println(payload);
  }

  if(topic == mqttAnalogA0){
    sensorValue = analogRead(pinAnalogA0);
    String trimmer_str; //see last code block below use these to convert the float that you get back from DHT to a string =str
    char trimmer[50];
    trimmer_str = String(sensorValue); //converting ftemp (the float variable above) to a string 
    trimmer_str.toCharArray(trimmer, trimmer_str.length() + 1); //packaging up the data to publish to mqtt whoa...
    client.publish(nrAnalogA0, trimmer);
  }  
 
  if(topic == mqttDigInput){
    String statoDigInput01 = "0";
    if(digitalRead(pinDigInput))  statoDigInput01 = "1";
    client.publish(nrDigInput, statoDigInput01);
  }

 
  if(topic == mqttDHT22){
    gettemperature();
    String temp_str; //see last code block below use these to convert the float that you get back from DHT to a string =str
    String hum_str;
    char temp[10];
    char hum[10];
    temp_str = String(temp_f); //converting ftemp (the float variable above) to a string 
    temp_str.toCharArray(temp, temp_str.length() + 1); //packaging up the data to publish to mqtt whoa...
    hum_str = String(humidity); //converting Humidity (the float variable above) to a string
    hum_str.toCharArray(hum, hum_str.length() + 1); //packaging up the data to publish to mqtt whoa...
    client.publish(nrTemperature, temp);
    client.publish(nrHumidity, hum);
  }

  if(topic == nrSerialData){
    Serial.println(payload);
    delay(500);

  }

}//messageReceived


