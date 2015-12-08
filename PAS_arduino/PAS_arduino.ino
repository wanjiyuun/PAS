#include "virtuabotixRTC.h"
#include <SoftwareSerial.h>
#include "pitches.h"
#define melodyPin 11
#include <stdio.h>
#include <string.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

SoftwareSerial BTSerial(3, 2); //TX, RX
virtuabotixRTC myRTC(7, 6, 5); //CLK, DAT, RST

LiquidCrystal_I2C lcd(0x27, 16, 2);

//알람 구조체
typedef struct alarm{
  String alarmname;
  int year;
  int month;
  int day;
  int hour[3];
  int minute[3];
  int flag[3];
  int empty;  //1 = 비었음, 0 = 찼음
  int imsi;
};

alarm arduinoAlarm[10];

//변수로 받을 정수값 temp
int sign = 1 ; //1 = 등록, 2 = 삭제
int year;
int month;
int day;
int hour;
int minute;

int song = 0;

//text를 자르는데 필요한 스트링 값들
String sign_s = "";
String name_s = "";
String year_s = "";
String month_s = "";
String day_s = "";
String hour_s_0 = "";
String minute_s_0 = "";
String hour_s_1 = "";
String minute_s_1 = "";
String hour_s_2 = "";
String minute_s_2 = "";
String command = "";
char temp;

//알람_루저_멜로디
int loser_melody[] = {
  NOTE_G5, 0, NOTE_AS4, 0, NOTE_G5, NOTE_G5, NOTE_AS4, 0,
  NOTE_F5, NOTE_F5, NOTE_G5, NOTE_G5, NOTE_GS5, NOTE_G5, NOTE_F5, 0,
  NOTE_DS5, 0, NOTE_G4, 0, NOTE_DS5, NOTE_DS5, NOTE_G4, 0, 
  NOTE_GS5, NOTE_G5, NOTE_F5, NOTE_F5, 0, NOTE_DS5, NOTE_F5, 0
};
//알람_루저_템포
int loser_tempo[] = { 
  4, 32, 4, 32, 8, 8, 4, 32, 
  8, 8, 8, 8, 8, 8, 4, 32,
  4, 32, 4, 32, 8, 8, 4, 32, 
  4, 8, 8, 4, 32, 8, 8, 32
};

//알람_꺼내먹어요_멜로디
int T_melody[]={
  // 시라솔# 솔#라 시솔#  솔#솔#라시솔#  솔#솔#라시솔#(파#솔파#미)
  NOTE_B5, NOTE_A5, NOTE_GS5, 0, NOTE_GS5, NOTE_A5, 0, NOTE_B5, NOTE_GS5, 0,
  NOTE_GS5, NOTE_GS5, NOTE_A5, NOTE_B5, NOTE_GS5, 0, 
  NOTE_GS5, NOTE_GS5, NOTE_A5, NOTE_B5, NOTE_GS5, 0, NOTE_FS5, NOTE_E5, 0
};
//알람_꺼내먹어요_템포
int T_tempo[] = {
  8, 8, 4, 32, 8, 4, 32, 8, 2, 32,
  8, 8, 8, 8, 2, 32,
  8, 8, 8, 8, 4, 32, 12, 12, 32
};

//알람선곡함수
void sing(int s, int index){      
   //iterate over the notes of the melody:
   song = s;
   print_time();
   Serial.println("ALARM !!!!!! TAKE IT !!!!!!");
   
   if(song == 1){
     //Serial.println(" 'TAKE IT'");
     int size = sizeof(T_melody) / sizeof(int);
     for (int thisNote = 0 ; thisNote < size ; thisNote ++) {

       // to calculate the note duration, take one second
       // divided by the note type.
       //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
       int noteDuration = 1000/T_tempo[thisNote];

       buzz(melodyPin, T_melody[thisNote], noteDuration, index);

       // to distinguish the notes, set a minimum time between them.
       // the note's duration + 30% seems to work well:
       int pauseBetweenNotes = noteDuration * 1.30;
       delay(pauseBetweenNotes);

       // stop the tone playing:
       buzz(melodyPin, 0, noteDuration, index);
    }
  }
   
   else if(song == 2){
     //Serial.println(" 'LOSER'");
     int size = sizeof(loser_melody) / sizeof(int);
     for (int thisNote = 0 ; thisNote < size ; thisNote ++) {

       // to calculate the note duration, take one second
       // divided by the note type.
       //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
       int noteDuration = 1000/loser_tempo[thisNote];

       buzz(melodyPin,loser_melody[thisNote],noteDuration,index);

       // to distinguish the notes, set a minimum time between them.
       // the note's duration + 30% seems to work well:
       int pauseBetweenNotes = noteDuration * 1.30;
       delay(pauseBetweenNotes);

       // stop the tone playing:
       buzz(melodyPin, 0, noteDuration, index);
    }
   }
}

