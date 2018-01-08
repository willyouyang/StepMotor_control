#include <Event.h>
#include <Timer.h>

#include <SoftwareSerial.h>

/*
This version is OK!!
by 2018/01/02 00:23
*/

#define PLS 12
#define DIR 11
#define COUNT 10
#define MOTORON 9

#define REVOLUTION 0.9

#define MAXPOS 90
#define MINPOS -90
#define INITPOS 0
#define UNIT 3.3

#define MAXSPEED 1.0
#define MIDDLESPEED 3.0
#define MINSPEED 5.0

SoftwareSerial cellserial(6,7); // (RX,TX)

Timer t;
volatile int timeout=1;
int angle;                  // move steps
int pos = INITPOS;          // record current position
float Speed = MINSPEED;  // motor speed

int type=0;              // 設定進入human_detect的flag
volatile int timerid;    // 用來告訴 pi 馬達有沒有轉，非零值代表馬達在轉
int set = 1;             // 設定馬達狀態不變時只印一次

char tag;          
String str;              // 記錄從手機接收到的字串命令
int s=0;                 // 字串的index,eg:str[s]
int inside=0;              // 設定完跳出迴圈

int point[5];            // 紀錄球道名稱
int mov[5];              // 馬達移動的步數
int count;               // 紀錄投球數
int getin = 0;

