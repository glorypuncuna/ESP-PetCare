#include "BluetoothSerial.h"
#define COUNT_LOW 0
#define COUNT_HIGH 8888
#define TIMER_WIDTH 16
#define SOUND_SPEED 0.034

#define WIFI_SSID "Loi"
#define WIFI_PASSWORD ""

#define API_KEY "***"

#define USER_EMAIL "kelompok7despro1@gmail.com"
#define USER_PASSWORD "****"

#define DATABASE_URL "https://petcare-5ae1b-default-rtdb.asia-southeast1.firebasedatabase.app/" 

#include <common.h>
#include <Firebase.h>
#include <FirebaseESP32.h>
#include <FirebaseFS.h>
#include <Utils.h>

#include <Arduino.h>
#include <WiFi.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#include "esp32-hal-ledc.h"

BluetoothSerial SerialBT;
String name = "DESPRO007";
char *pin = "1234"; 
bool connected;
int bluetoothFlag = 0;
int usFlag;
int count = 1;
int countTime = 0;
int usDist;

const int trigPin = 5;
const int echoPin = 18;
const int buzzerPin = 27;

long duration;
float distanceCm;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String serialNumber = "DESPRO007";

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 3600;

void setup() {
  Serial.begin(115200);
  Serial.println(ESP.getFreeHeap());
  wifiSetup();
  sendDataFirebase();
  //Ultrasonik
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //Buzzer
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);

  //Bluetooth sebagai master
  SerialBT.begin("DESPRO", true); 
  Serial.println("Perangkat dimulai.....");
  connected = SerialBT.connect(name);

  ledcSetup(1, 50, TIMER_WIDTH);
  ledcAttachPin(2, 1);
}

void loop() {
  bluetooth();    
}

void wifiSetup(){
  //Wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
  }
  
  //Setting firebase dan NTP untuk local date
  config.api_key = API_KEY;

  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  }

void bluetooth(){
  bluetoothFlag = 0;
  usFlag = 0;
  int ultraDist = ultrasonic();
  if(SerialBT.connect(name)) {
    Serial.println("Connected Succesfully!");
    if (ultraDist < 10){
      usFlag = 1; 
      count += 1; 
      }
    bluetoothFlag = 1;
    countTime += 1;
  }
  if (SerialBT.disconnect()) {
    Serial.println("Disconnected Succesfully!");
  }

  if (bluetoothFlag == 1 && count == 1 && usFlag == 1){
    digitalWrite(buzzerPin, LOW);
    delay(500);
    digitalWrite(buzzerPin, HIGH);
    servo();
    Serial.println("Servo Jalan");
    ESP.restart();
    } else if (bluetoothFlag == 0){
      count = 0;
      countTime = 0;
      digitalWrite(buzzerPin, HIGH);
      Serial.println("Tidk Terdeteksi");
    }

   if (countTime == 3600){
    count = 0;
    }
    
  Serial.println(count);
  Serial.println(bluetoothFlag);
  Serial.println(usFlag);
  delay(1000);
}


void servo() {
   for (int i=COUNT_LOW ; i < COUNT_HIGH ; i=i+200)
   {
      ledcWrite(1, i);
      delay(50);
   }
}

int ultrasonic(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  
  distanceCm = duration * SOUND_SPEED/2;
  
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);

  return distanceCm;
}

void sendDataFirebase(){
    String date = printLocalTime();
    String pathName = serialNumber + "/" + date;
    Serial.println(pathName);
    if (Firebase.RTDB.setInt(&fbdo, pathName, 1)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    
    WiFi.disconnect(true);
    delay(5000);
 }


 String printLocalTime()
{
  time_t rawtime;
  struct tm timeinfo;
  String aaString;
  if(!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return "Failed";
  }
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  String asString(timeStringBuff);
  return asString;
}