void buzz(int targetPin, long frequency, long length, int index) {
  digitalWrite(13, HIGH);
  if(index == 0)
    digitalWrite(8, HIGH);
  else if(index == 1)  
    digitalWrite(9, HIGH);
  else 
    digitalWrite(10,HIGH);
  long delayValue = 1000000/frequency/2; // calculate the delay value between transitions
  //// 1 second's worth of microseconds, divided by the frequency, then split in half since
  //// there are two phases to each cycle
  long numCycles = frequency * length/ 1000; // calculate the number of cycles for proper timing
  //// multiply frequency, which is really cycles per second, by the number of seconds to 
  //// get the total number of cycles to produce
  for (long i = 0 ; i < numCycles ; i ++){ // for the calculated length of time...
    digitalWrite(targetPin, HIGH); // write the buzzer pin high to push out the diaphram
    delayMicroseconds(delayValue); // wait for the calculated delay value
    digitalWrite(targetPin, LOW); // write the buzzer pin low to pull back the diaphram
    delayMicroseconds(delayValue); // wait again or the calculated delay value
  }
  digitalWrite(13, LOW);
  digitalWrite(8, LOW); // Green Off  
  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
}

//alarm initializing 함수
void initAlarm(){
  int i, j;
    for(i = 0 ; i < 10 ; i ++){
      arduinoAlarm[i].alarmname = "\0";
      arduinoAlarm[i].year = -1;
      arduinoAlarm[i].month = -1;
      arduinoAlarm[i].day = -1;
      arduinoAlarm[i].empty = 1;
      arduinoAlarm[i].imsi = 0;
      for(j = 0 ; j < 3 ; j ++){
        arduinoAlarm[i].hour[j] = -1;
        arduinoAlarm[i].minute[j] = -1;
        arduinoAlarm[i].flag[j] = 0; 
    }
  }
}

//12시가 되었을 때 모든 알람의 울림을 초기화 함
void initAlarm_12(){
  int i, j;
  for(i = 0 ; i < 10 ; i ++){
    for(j = 0 ; j < 3 ; j ++){
      arduinoAlarm[i].flag[j] = 0; 
    }
  }
}

void deleteAlarm(String alarm_name){
  int i, j;
  for(i = 0 ; i < 10 ; i ++){
    if(arduinoAlarm[i].alarmname == alarm_name){
      arduinoAlarm[i].alarmname = "\0";
      arduinoAlarm[i].year = -1;
      arduinoAlarm[i].month = -1;
      arduinoAlarm[i].day = -1;
      arduinoAlarm[i].empty = 1;
      arduinoAlarm[i].imsi = 0;
      for(j = 0 ; j < 3 ; j ++){
        arduinoAlarm[i].hour[j] = -1;
        arduinoAlarm[i].minute[j] = -1;
        arduinoAlarm[i].flag[j] = 0; 
      }//for
    }//if
  }//for
}

//<> 제거 함수
String commandcut() {
  String st = "";
  unsigned int i = 0;
  if (command[i] == '<') {  //if it encounters '<'
    i++;
    while (command[i] != '>') { //if it finishes with '>'
      st = st + command[i]; //the gap string between '<', '>' is the real text
      i++;
    }
  }
  return st;  //return real text
}

