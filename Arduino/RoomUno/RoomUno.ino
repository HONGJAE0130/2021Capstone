#include <avr/pgmspace.h>

//Wiren
#include <Wire.h>

//Bluetooth
#include <SoftwareSerial.h>
SoftwareSerial bluetooth(2, 3);

//사용 가능한 메모리 부족, 안정성에 문제가 생길 수 있음
String getpayload2; // 임의로 만든 놈, 2 datatype을 전송하지 위함
int repeat=0;
int count=0;
int check=0;
const char* DeviceAttached; 
const char* Dname;
const char* roomnum;
const char* ecgdata;
const char* NoBlueDevice;
int status;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  bluetooth.begin(115200);
  roomnum="CareRoom1";
  pinMode(7, INPUT);
  Serial.println("begin");
}

void loop() {
  // put your main code here, to run repeatedly:
  status=digitalRead(7);
  Serial.println(status);
  getBlueData();
  //여기서 데이터 저장하는 부분을 만들자
  Serial.println(F("Nothing is attached."));
  NoBlueDevice="-";
  check=0;
  if(status!=0) sendWireData();
  repeat++;
  if(repeat==200) initData();
  delay(1000);
}

//블루투스 연결 유무 확인 방법을 알아보자
void getBlueData(){
  String getpayload=""; //나중에 getpayload2 대신 정의하면 됨
  Serial.println(F("BlueTooth Thread Ready..."));    
  if (bluetooth.available()) {       
    char RdChar=(char)bluetooth.read();
    getpayload+=RdChar;
  }
  DevideData(getpayload);
}

void DevideData(String str){
  //연결은 되었는데 넘어오는 데이터가 없을 때
  if(str==NULL || str=="") {
    check=0;
    return;
  }
  //넘어오는 데이터가 존재할 때
  else{
    check=1;
    tokenstr(str);
  }
}

void tokenstr(String str1){
  char wstr[64];
  sprintf(wstr, "%s", str1.c_str());
  char * token = strtok(wstr, " ");
  int count=0;
  while(token!=NULL){
    if(count==0) DeviceAttached=token;
    if(count==1) Dname=token;
    if(count==2) {
      ecgdata=token; 
    }
    token=strtok(NULL, " ");
    count++;
  }
}
//CareRoom1 Y Patient1 1)500,500,500,500,500,500,500
void sendWireData(){
  char payload[32];
  if(check==0){
    sprintf(payload, "%s %s", roomnum, NoBlueDevice);
  }
  // CareRoom1 Y Patient 정보를 넘김
  if(check==1) {
    sprintf(payload, "%s %s %s", roomnum, DeviceAttached, Dname);
    check++;
  }
  // 1)ecgdata ...... 를 보냄
  if(check==2){
    sprintf(payload, "%s", ecgdata);
  }
  Serial.println("Sending Payload");
  Serial.println(payload);
  Wire.begin();
  Wire.beginTransmission(8);
  Wire.write(payload);
  Wire.endTransmission();;
  Serial.println("End Transmission...");
}

void initData(){
  repeat=0;
  check=0;
}
