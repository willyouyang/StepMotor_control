#include <Event.h>
#include <Timer.h>

#define PLS 12
#define DIR 11

#define REVOLUTION 0.9

#define MAXPOS 180
#define INITPOS 90
#define UNIT 3.3

#define MAXSPEED 1.6
#define MIDDLESPEED 3.0
#define MINSPEED 5.0

Timer t;
float angle;             // move steps
float pos = INITPOS;     // record current position
float Speed = MINSPEED;   // min cycle 1 milliseconds means max frequency 1000HZ
int flag=1;          // chose mode

void setup() {
  Serial.begin(9600);
	pinMode(PLS, OUTPUT);
	pinMode(DIR, OUTPUT);
 digitalWrite(PLS, LOW);
 
}

void loop() {
 // select mode
 if(flag){
  mode();
  Serial.println("hello");
 }
 // motor work
 else{
  if(Serial.available()){
    
    angle = Serial.parseFloat();
    Serial.print("move:\t");
    Serial.println(angle);
    Move();
    Serial.print("position\t");
    Serial.println(pos);
  }
 
  t.update();              // update Timer event status
 }
 
}

int mode(){
  char mode;
  int state;
  if(Serial.available()){
    mode = Serial.read();
    if(mode == 'a'){ 
      state = 1;
      Serial.println("Free mode!!");
    }
    flag = 0;                // finishing choosing mode
  }
 
  return state;
}

void Move(){
  int steps;
  
  updatePos();         // get new pos value
  checkLR();           //check CW or CCW

  angle *= UNIT;      // change to real motor steps
  steps = abs(angle/REVOLUTION);  // number of pulses
  //toggle pin PLS every Speed milliseconds, move steps pulses
  t.oscillate( PLS, Speed, LOW, steps);
}

void updatePos(){
  float prev;
  
  prev = pos;          // check previous position
  pos += angle;        // get new pos
  
  // check if pos exceeds the boundary
  if(pos > MAXPOS){
    pos = MAXPOS;
    angle = pos - prev;
  }
  else if( pos < 0){
    pos = 0;
    angle = pos - prev; 
  }
}

void checkLR(){
  if( angle < 0){
    digitalWrite( DIR, LOW);
  }
  else{
    digitalWrite( DIR, HIGH);
  }
}