//각 값들을 분리하는 함수
void each_cut(String st) {
  int i = 0, j = 0, k = 0;
  int second = 0;
  int third = 0;
  //count = count + 1;
  sign_s = "\0";
  name_s = "\0";
  year_s = "\0";
  month_s = "\0";
  day_s = "\0";
  hour_s_0 = "\0";
  minute_s_0 = "\0";
  hour_s_1 = "\0";
  minute_s_1 = "\0";
  hour_s_2 = "\0";
  minute_s_2 = "\0";
  if (st != '\0') { //if string is not null
    while (i < st.length()) { //do it while i is less then st's length
      if (st[i] == ',') { //if it encounters ','
        i++;
        j++;
      } //if
      if(j == 0){
         sign_s = sign_s + st[i];
         sign = sign_s.toInt();
      }
      else if (j == 1) 
        name_s = name_s + st[i]; 
      else if (j == 2){
        year_s = year_s + st[i];
      }
      else if (j == 3)
        month_s = month_s + st[i]; 
      else if (j == 4)
        day_s = day_s + st[i];
      else if (j == 5){ //첫번째 시
        hour_s_0 = hour_s_0 + st[i];
      }        
      else if (j == 6){ //첫번째 분
        minute_s_0 = minute_s_0 + st[i];
      }
      else if (j == 7){ //두번째 시
        hour_s_1 = hour_s_1 + st[i];
        second = 1;      
      }             
      else if (j == 8){ //두번째 분
        minute_s_1 = minute_s_1 + st[i]; 
      }
      else if (j == 9){ //세번째 시
        hour_s_2 = hour_s_2 + st[i];
        third = 1;
      }             
      else{ //세번째 분
        minute_s_2 = minute_s_2 + st[i]; 
      }
      i++;
    }//while
  }//if
  if((sign == 1)){
    for(k = 0 ; k < 10 ; k ++){
      if((arduinoAlarm[k].empty == 1) && (year_s.toInt() != 0)){
        arduinoAlarm[k].alarmname = name_s;
        arduinoAlarm[k].year = year_s.toInt();
        arduinoAlarm[k].month = month_s.toInt();
        arduinoAlarm[k].day = day_s.toInt();
        arduinoAlarm[k].hour[0] = hour_s_0.toInt();
        arduinoAlarm[k].minute[0] = minute_s_0.toInt();
        if(second == 1){
          arduinoAlarm[k].hour[1] = hour_s_1.toInt();
          arduinoAlarm[k].minute[1] = minute_s_1.toInt();
        }
        if(third == 1){
          arduinoAlarm[k].hour[2] = hour_s_2.toInt();
          arduinoAlarm[k].minute[2] = minute_s_2.toInt();
        }
        arduinoAlarm[k].empty = 0;
        break;
      }//if
    }//for
  }
  else if(sign == 2){ //sign = 2, 삭제버튼누름
    deleteAlarm(name_s);
  }
}

void print_time(){
  //This allows for the update of variables for time or accessing the individual elements.              
  myRTC.updateTime();                                                                                   
                                                                                                 
  //Start printing elements as individuals                                                       
  Serial.print("Current Date / Time: ");                                                         
  Serial.print(myRTC.year);                                                                  
  Serial.print("/");
  Serial.print(myRTC.month);                                                                    
  Serial.print("/");
  Serial.print(myRTC.dayofmonth); 
  Serial.print("  ");
  Serial.print(myRTC.hours);                                                                    
  Serial.print(":");                                                                                     
  Serial.print(myRTC.minutes);                                                                  
  Serial.print(":");                                                                        
  Serial.println(myRTC.seconds);
  delay(1000);                             
}

void print_alarm(){
  int i;
  for(i = 0 ; i < 10 ; i ++){
    if((arduinoAlarm[i].empty == 0) && (arduinoAlarm[i].imsi == 0)){
      arduinoAlarm[i].imsi = 1;
      Serial.print("Alarm name : ");
      Serial.println(arduinoAlarm[i].alarmname);
      Serial.print("Due date : ");
      Serial.print(arduinoAlarm[i].year);
      Serial.print("/");
      Serial.print(arduinoAlarm[i].month);
      Serial.print("/");
      Serial.println(arduinoAlarm[i].day);
      Serial.print("FirstAlarm : ");
      Serial.print(arduinoAlarm[i].hour[0]);
      Serial.print(" : ");
      Serial.println(arduinoAlarm[i].minute[0]);
      Serial.print("SecondAlarm : ");
      Serial.print(arduinoAlarm[i].hour[1]);
      Serial.print(" : ");
      Serial.println(arduinoAlarm[i].minute[1]);
      Serial.print("ThirdAlarm : ");
      Serial.print(arduinoAlarm[i].hour[2]);
      Serial.print(" : ");
      Serial.println(arduinoAlarm[i].minute[2]);
     }//if empty 
   }//for  
}

