/* The life assitance v1. face with eye blink
   Display humidity, tempreture and time.
   Time and alarm can be setup.
*/
#include <Timer.h>
#include <Time.h>
#include <EEPROM.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h> //LCD with i2c pack library
#include <IRremote.h>//IR remote library
#include "DHT.h"// humidity and tempreture sensor library

// pins setup
#define DHTPIN 8     // what digital pin we're connected to
#define DHTTYPE DHT11  // DHT type
int Remot_RECV_PIN = 9; //Pin 9 for IR remote receiver
int buzzerPin = 5; //Define buzzerPin
const int SW_pin = 7; // digital pin connected to switch output of joystick
const int X_pin = A0; // analog pin connected to X output of joystick
const int Y_pin = A1; // analog pin connected to Y output of joystick

// variables
char lcdbacklight = 1; // flag of lcd back light
float humi = 0, temp = 0; // Read humidity and temperature as Celsius (the default)
int addr = 0; // initial EEPROM adress

// classes and objects
IRrecv irrecv(Remot_RECV_PIN);// IR remote class
decode_results Remote_addr;// IR key address
DHT dht(DHTPIN, DHTTYPE);// initialise the dht humi and temp sensor
Timer t;
TimeElements tm;
TimeElements tm_alarm;
time_t t1;

