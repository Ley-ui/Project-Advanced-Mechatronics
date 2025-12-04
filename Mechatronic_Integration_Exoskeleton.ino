#include <Servo.h>
#include <U8g2lib.h>

//------------- SERVOS ----------------------
Servo Servo_int;
Servo Servo_ext;
String label = "";

//------------- STATES ----------------------
enum State { RESTING, CONTRACTING };
State current_state = RESTING;

//------------ COUNTERS ---------------------
int cpt_motion = 0;
int cpt_rest = 0;

//----------- AUTO MODE ---------------------
int auto_step = 0;
unsigned long auto_last_time=0;

//-------------- OLED -----------------------
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

//------------- POTENTIOMETER ----------------
int potPin = A1;
int potValue = 0;

void setup() {
  Serial.begin(9600);

  //Init servos
  Servo_int.attach(9);
  Servo_ext.attach(7);

  //Init OLED
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  
  Serial.println("Ready. Press 1 to close the hand, and 2 to open it");
}

void loop() {
  potValue = analogRead(potPin);
  Serial.print("Potentiometer =");
  Serial.println(potValue);

  //------------- 3 STATES ----------------
  
  //Maximum : OFF
  if (potValue >  800) {
    current_state = RESTING;
    Servo_int.write(90);
    Servo_ext.write(90);
    display_OLED("Mode : ", "OFF", 2, 12, 30, true);
    Serial.println("OFF");
    return;
  }
  
  //Minimum : AUTO
 else if (potValue < 200){
    display_OLED("Mode", "AUTO", 2, 12, 30, true);
    Serial.println("AUTO");

    //close
    display_OLED("Movement :", "Opening and closing", 2, 45, 55, false);
    Servo_int.write(10);
    Servo_ext.write(0);
    delay(3000);

    //wait before reopening
    Servo_int.write(90);
    Servo_ext.write(90);
    delay(3000);

    //open
    Servo_int.write(180);
    Servo_ext.write(180);
    delay(3000);

    //wait before reclosing
    Servo_int.write(90);
    Servo_ext.write(90);
    delay(5000);
    return;
  }

  //Middle : SERIAL
 // else{
    display_OLED("Mode", "Serial", 2, 12, 30, true);
    Serial.println("SERIAL");

    if (Serial.available()){
      label = Serial.readStringUntil('\n');
      label.trim(); // clean spaces
      Serial.println(label);

        //label = Serial.parseInt();   
        
      if (label == "1") {
        cpt_motion++;
        cpt_rest=0;
      }
      
      else if (label == "0"){
        Serial.println("Resting");
        display_OLED("Movement :", "Resting    ", 2, 45, 55, true);
        cpt_rest++;
        cpt_motion=0;
      }
    }
      
      if(current_state == RESTING && cpt_motion >=2){
        current_state = CONTRACTING;
        cpt_motion=0;
        cpt_rest=0;
      }

     
//------------- CURRENT STATE -------------------
      if(current_state == CONTRACTING){

          Serial.println("Contracting");
          display_OLED("Movement :", "Contracting", 2, 45, 55, false);
        
          //close
          Servo_int.write(10);
          Servo_ext.write(45);
          do_movement_step(2500);
      
          //wait before reopening
          Servo_int.write(90);
          Servo_ext.write(90);
          do_movement_step(3000);
      
          //open
          Servo_int.write(180);
          Servo_ext.write(135);
          do_movement_step(2500);
      
          //wait before reclosing
          Servo_int.write(90);
          Servo_ext.write(90);
          do_movement_step(5000);

          if(cpt_rest >=2){
            current_state = RESTING;
            cpt_motion=0;
            cpt_rest=0;
          }
        }
        else if(current_state == RESTING){
          Serial.println("Stop 2");
          display_OLED("Mode :", "Serial", 2, 12, 30, true);
          display_OLED("Movement :", "Resting", 2, 45, 55, false);
          delay(1000);
        }  
   // } 
}

//-----------Displaying on OLED --------------------
void display_OLED(const char* line1, const char* line2, int x, int y1, int y2, boolean yes) {
  if(yes){
    u8g2.clearBuffer();
  }
  u8g2.setCursor(x, y1);
  u8g2.print(line1);
  u8g2.setCursor(x, y2);
  u8g2.print(line2);
  u8g2.sendBuffer();
}

//---------- NOn blocking wait function ------------
void do_movement_step(unsigned long duration){
  unsigned long start = millis();

  while(millis() - start < duration){

    //Read serial at the same time as moving
    if(Serial.available()){
      label = Serial.readStringUntil('\n');
      label.trim();

      if (label == "1") {
        cpt_motion++;
        cpt_rest=0;
      }
  
      else if (label == "0"){
        Serial.println("Resting");
        display_OLED("Movement :", "Resting    ", 2, 45, 55, true);

        cpt_rest++;
        cpt_motion=0;
      }
    }
    delay(10);
  }
}