//시간비교함수
void compare_time(int alarm_index){
  int k, i;
  int comp_hour = -1;
  int comp_min = -1;
  
  for(i = 0 ; i < 3 ; i ++){ 
    comp_hour = arduinoAlarm[alarm_index].hour[i];
    comp_min = arduinoAlarm[alarm_index].minute[i];
    myRTC.updateTime();  
    if(arduinoAlarm[alarm_index].flag[i] == 0){ //아직 비교하지 않았다면
       if(myRTC.year <= arduinoAlarm[alarm_index].year){  //종료일과 년이 같거나 작다면
        if((myRTC.month < arduinoAlarm[alarm_index].month) ||
          ((myRTC.month == arduinoAlarm[alarm_index].month) && (myRTC.dayofmonth <= arduinoAlarm[alarm_index].day))){
            //종료일의 월보다 현재 월이 더 작거나 || 종료일의 월과 현재 월이 같고, 날짜도 더 작거나 같은 경우
            if((comp_hour == myRTC.hours) && (comp_min == myRTC.minutes)){
                lcd.backlight();
                lcd.print("Time > ");
                lcd.print(myRTC.hours);
                lcd.print(" : ");
                lcd.print(myRTC.minutes);
                
                if(i == 0){
                  digitalWrite(8, HIGH);  //R
                  //lcd.print(comp_hour + " : " + comp_min);
                  lcd.setCursor(0, 1);
                  lcd.print(arduinoAlarm[alarm_index].alarmname + " - 1st");
                  sing(1, i); sing(1, i);
                 }
                 else if(i == 1){
                  digitalWrite(9, HIGH); //G
                  lcd.setCursor(0, 1);
                  lcd.print(arduinoAlarm[alarm_index].alarmname + " - 2nd");
                  sing(2, i); sing(2, i);
                 }
                 else if(i == 2){
                  digitalWrite(10, HIGH);  //B
                  lcd.setCursor(0, 1);
                  lcd.print(arduinoAlarm[alarm_index].alarmname + " - 3rd");
                  sing(1, i); sing(2, i);
                 }
                 lcd.clear();
                 lcd.noBacklight();
                 digitalWrite(8, LOW); // Green Off  
                 digitalWrite(9, LOW);
                 digitalWrite(10, LOW);
                arduinoAlarm[alarm_index].flag[i] = 1;
            }  //시간비교 if
            else{  //다른 조건은 다 좋은데 시간이 다르다면
             break;
            }
         }  //시간비교 가능한경우의 년, 월, 일 비교 끝
         else{ //종료일의 월이 같고, 현재 일은 더 크다면 알람을 지움
            deleteAlarm(arduinoAlarm[alarm_index].alarmname);
            break;
         }
       } //년도 비교(더 년도가 작거나 같은 경우)
       else{ //해당 년도보다 더 크면
        deleteAlarm(arduinoAlarm[alarm_index].alarmname);
        break;
       }
    } //if flag = 0(아직 비교 안한경우)
  }//for
}//함수끝

void setup() {

  Serial.begin(9600);
  BTSerial.begin(9600);
  myRTC.setDS1302Time(20, 31, 21, 2, 5, 10, 2015);
  
  lcd.init(); // initialize lcd 
  
  pinMode(11, OUTPUT);//buzzer
  pinMode(13, OUTPUT);//led indicator when singing a note
  
  pinMode(10, OUTPUT); //Red 
  pinMode(9, OUTPUT);  //Green   
  pinMode(8, OUTPUT); // Blue
  
  initAlarm();
}

void loop() {
  int i, k;
  String st = "";
  command = "";

  while(BTSerial.available()){
   temp = BTSerial.read();
   command = command + temp;
   Serial.println(command);
  }
  st = commandcut(); //read the real text
  each_cut(st);

  print_alarm();
  print_time();

  //12시가 되었을 때, 모든 알람의 flag 초기화
  if((myRTC.hours == 0)  && (myRTC.minutes == 0)){
    initAlarm_12();
  }

  //모든 알람 구조체 방문 후 시간 비교 -> 시간 일치하면 알람 울림
  for(k = 0 ; k < 10 ; k ++){
    if(arduinoAlarm[k].empty != 1){ //빈 알람이 아니라면!
      compare_time(k);
    }
  }
}

