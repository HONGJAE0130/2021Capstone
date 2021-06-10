#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h> // change to #include <WiFi101.h> for MKR1000
#include <ArduinoJson.h>
#include "arduino_secrets.h"
const char ssid[]        = SECRET_SSID;
const char pass[]        = SECRET_PASS;
const char broker[]      = SECRET_BROKER;
const char* certificate  = SECRET_CERTIFICATE;

WiFiClient    wifiClient;            // Used for the TCP socket connection
BearSSLClient sslClient(wifiClient); // Used for SSL/TLS connection, integrates with ECC508
MqttClient    mqttClient(sslClient);

#include <SPI.h>
#include <WiFiUdp.h>
#include <RTCZero.h>
RTCZero rtc;
unsigned long lastMillis = 0;
unsigned long epoch;
const int GMT = 2;
int Htime;
int Mtime;

//Wire 통신용
#include <Wire.h>
#include <String.h>
char wstr[32];

//payload 저장용
const char* roomnum;
const char* datatype;
const char* DeviceAttached;
const char* Dname;
const char* name;
const char* heart;
const char* ecgdata;
const char* conList;
char bluetoothAttached=NULL;

const char* conList1;
const char* name1;

int c = 0;
int check;
int repeat=0;
int first=0;

//데이터를 쪼개서 보내고 받을 때 사용할 문자열
char data0[32];
String str = "";
char charstr[32];

//와이파이와 연결된 시간 반환
unsigned long getTime() {
  return WiFi.getTime();
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);
  if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    while (1);
  }
  ArduinoBearSSL.onGetTime(getTime);
  sslClient.setEccSlot(0, certificate);
  mqttClient.onMessage(onMessageReceived);
  roomnum="CareRoom1";
  name1="김동률";
  conList1="18:00@needtofill";
}

void loop()
{
  getConnection();
  getWireData();
  getTimeData();
  sendData();
  //일렬의 데이터 최대 개수가 28개, 56번 반복하면 좀 쉬자.
  repeat++;
  delay(1000);
}

void getRidOfWireData(){
  Wire.begin(8);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent2); // register event
}

void receiveEvent2(int howMany)
{
  Serial.println("Erasing data...");
  while (0 < Wire.available()) // loop through all but the last
  {
    char c = Wire.read(); // receive byte as a character
  }
  Serial.println("Erased Data.");
}

void getConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (!mqttClient.connected()) {
    // MQTT client is disconnected, connect
    connectMQTT();
  }
  mqttClient.poll();
}

//와이파이 연결
void connectWiFi() {
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(ssid);
  Serial.print(" ");

  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();

  Serial.println("You're connected to the network");
  Serial.println();
}

//MQTT 서버의 shadow에 연결
void connectMQTT() {
  Serial.print("Attempting to MQTT broker: ");
  Serial.print(broker);
  Serial.println(" ");

  while (!mqttClient.connect(broker, 8883)) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();

  Serial.println("You're connected to the MQTT broker");
  Serial.println();

  // subscribe to a topic 사물 변경 요함
  mqttClient.subscribe("$aws/things/CareRoom1/shadow/update/delta");
  mqttClient.subscribe("$aws/things/Patient1/shadow/update/delta");
}

void getWireData() {
  Wire.begin(8);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
}

void receiveEvent(int howMany)
{
  int Sdata=0;
  Serial.print("Got data: ");
  while (0 < Wire.available()) // loop through all but the last
  {
    char c = Wire.read(); // receive byte as a character
    str += String(c);
  }
  Serial.println(str);
  sprintf(charstr, "%s", str.c_str());
  tokenstr(); //데이터를 쪼개는 메소드
}

void tokenstr() {
  Serial.println(charstr);
  char* token = strtok(charstr, " ");
  int count = 0;
  while(token!=NULL){
    if(count==0) {
      if(strncmp(token, "Car", 3)==0) roomnum=token;
      else {
        ecgdata=token;
        bluetoothAttached='Y';
      }
    }
    if(count==1) {
      if(strncmp(token, "-", 1)==0 || strncmp(token, "-C", 2)) bluetoothAttached='N'; //이 데이터를 통해 연결된 것이 없음을 통지
      else{ 
        DeviceAttached=token;
        bluetoothAttached='Y';
      }
    }
    if(count==2) Dname=token;
    token=strtok(NULL, " ");
    count++;
  }
}

