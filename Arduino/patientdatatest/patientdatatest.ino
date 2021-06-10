#define USE_ARDUINO_INTERRUPTS true
#include <SoftwareSerial.h> 
//블루투스용
SoftwareSerial BTSerial(2, 3); // 소프트웨어 시리얼 (TX,RX) 

//심전도용
const int LoP = 10; //Lo+
const int LoM = 11; //Lo-
int Threshold=550;

char heartdata[900];
char ecgdata[32];
const char* name;
const char* DeviceAttached;
int count=0;
int repeat=0;
int index;
int index1;
int before=0;
int num=1;
int datasent=0;
boolean getout=false;


void setup(){
  Serial.begin(115200);            
  Serial.println("Serial begin");     
  BTSerial.begin(115200);
  Serial.println("Bluetooth begin");
  pinMode(LoP, INPUT);
  pinMode(LoM, INPUT);
  name="Patient1";
}

void loop(){
  //heartdata가 꽉 채워질때까지 기다리다가 다 채워지면 전송한다. 
  if (BTSerial.available()) { 
    //데이터 받는 놈      
  }
  getHeartdata();
  repeat++;
  if(datafull()==true){
    Serial.println("Sending Message2");
    char payload[1024];
    getDeviceStatus2(payload);
    int wait = 30000 - ((repeat+num)*10); //30초 주기를 맞추기 위해
    delay(wait);
    datasent++;
  }
  if(repeat==200){
    Serial.print("Sending Message");
    char payload[1024];
    if(datafull==true) {
      Serial.println("2");
      getDeviceStatus2(payload);
    }
    else {
      Serial.println("1");
      getDeviceStatus1(payload);
    }
    delay(28000); //30초 주기를 맞추가 위해 추가
    datasent++;
  }
  if(datasent!=0) initData();
  delay(10);//전체 과정을 5초 주기로 반복
}

//데이터가 부족하거나 없을 때 ecgdata없이 전송
void getDeviceStatus1(char* payload){
  strcpy(ecgdata, "");
  sprintf(payload, "%s %s %s", DeviceAttached, name, ecgdata);
  Serial.println(payload);
  BTSerial.write(payload);                                                 
  BTSerial.println();
}

//heartdata가 포함된 데이터를 보내기.
//heartdata는 755~944 char 데이터 보낼 예정
void getDeviceStatus2(char* payload){
  while(true){
    setDeviceData();
    sprintf(payload, "%s %s %s", DeviceAttached, name, ecgdata);
    Serial.println(payload);
    BTSerial.write(payload);                                                 
    BTSerial.println();
    delay(1000);
    if(getout==true) break;
  }
}

void setDeviceData(){
  int repeat1=0;
  String temp="";
  for(int i=0; i<32; i++){
    char a = heartdata[i+before];
    temp += a; 
    //이 값들을 temp 값으로 바꾸기만 하면 될듯
  }
  do{
    index = temp.indexOf(',', before); //7번째가 정해짐
    if(index == -1) break;
    index1 = temp.indexOf(',', index+1);
    if(index1 == -1) break;
    before=index1+1;
    repeat1++;
  } while(repeat1<4);
  if(index==-1 || index1==-1){
    Serial.println("temp is gone");
    String cuttemp = temp.substring(0, index);
    sprintf(ecgdata, "%d)%s", num, cuttemp.c_str());
    temp="";
    getout = true;
    //끝을 내는 무엇을 반환
  }
  before=index+1;
  String cuttemp = temp.substring(0, index);
  sprintf(ecgdata, "%d)%s", num, cuttemp.c_str()); 
  num++;
  initindex();
}

//우선 연결 확인 하고 정해진 시간에 데이터를 200개씩 수집해서 문자열에 저장
void getHeartdata(){
  //제대로 연결됨 확인(착용중 여부)
  if((digitalRead(LoP) == 1)||(digitalRead(LoM) == 1)){
    DeviceAttached="N";
    strcpy(heartdata, "");
    count=0;
  }
  //1 N Patient1 Normal 식으로 출력됨
  else{
    //검사 시작
    DeviceAttached="Y";
    int rd;
    rd = analogRead(A0);
    char temp[4];
    if(count==188) sprintf(temp, "%d", rd); // 맨 마지막에는 , 없이 
    else sprintf(temp, "%d,", rd);
    strcat(heartdata, temp);  
    Serial.println(heartdata);
    count++;
  }
  delay(10);
}

//위 heartdata가 189개를 쌓았을 때 datafull을 반환
boolean datafull(){
  if(count==189) return true;
  else return false;
}

void initindex(){
  index=0;
  index1=0;
  before=0;
}

void initData(){
  num=1;
  datasent=0;
  count=0;
  repeat=0;
  strcpy(heartdata, "");
}