void setup(){
  Serial.begin(115200);          // for cellphone
  cellserial.begin(9600);       // for rasperi pi
  //SC.begin(9600);               // for scoreboard

  pinMode(PLS, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(COUNT, INPUT);
  pinMode(MOTORON, OUTPUT);
  randomSeed(analogRead(3));

  digitalWrite(PLS, LOW);
  digitalWrite(MOTORON, LOW);
  
  int k;
  for(k=0;k<5;k++){
    point[k] = 0;
  }

   for(k=0;k<5;k++){
    mov[k] = 0;
  }
}

void loop(){
  //inside=1;
  if(cellserial.available()){
      str = cellserial.readString();
      cellserial.print("getstring:");
      cellserial.println(str);
      inside = 1;
  
  
    while(inside){
      switch(str[s]){
        case 'R':
          cellserial.println("random select on!!");
          Random_select();
          cellserial.println("random select off!!");
          reset();
          inside = 0;
          s=0;
          break;
        case 'T':
          s++;
          if(str[s] == 'O'){
            digitalWrite(MOTORON, HIGH);
            cellserial.println("motor on!!");
          }
          else if(str[s] == 'F'){
            digitalWrite(MOTORON, LOW);
            cellserial.println("motor off!!");
          }
          s=0;
          inside=0;
          break;
        case 'Y':                // 進入機器追蹤模式
          cellserial.println("human_detect start!!");
          //reset();
          type = 1;
          Serial.println(0);     // turn on the camera
          while(type){
            human_detect();
          }
          cellserial.println("human_detect off!!");
          reset();
          s=0;
          inside = 0;
          break;
        case 'P':              // 設定球道順序
          tag = str[s];
          s++;
          motorposition();
          s++;
          break;
        case 'C':            // 設定每個球道的練習球數
          tag = str[s];
          motorcount();
          s++;
          break;
        case 'S':           // 啟動馬達
          tag = str[s];
          cellserial.println("start one loop");
          startmove();
          reset();
          inside = 0;   
          s=0;
          cellserial.println("finished one loop!!");
          break;
        case ';':
          break;
        default:
          cellserial.println("wrong typing!");
          inside = 0;
          s=0;
          break; 
      }
    } // end of while(inside)
    
    t.update();
  }
}

void motor_state(){
  // 取得馬達的狀態(旋轉1 or 靜止0)
  timerid = t.findFreeEventIndex();
    
  if(timerid && set){                
  Serial.println(timerid);
  cellserial.println(timerid);
  set = 0;     
  }
  else if(!timerid && !set ){
    set = 1;
    //Serial.print("from motor_state: ");
    Serial.println(timerid);
    cellserial.println(timerid);
  }
 
}

void speed_set(){
  if(angle <=12 || angle >=-12){
    Speed = MINSPEED;
  }
  else if(angle > 25 || angle <-25){
    Speed = MAXSPEED;
  }
  else{
    Speed = MIDDLESPEED;
  }
}

void reset(){
  angle = 0 - pos;
  
  //Serial.print("move:\t");
  //Serial.println(angle);
  //speed_set();
  Move();
  //Serial.print("position:\t");
  //Serial.println(pos);
  timerid = t.findFreeEventIndex();
  while(timerid){
    //cellserial.print("timerid: "); 
    //cellserial.println(timerid);
    timerid = t.findFreeEventIndex();
    motor_state();
    t.update();
  }

}

// 進入機器跟蹤模式
void human_detect(){    
    // 如果從手機接收到字元'Z'，離開 reactive mode
    if(cellserial.available()){
      String leave;

      leave = cellserial.readString();
      if(leave[0] == 'Z'){
        type = 0;
        Serial.println(1);      // turn off the camera
        //initial = 1;
        //Serial.print("in z: ");
       // Serial.println(initial);
      }
      else if(leave[1] == 'O'){
        cellserial.println("human_detect and turn on");
        digitalWrite(MOTORON,HIGH);
        cellserial.println("motor on!!");
      }
      else if(leave[1] == 'F'){
        cellserial.println("human_detect and turn off");
        digitalWrite(MOTORON,LOW);
        cellserial.println("motor off!!");
      }
    }    
    
    //開始讀取角度值
    if(Serial.available()){
      angle = Serial.parseInt();      // 讀取角度值

      cellserial.print("move:\t");
      cellserial.println(angle);
      //digitalWrite(ENA, HIGH);
      Move();                          // 開始設定timer計數，移動馬達
      cellserial.print("position:\t");
      cellserial.println(pos);
    

    timerid = t.findFreeEventIndex();
    while(timerid){
     // speed_set();
      motor_state(); 
      t.update();                       // update Timer event status
   }
    }
}

void set_time(){
  timeout = 0;
  cellserial.println("go to set");
}

void Random_select(){
  int k;
  int c = 1;
  int timecount;
  timeout = 1;

  timecount = t.after(60000,set_time);
  cellserial.println(timecount);
  while(timeout){
    //motor_state();
    angle = random(-24,24);
    if(ags(angle) <=10){
      angle*=10;
    }
    
    count = 1;
    cellserial.print("angle: ");
    cellserial.println(angle);
    cellserial.print("count: ");
    cellserial.println(count);
    
    updatePos();         // get new pos tokenue
    checkLR();           // check CW or CCW
    Move();

    timerid = t.findFreeEventIndex();
    while(timerid){
      timerid = t.findFreeEventIndex();
      if(timerid == 1){
       break;
      }
      t.update();
    }
    
    while(count){
      if(!timeout){
        break;
      }
      c = digitalRead(COUNT);

      if(c == 0){
        count--;
       // SC.println("7");
        cellserial.println("count!!");
        delay(1000);
      }
    }
    
    t.update();
  } // end of while(timeout) 
  t.stop(timecount);
  timeout=1;
}
// 移動馬達幾步
void Move(){
  int steps;           // 告訴馬達要移動的脈波數
  
  updatePos();         // get new pos tokenue
  checkLR();           // check CW or CCW

  angle *= UNIT;       // change to real motor steps
  steps = abs(angle/REVOLUTION);  // number of pulses
  
  //toggle pin PLS every Speed milliseconds, move steps pulses
  t.oscillate( PLS, Speed, LOW, steps);
  
}

// 更新馬達位置
void updatePos(){
  float prev;          // 記錄前一個位置
  
  prev = pos;          // check previous position
  pos += angle;        // get new pos
  
  // check if pos exceeds the boundary
  if(pos > MAXPOS){
    pos = MAXPOS;
    angle = pos - prev;
  }
  else if( pos < MINPOS){
    pos = MINPOS;
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

// 得到每個球道需要的投球數
void motorcount(){
  int k=0;
  int n;
  count =0;
  cellserial.println("motorCount");////////////////
  s++;
  while (k<2) {
      cellserial.println(str[s]);
      if (tag == 'C') {
        n = str[s] -'0';
        
        if (n>=0 && n<10)
        {
          count = count*10+n;
          k++; 
          s++; 
        }
        else if (str[s] == ';')
        {
          cellserial.println("completed]");
          k=2;
        }
        else 
        {}
      }
      else
      {
        cellserial.println("[Error] Wrong function");
        inside=0;
        s=0;
        break;
      }
  }
  cellserial.println(count);
  
}

// 設定位置順序
void motorposition(){
  int j=0;
  int k;
  
  for (k=0;k<5;k++)
  {
    mov[k] = 0;
  }          
  cellserial.println("[motorPositiong");////////////////

  while (1) {
      cellserial.println(str[s]);
      if (tag == 'P' && str[s] != ';') {
        point[j]=str[s] -'0';
        if (point[j]>=1 && point[j]<=5 && j<=4)
        {
          cellserial.println(point[j]);
          j++;
        }

        //setting turn Left/right
        if (j>=2 && j<=5) {
          mov[j-1] = point[j-1]-point[j-2];
        }  
        else if (j==1)
        {
          mov[j-1] = point[j-1]-3;
        }
        s++;
      }
      else if (str[s] == ';')
      {
        int k;
        for (k=j;k<5;k++) {
          point[k]= 0;
         }
         
        cellserial.println("completed]");

        for (k=0;k<5;k++)
        {
          cellserial.println(point[k]);
        }
        for (k=0;k<5;k++)
        {
          cellserial.print("mov ");
          cellserial.println(mov[k]);
        }
        break;
      }
      else
      {
        cellserial.println("[Error] Wrong function");
        s=0;
        inside=0;
        break;
      }

    
  }//end of while(1)
  
}

void startmove(){  
  int j=0;
  int precount;
  int c=-1; 
  int block = 1;

  precount = count;
    while(j<5){      
      angle = 12*mov[j];
      
      cellserial.print("mov: ");
      cellserial.println(angle);
      speed_set();
      Move();
      cellserial.print("pos: ");
      cellserial.println(pos);
      
      cellserial.println("waiting to read!");
      while(count){
      //馬達停止旋轉，可以進入下一步驟  
      timerid = t.findFreeEventIndex();
      //cellserial.print("timerid: ");
      //cellserial.println(timerid);
      while(timerid){
        //cellserial.println("go into");
        motor_state();
        t.update();
      }
      
        // 運轉的中途可以切換human_detect or 打開傳動馬達開關
        if(cellserial.available()){
          str = cellserial.readString();
          cellserial.print("getstring:");
          cellserial.println(str);

          if(str[1] == 'O'){ //motor power on
            digitalWrite(MOTORON,HIGH);
            cellserial.println("motor on!!");
          }
          else if(str[1] == 'F'){ //motor power off
            digitalWrite(MOTORON,LOW);
            cellserial.println("motor off!!");
          }
         else if(str[0] == 'Y'){ //go in human_detect
            reset();
            Serial.println(0);
            type = 1;
            while(type){
              human_detect();
            }
            reset();
            //j=5;
            block = 0;
            count = 0;
            break;   // leave while(count)
          }
      } // end of while(cellserial.available())
        c = digitalRead(COUNT);
        if(c==0){
          count--;
          //global_count++;
          //SC.println("7");
          cellserial.println("count!!");
          // 延遲一秒，避免從微動開關收到多餘的訊號
          delay(1000);
        } 
      }// end of while(count) 

        // if you have gotton into human_detect mode
        if(!block){
          break;   //leave while(j<5)
        }
        
        cellserial.println("count finished!!");
        count = precount;
        j++;
        // 延遲3秒，讓使用者可以投完
        delay(3000);
        
        t.update();
      } // end of while(j<5)
}