void getTimeData() {
  while (true) {
    Serial.print(".");
    epoch = getTime(); // 와이파이를 통해 현재 시간을 받는다.
    if (epoch != NULL) break;
  }
  Serial.println();
  rtc.setEpoch(epoch);
  Htime = rtc.getHours() + GMT;
  Mtime = rtc.getMinutes();
  setDataType();
  Serial.println(Htime);
}

//2, 3이면 한국 시간으로 새벽 2시 0~3분 이를 수정해서 데이터를 보내면 된다.
void setDataType() {
  if (Htime == 2 && Mtime < 3) {
    datatype = "2";
  }
  else datatype = "1";
}

void sendData() {
  char payload[256];
  char payload2[256]; //Patient1에게 전송할 데이터
  setDeviceStatus(payload);
  setDeviceStatus2(payload2);
  sendMessage(payload, payload2);
  Serial.println(str);
}

//datatype은 gettimed에서, roomnum, DeviceAttached, name, ecg는 받는다.
void setDeviceStatus(char* payload) {
  //환자용 디바이스와 블루투스 미연결 상태
  Serial.println(bluetoothAttached);
  if(bluetoothAttached=='N' || bluetoothAttached==NULL){
    DeviceAttached="N";
    datatype="3";
    Dname="";
    name="";
    heart="";
    conList="";
    ecgdata="";
    sprintf(payload,"{\"state\":{\"reported\":{\"Datatype\":\"%s\",\"Room\":\"%s\",\"Attached\":\"%s\",\"DeviceName\":\"%s\",\"Name\":\"%s\",\"Heart\":\"%s\",\"List\":\"%s\",\"ECGdata\":\"%s\"}}}",
  datatype, roomnum, DeviceAttached, Dname, name, heart, conList, ecgdata);
  }
  else{
    if(strcmp(Dname, "Patient1")==0){
      heart="Normal";
      sprintf(payload,"{\"state\":{\"reported\":{\"Datatype\":\"%s\",\"Room\":\"%s\",\"Attached\":\"%s\",\"DeviceName\":\"%s\",\"Name\":\"%s\",\"Heart\":\"%s\",\"List\":\"%s\",\"ECGdata\":\"%s\"}}}",
    datatype, roomnum, DeviceAttached, Dname, name1, heart, conList1, ecgdata); 
    }
  }
}

//datatype은 gettimed에서, roomnum, DeviceAttached, name, ecg는 받는다.
void setDeviceStatus2(char* payload2) {
  //환자용 디바이스와 블루투스 미연결 상태
  Serial.println(bluetoothAttached);
  if(bluetoothAttached=='N' || bluetoothAttached==NULL){
    DeviceAttached="N";
    datatype="3";
    Dname="Patient1";
    heart="";
    ecgdata="";
    sprintf(payload2,"{\"state\":{\"reported\":{\"Datatype\":\"%s\",\"Room\":\"%s\",\"Attached\":\"%s\",\"DeviceName\":\"%s\",\"Name\":\"%s\",\"Heart\":\"%s\",\"List\":\"%s\",\"ECGdata\":\"%s\"}}}",
  datatype, "", DeviceAttached, Dname, name1, heart, conList1, ecgdata);
  }
  else{ 
    name1="김동률";
    conList1="18:00@DoSomeThing";
    sprintf(payload2,"{\"state\":{\"reported\":{\"Datatype\":\"%s\",\"Room\":\"%s\",\"Attached\":\"%s\",\"DeviceName\":\"%s\",\"Name\":\"%s\",\"Heart\":\"%s\",\"List\":\"%s\",\"ECGdata\":\"%s\"}}}",
  datatype, roomnum, DeviceAttached, Dname, name1, heart, conList1, ecgdata);
  }
}

void sendMessage(char* payload, char* payload2) {
  char TOPIC_NAME1[]= "$aws/things/CareRoom1/shadow/update";
  char TOPIC_NAME2[]= "$aws/things/Patient1/shadow/update";

  Serial.print("Publishing send message:");
  Serial.println(payload);
  mqttClient.beginMessage(TOPIC_NAME1);
  mqttClient.print(payload);
  mqttClient.endMessage();
  delay(100);
  //Patient1이 연결되면 해당 데이터도 보내라
  if(strcmp(Dname, "Patient1")==0){
    Serial.println(payload2);
    mqttClient.beginMessage(TOPIC_NAME2);
    mqttClient.print(payload2);
    mqttClient.endMessage();
    delay(100);
  }
}

void onMessageReceived(int messageSize) {
  
}