// Custom characters for LCD
uint8_t empty[8]  = {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1};
uint8_t eyebrow_happy[8]  = {0x0,0x0,0x0,0xe,0x11,0x0,0x0,0x0};
uint8_t eyebrow_angry_right[8]  = {0x0,0x0,0x0,0x10,0x8,0x4,0x2,0x1};
uint8_t eyebrow_close[8]  = {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1f};
uint8_t eyebrow_angry_left[8]  = {0x0,0x0,0x0,0x1,0x2,0x4,0x8,0x10};
uint8_t eye_left[8] = {0xe,0x11,0x17,0x17,0x17,0x17,0x11,0xe};
uint8_t eye_right[8] = {0xe,0x11,0x1d,0x1d,0x1d,0x1d,0x11,0xe};
uint8_t eye_up[8] = {0xe,0x15,0x15,0x15,0x15,0x11,0x11,0xe};
uint8_t eye_down[8] = {0xe,0x11,0x11,0x15,0x15,0x15,0x15,0xe};
uint8_t eye_right_up[8] = {0xe,0x1d,0x1d,0x1d,0x1d,0x11,0x11,0xe};
uint8_t eye_right_down[8] = {0xe,0x11,0x11,0x1d,0x1d,0x1d,0x1d,0xe};
uint8_t eye_left_up[8] = {0xe,0x17,0x17,0x17,0x17,0x11,0x11,0xe};
uint8_t eye_left_down[8] = {0xe,0x11,0x11,0x17,0x17,0x17,0x17,0xe};
uint8_t eye_front[8] = {0xe,0x11,0x15,0x15,0x15,0x15,0x11,0xe};
uint8_t eye_close[8] = {0x0,0x0,0x0,0x11,0x1f,0x0,0x0,0x0};
uint8_t mouth_happy[8]  = {0x4,0xe,0xe,0x0,0x0,0x11,0xe};
uint8_t mouth_sad[8] = {0x4,0xe,0xe,0x0,0x0,0xe,0x11};
uint8_t degrees[8] = {0x6,0x9,0x9,0x6,0x0,0x0,0x0};
LiquidCrystal_I2C lcd(0x3f, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
// Different states for control and display
enum EYE_STATE{EYE_LEFT,EYE_RIGHT,EYE_UP,EYE_DOWN,EYE_CENTER};
enum LCD_STATE{NORMAL, TIMESET,ALARM};
enum ALARM_STATE{START_AL,HOUR10_AL,HOUR_AL,MIN10_AL,MIN_AL,END_AL};
enum TIMESET_STATE{START,HOUR10,HOUR,MIN10,MIN,SEC10,SEC,YEAR1000,YEAR100,YEAR10,YEAR,MONTH10,MONTH,DAY10,DAY,END};


void setup() {
    EEPROM.get(addr, t1);//get time from EEPROM
    setTime(t1);// initialize time
    
    lcd.begin(20,4); 
    // the lcd createchar can not be set up more than NO. 8 together.
    lcd.createChar(0, empty);
    lcd.createChar(1, eyebrow_happy);
    lcd.createChar(2, eye_left);
    lcd.createChar(3, mouth_happy);
    lcd.createChar(4, eye_close);
    lcd.createChar(5, eyebrow_close);
    lcd.createChar(6, degrees);
    lcd.createChar(7, eye_up);
    lcd.createChar(8, eye_down);
    
    // eyebrow
    lcd.setCursor(1,0);
    lcd.write(1);
    lcd.setCursor(3,0);
    lcd.write(1);
    // eyes
    eye_lookleft();
    // mouth
    lcd.setCursor(2,2);
    lcd.write(3);
    // words
    lcd.setCursor(5,0);
    lcd.print("Hi, I am Lily!");
    lcd.setCursor(5,1);
    lcd.print("Your life");
    lcd.setCursor(5,2);
    lcd.print("Assistance.");
    
    pinMode(buzzerPin, OUTPUT); //Set buzzerPin as output
    analogWrite(buzzerPin, LOW);
    t.every(3000,eyeblink);//set timer for blink eyes
    t.every(2000,Humi_temp);
    irrecv.enableIRIn();// IR remote receiver enable
    dht.begin();
    pinMode(SW_pin, INPUT);// setup joysticker switch to input
    digitalWrite(SW_pin, HIGH);// setup joysticker switch to high
    
    delay(3000);
    lcd.clear();
}


//string str[20];
void loop() {
  static String char_remote = "nothing"; // IR remote character
  static bool switch_flag = false;// swith using joystick or IR remote
  static bool new_clockset = true;// flag for new time setup
  static bool new_alarmset = true;// flag for new alarm setup
  static int lcd_state = NORMAL;
  //Get the first value of humidity and temprature
  humi = dht.readHumidity();
  temp = dht.readTemperature();
  t.update();  //update timer
 
  // check joysticker button
  if (!digitalRead(SW_pin)){
    delay(250);
    switch_flag = !switch_flag;
  }
  // get the IR remote result
  if (irrecv.decode(&Remote_addr)) {
    char_remote = IR_key(char_remote);
    irrecv.resume(); // Receive the next value
  }
  
  // check remote string for lcd state
  if(char_remote == "ch+"){
    lcd_state += 1;
    // if lcd sate change, the mode setup is made.
    new_clockset = true;
    new_alarmset = true;
    if(lcd_state >=3){
      lcd_state -= 1;
    }
   // char_remote = "nothing";
  }
  else if (char_remote == "ch-"){
    lcd_state -= 1;
    // if lcd sate change, the mode setup is made.
    new_clockset = true;
    new_alarmset = true;
    if(lcd_state < 0){
      lcd_state = 0;
    }
  //  char_remote = "nothing";
  }
    
  switch(lcd_state){
    case NORMAL: 
         normal_display(char_remote);
         break;
    case TIMESET:
         lcd.setCursor(5,0); 
         lcd.print("Setup time:     ");
         lcd.setCursor(5,1);
         lcd.print("                ");
         time_set_fun(lcd_state, char_remote, new_clockset);
         new_clockset = false;
         break;
    case ALARM:
         lcd.setCursor(5,0); 
         lcd.print("Setup alarm:   ");
         lcd.setCursor(5,1);
         lcd.print("                ");
         alarm_set_fun(lcd_state, char_remote, new_alarmset);
         new_alarmset = false;// give new alarm setup flag, then set to flase
         break;
   // default: break; 
  }  
 
  face_display(switch_flag, char_remote);
  char_remote = "nothing";  
}
// Fuction for alarm setup
void alarm_set_fun(int &lcd_state, String char_remote, bool new_alarmset){
  static int alarmset_state = START_AL;
  static int H_prev = 0, M_prev = 0;// previous alarm store
  if(new_alarmset) alarmset_state = START_AL;
  switch(alarmset_state){
    case START_AL:
         tm_alarm.Hour = H_prev;
         tm_alarm.Minute = M_prev;
         alarm_set_display(alarmset_state);
         alarmset_state = HOUR10_AL;break;
    case HOUR10_AL:
         if(char_remote != "nothing"){
           if(char_remote == "EQ" && char_remote == "ch+" && char_remote == "ch-"){ alarmset_state = END_AL;}
           else if(char_remote == "right"){ alarmset_state++;}
           else {
             tm_alarm.Hour = tm_alarm.Hour%10 + (char_remote.toInt()%10)*10;
             alarmset_state = HOUR_AL;
             
           }
         }
         alarm_set_display(alarmset_state);
         break;
    case HOUR_AL:
         if(char_remote != "nothing"){
           if(char_remote == "EQ"){ alarmset_state = END_AL;}
           else if(char_remote == "right"){ alarmset_state++;}
           else {
             tm_alarm.Hour = int(tm_alarm.Hour/10)*10 + char_remote.toInt()%10;
             alarmset_state = MIN10_AL;
             
           }
         }
         alarm_set_display(alarmset_state);
         break;
    case MIN10_AL:
         if(char_remote != "nothing"){
           if(char_remote == "EQ"){ alarmset_state = END_AL;}
           else if(char_remote == "right"){ alarmset_state++;}
           else {
             tm_alarm.Minute = tm_alarm.Minute%10 + (char_remote.toInt()%10)*10;
             alarmset_state = MIN_AL;
             
           }
         }
         alarm_set_display(alarmset_state);
         break;
    case MIN_AL:
         if(char_remote != "nothing"){
           if(char_remote == "EQ"){ alarmset_state = END_AL;}
           else if(char_remote == "right"){ alarmset_state++;}
           else {
             tm_alarm.Minute = int(tm_alarm.Minute/10)*10 + char_remote.toInt()%10;
             alarmset_state = END_AL;
             
           }
         }
         alarm_set_display(alarmset_state);
         break;
    case END_AL:
         tm_alarm.Second = 0;
         H_prev = tm_alarm.Hour;
         M_prev = tm_alarm.Minute;
         lcd_state = NORMAL;
         alarmset_state = START_AL;
         break;
  }//end of switch
}
// alarm setup display
void alarm_set_display(int alarmset_state){
  lcd.setCursor(5,2);
  lcd.print(int(tm_alarm.Hour/10));
  lcd.print(tm_alarm.Hour%10);
  lcd.print(":");
  lcd.print(int(tm_alarm.Minute/10));
  lcd.print(tm_alarm.Minute%10);
  
  delay(250);
  if (alarmset_state <= 6){
    if(alarmset_state <= 2){lcd.setCursor((4+alarmset_state),2);}
    else if(alarmset_state <= 4){lcd.setCursor((5+alarmset_state),2);}
    lcd.print(" ");
  }
  delay(250);
}
// Function for clock setup
void time_set_fun(int &lcd_state, String char_remote, bool new_clockset){
    static int timeset_state = START;
    static int year1 = 2018; // for year setup
    if(new_clockset) timeset_state = START;
  switch(timeset_state){
    case START:
         t1 = now();
         tm.Year = year(t1);
         tm.Month = month(t1);
         tm.Day = day(t1);
         tm.Hour = hour(t1);
         tm.Minute = minute(t1);
         tm.Second = second(t1);
         year1 = 2018;
         time_set_display(year1,timeset_state);
         timeset_state = HOUR10;break; // no break, directly to the HOUR10 state         
    case HOUR10:    
         if(char_remote != "nothing"){
           if(char_remote == "EQ" && char_remote == "ch+" && char_remote == "ch-"){ timeset_state = END;}
           else if(char_remote == "right"){ timeset_state++;}
           else {
             tm.Hour = tm.Hour%10 + (char_remote.toInt()%10)*10;
             timeset_state = HOUR;mmmm
             
           }
         }
         time_set_display(year1, timeset_state);
         break;
    case HOUR:
         if(char_remote != "nothing"){ 
          
           if(char_remote == "EQ"){timeset_state = END;}
           else if(char_remote == "left"){ timeset_state--;}
           else if(char_remote == "right"){ timeset_state++;}
           else {
             tm.Hour = int(tm.Hour/10)*10 + char_remote.toInt()%10;
             timeset_state = MIN10;
           }
         }
         time_set_display(year1, timeset_state);
         break;
    case MIN10:
         if(char_remote != "nothing"){ 
          
           if(char_remote == "EQ"){ timeset_state = END; }
           else if(char_remote == "left"){ timeset_state--;}
           else if(char_remote == "right"){ timeset_state++;}
           else {
             tm.Minute = tm.Minute%10 + (char_remote.toInt()%10)*10;
             timeset_state = MIN;
           }
         }
         time_set_display(year1, timeset_state);
         break;
    case MIN:
         if(char_remote != "nothing"){ 
           if(char_remote == "EQ"){ timeset_state = END;}
           else if(char_remote == "left"){ timeset_state--;}
           else if(char_remote == "right"){ timeset_state++;}
           else {
             tm.Minute = int(tm.Minute/10)*10 + char_remote.toInt()%10;
             timeset_state = SEC10;
           }
         }
         time_set_display(year1, timeset_state);
         break;
    case SEC10:
         if(char_remote != "nothing"){ 
          
           if(char_remote == "EQ"){timeset_state = END;}
           else if(char_remote == "left"){ timeset_state--;}
           else if(char_remote == "right"){ timeset_state++;}
           else {
             tm.Second = tm.Second%10 + (char_remote.toInt()%10)*10;
             timeset_state = SEC;
           }
         }
         time_set_display(year1, timeset_state);
         break;
    case SEC:
         if(char_remote != "nothing"){ 
           if(char_remote == "EQ"){timeset_state = END;}
           else if(char_remote == "left"){ timeset_state--;}
           else if(char_remote == "right"){ timeset_state++;}
           else {
             tm.Second = int(tm.Second/10)*10 + char_remote.toInt()%10;
             timeset_state = YEAR1000;
           }
         }
         time_set_display(year1, timeset_state);
         break;
    case YEAR1000:
         if(char_remote != "nothing"){ 
          
           if(char_remote == "EQ"){ timeset_state = END;}
           else if(char_remote == "left"){ timeset_state--;}
           else if(char_remote == "right"){ timeset_state++;}
           else {
             
             year1 = year1%1000 + (char_remote.toInt()%10)*1000;
             //tm.Year = year1 -1970;
             timeset_state = YEAR100;
           }
         }
         time_set_display(year1, timeset_state);
         break;
    case YEAR100:
         if(char_remote != "nothing"){ 
          
           if(char_remote == "EQ"){ timeset_state = END;}
           else if(char_remote == "left"){ timeset_state--;}
           else if(char_remote == "right"){ timeset_state++;}
           else {
             
             year1 = int(year1/1000)*1000 + year1%100 + (char_remote.toInt()%10)*100;
             //tm.Year = year1 -1970;
             timeset_state = YEAR10;
           }
         }
         time_set_display(year1, timeset_state);
         break;
    case YEAR10:
         if(char_remote != "nothing"){ 
          
           if(char_remote == "EQ"){ timeset_state = END;}
           else if(char_remote == "left"){ timeset_state--;}
           else if(char_remote == "right"){ timeset_state++;}
           else {
             
             year1 = int(year1/100)*100 + year1%10 + (char_remote.toInt()%10)*10;
             //tm.Year = year1 -1970;
             timeset_state = YEAR;
           }
         }
         time_set_display(year1, timeset_state);
         break;
    case YEAR: 
         if(char_remote != "nothing"){ 
          
           if(char_remote == "EQ"){ timeset_state = END;}
           else if(char_remote == "left"){ timeset_state--;}
           else if(char_remote == "right"){ timeset_state++;}
           else {
             
             year1 = int(year1/10)*10 + (char_remote.toInt()%10);
             //tm.Year = year1 -1970;
             timeset_state = MONTH10;
           }
         }
         time_set_display(year1, timeset_state);
         break;
    case MONTH10:
         if(char_remote != "nothing"){ 
          
           if(char_remote == "EQ"){ timeset_state = END;}
           else if(char_remote == "left"){ timeset_state--;}
           else if(char_remote == "right"){ timeset_state++;}
           else {
             
             tm.Month = tm.Month%10 + (char_remote.toInt()%10)*10;             
             timeset_state = MONTH;
           }
         }
         time_set_display(year1, timeset_state);
         break;
    case MONTH:
         if(char_remote != "nothing"){ 
          
           if(char_remote == "EQ"){ timeset_state = END;}
           else if(char_remote == "left"){ timeset_state--;}
           else if(char_remote == "right"){ timeset_state++;}
           else {             
             tm.Month = int(tm.Month/10)*10 + (char_remote.toInt()%10);             
             timeset_state = DAY10;
           }
         }
         time_set_display(year1, timeset_state);
         break;
    case DAY10:
         if(char_remote != "nothing"){ 
          
           if(char_remote == "EQ"){ timeset_state = END;}
           else if(char_remote == "left"){ timeset_state--;}
           else if(char_remote == "right"){ timeset_state++;}
           else {             
             tm.Day = tm.Day%10 + (char_remote.toInt()%10)*10;             
             timeset_state = DAY;
           }
         }
         time_set_display(year1, timeset_state);
         break;
    case DAY:
         if(char_remote != "nothing"){ 
          
           if(char_remote == "EQ"){ timeset_state = END;}
           else if(char_remote == "left"){ timeset_state--;}
           else if(char_remote == "right"){ timeset_state++;}
           else {             
             tm.Day = int(tm.Day/10)*10 + (char_remote.toInt()%10);             
             timeset_state = END;
             
           }
         }
         time_set_display(year1, timeset_state);
         break;
    case END:
         tm.Year = year1 - 1970;
         t1 = makeTime(tm);
         setTime(t1);
         timeset_state = START;
         lcd_state = NORMAL; // back to normal timer display
         //delay(1000);
         break;
 }// end of switch
}// end function
// clock setup display
void time_set_display(int year, int timeset_state){
  lcd.setCursor(5,2);
  lcd.print(int(tm.Hour/10));
  lcd.print(tm.Hour%10);
  lcd.print(":");
  lcd.print(int(tm.Minute/10));
  lcd.print(tm.Minute%10);
  lcd.print(":");
  lcd.print(int(tm.Second/10));
  lcd.print(tm.Second%10);
  lcd.setCursor(5,3);
  lcd.print(int(year/1000));
  lcd.print(int((year%1000)/100));
  lcd.print(int((year%100/10)));
  lcd.print(year%10);
  lcd.print(" ");
  lcd.print(int(tm.Month/10));
  lcd.print(tm.Month%10);
  lcd.print(" ");
  lcd.print(int(tm.Day/10));
  lcd.print(tm.Day%10);  
  
  delay(250);
  if (timeset_state <= 6){
    if(timeset_state <= 2){lcd.setCursor((4+timeset_state),2);}
    else if(timeset_state <= 4){lcd.setCursor((5+timeset_state),2);}
    else if(timeset_state <= 6){lcd.setCursor((6+timeset_state),2);}
    lcd.print(" ");
  }
  else {
    if(timeset_state <= 10){ lcd.setCursor((timeset_state-2),3);}
    else if(timeset_state <= 12){ lcd.setCursor((timeset_state-1),3);}
    else if(timeset_state <= 14){ lcd.setCursor((timeset_state),3);}
    lcd.print(" ");
  }
  delay(250);
}

// normal display
void normal_display(String char_remote){
  static int alarm_flag = 0;
  t1 = now();// get current time  
  int current_hour = hour(t1);
  int current_min = minute(t1);
  int current_sec = second(t1);
  
  if(char_remote == "play"){
    alarm_flag = 0;
    analogWrite(buzzerPin, 0);
    /*lcd.setCursor(5,1); 
    lcd.print("                ");//clean the line;*/
  }  
  // display lcd state
  Humi_temp_display();
  /*lcd.setCursor(5,0); 
  lcd.print("Clock time:");*/
  
  if((tm_alarm.Hour == current_hour) && (tm_alarm.Minute == current_min) && (tm_alarm.Second == current_sec)){
    alarm_flag = 1;
    /*lcd.setCursor(5,1); 
    lcd.print("Time to get up!");*/
    }
    if (alarm_flag && current_sec%2){analogWrite(buzzerPin, 200); }
    else {analogWrite(buzzerPin, LOW);}
    //Display time
    lcd.setCursor(5,2);         
    lcd.print(int(current_hour/10));
    lcd.print(current_hour%10);
    lcd.print(":");
    lcd.print(int(current_min/10));
    lcd.print(current_min%10);
    lcd.print(":");
    lcd.print(int(current_sec/10));
    lcd.print(current_sec%10);
    lcd.print("  ");
    lcd.setCursor(5,3);
    lcd.print(year(t1));
    lcd.print(" ");
    lcd.print(int(month(t1)/10));
    lcd.print(month(t1)%10);
    lcd.print(" ");
    lcd.print(int(day(t1)/10));
    lcd.print(day(t1)%10);
    lcd.print(" ");
    EEPROM.put(addr, t1);// store the time;
}

// display face on LCD 
void face_display(bool switch_flag, String char_remote){
     static int eye_state = EYE_LEFT;
     int x_pin, y_pin;
    if(switch_flag){
      x_pin = analogRead(X_pin);
      y_pin = analogRead(Y_pin);
      /*lcd.setCursor(5,3);
      lcd.print("x:");
      lcd.print(x_pin);
      lcd.print(", y:");
      lcd.print(y_pin);*/
      if((407<x_pin && x_pin<607) && (398<y_pin && y_pin<598)){
        eye_lookfront();
      }
      else if((0<=x_pin && x_pin<100) && (398<y_pin && y_pin<598)){
        eye_lookup();
      }
      else if((900<x_pin && x_pin<=1023) && (398<y_pin && y_pin<598)){
        eye_lookdown();
      }
      else if((407<x_pin && x_pin<607) && (0<=y_pin && y_pin<100)){
        eye_lookleft();
      }
      else if((407<x_pin && x_pin<607) && (900<y_pin && y_pin<=1023)){
        eye_lookright();
      }
      else if((0<=x_pin && x_pin<100) && (120<y_pin && y_pin<398)){
        eye_lookleftup();
      }
      else if((900<x_pin && x_pin<=1023) && (120<y_pin && y_pin<398)){
        eye_lookleftdown();
      }
      else if((0<=x_pin && x_pin<300) && (900<y_pin && y_pin<=1023)){
        eye_lookrightup();
      }
      else if((800<x_pin && x_pin<=1023) && (900<y_pin && y_pin<=1023)){
        eye_lookrightdown();
      }
    }
    else { // switch_flag = 0
      
      if(char_remote == "left"){ eye_state = EYE_RIGHT; }
      else if(char_remote == "right"){ eye_state = EYE_LEFT; }
      else if(char_remote == "up"){ eye_state = EYE_UP; }
      else if(char_remote == "down"){ eye_state = EYE_DOWN; }
      else if(char_remote == "center"){ eye_state = EYE_CENTER; }
      else if(char_remote == "switch"){
        // turn on or off the lcd backlight
        lcdbacklight = !lcdbacklight;
        if(lcdbacklight !=0){
           lcd.backlight();
        }
        else {
           lcd.noBacklight();
        }
      }
      
      switch(eye_state){
        case EYE_LEFT: eye_lookleft();break;
        case EYE_RIGHT: eye_lookright();break;
        case EYE_UP: eye_lookleftup();break;
        case EYE_DOWN: eye_lookleftdown(); break;
        case EYE_CENTER: eye_lookfront();break;
      }
    }
      // eyebrow
    lcd.setCursor(1,0);
    lcd.write(1);
    lcd.setCursor(3,0);
    lcd.write(1);
     // mouth
    lcd.setCursor(2,2);
    lcd.write(3);
}

void Humi_temp_display(){
 // display the humidity and temprature
    lcd.setCursor(5,0);
    lcd.print("Humi: ");
    lcd.print(humi);
    lcd.print("%");
    lcd.setCursor(5,1);
    lcd.print("Temp: ");
    lcd.print(temp);
    lcd.setCursor(16,1);
    lcd.createChar(6, degrees);
    lcd.setCursor(16,1);
    lcd.write(6);
    lcd.print("C");
}
// get humidity and tempreture
void Humi_temp(){
    humi = dht.readHumidity();
    temp = dht.readTemperature();
    if(isnan(humi)) {
     humi = 0;
    }
    else if(isnan(humi)){
      temp = 0;
    }
  
}
// blink eyes
void eyeblink(){
   // blink eyes
   // eyebrow close
    lcd.setCursor(1,0);
    lcd.write(5);
    lcd.setCursor(3,0);
    lcd.write(5);
    // eyes close
    lcd.setCursor(1,1);
    lcd.write(4);
    lcd.setCursor(3,1);
    lcd.write(4);
    delay(250);
   
}
// IR remote address translation
String IR_key(String char_remote){
  String char_rmt;
  switch(Remote_addr.value){
    case 0x61D648B7: char_rmt = "switch"; break;// from here is the first remote
    case 0x61D6D827: char_rmt = "up"; break;
    case 0x61D658A7: char_rmt = "down"; break;
    case 0x61D620DF: char_rmt = "left"; break;
    case 0x61D6609F: char_rmt = "right"; break;
    case 0x61D6A05F: char_rmt = "center"; break;
    case 0xC7138F7F: char_rmt = "center"; break;// one remote last buttom
    case 0xFF6897: char_rmt = "0"; break;//from here this is another remote
    case 0xFF30CF: char_rmt = "1"; break;
    case 0xFF18E7: char_rmt = "2"; break;
    case 0xFF7A85: char_rmt = "3"; break;
    case 0xFF10EF: char_rmt = "4"; break;
    case 0xFF38C7: char_rmt = "5"; break;
    case 0xFF5AA5: char_rmt = "6"; break;
    case 0xFF42BD: char_rmt = "7"; break;
    case 0xFF4AB5: char_rmt = "8"; break;
    case 0xFF52AD: char_rmt = "9"; break;
    case 0xFF9867: char_rmt = "100+"; break;
    case 0xFFB04F: char_rmt = "200+"; break;
    case 0xFFE01F: char_rmt = "-"; break;
    case 0xFFA857: char_rmt = "+"; break;
    case 0xFF906F: char_rmt = "EQ"; break;
    case 0xFF22DD: char_rmt = "prev"; break;
    case 0xFF02FD: char_rmt = "next"; break;
    case 0xFFC23D: char_rmt = "play"; break;
    case 0xFFA25D: char_rmt = "ch-"; break;
    case 0xFF629D: char_rmt = "ch"; break;
    case 0xFFE21D: char_rmt = "ch+"; break;// another remote last buttom
    //case 0xFFFFFFFF: char_rmt = char_remote; break;
    default:  char_rmt = char_remote; break; //char_rmt = "nothing";
  }
  delay(200);
  return char_rmt;
}

// Functions for eye behaviours
void eye_lookleft(){

  lcd.createChar(2, eye_left);
  lcd.setCursor(1,1);
  lcd.write(2);
  lcd.setCursor(3,1);
  lcd.write(2);

}
void eye_lookright(){
  lcd.createChar(2, eye_right);
  lcd.setCursor(1,1);
  lcd.write(2);
  lcd.setCursor(3,1);
  lcd.write(2);
}
void eye_lookup(){
  lcd.createChar(2, eye_up);
  lcd.setCursor(1,1);
  lcd.write(2);
  lcd.setCursor(3,1);
  lcd.write(2);
}
void eye_lookleftup(){
  lcd.createChar(2, eye_left_up);
  lcd.setCursor(1,1);
  lcd.write(2);
  lcd.setCursor(3,1);
  lcd.write(2);
}
void eye_lookrightup(){
  lcd.createChar(2, eye_right_up);
  lcd.setCursor(1,1);
  lcd.write(2);
  lcd.setCursor(3,1);
  lcd.write(2);
}
void eye_lookdown(){
  lcd.createChar(2, eye_down);
  lcd.setCursor(1,1);
  lcd.write(2);
  lcd.setCursor(3,1);
  lcd.write(2);
}
void eye_lookleftdown(){
  lcd.createChar(2, eye_left_down);
  lcd.setCursor(1,1);
  lcd.write(2);
  lcd.setCursor(3,1);
  lcd.write(2);
}
void eye_lookrightdown(){
  lcd.createChar(2, eye_right_down);
  lcd.setCursor(1,1);
  lcd.write(2);
  lcd.setCursor(3,1);
  lcd.write(2);
}
void eye_lookfront(){
 
  lcd.createChar(2, eye_front);
  lcd.setCursor(1,1);
  lcd.write(2);
  lcd.setCursor(3,1);
  lcd.write(2);
}