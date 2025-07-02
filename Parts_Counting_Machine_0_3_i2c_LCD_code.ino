#include <LCD5110_Graph.h>
#include <Stepper.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C  lcd(0x3F,2,1,0,4,5,6,7); // 
/*
If you have the PCF8574T the default I2C bus address is 0x27. 
If you have the PCF8574AT the default I2C bus address is 0x3F
SCL is A5
SDA is A4
*/

int CLK = 2;//CLK->D2
int DT = 3;//DT->D3
int SW = 4;//
const int interrupt0 = 0;// Interrupt 0 = pin 2 
int encount = 0;//Define the count
int lastCLK = 0;//CLK initial value

const int stepsPerRevolution = 200;
Stepper myStepper(200, A0,A1,A2,A3); 




int stoplimit = 0;
int motorPower = 9;
int startButton = 7;
int indexPower=  8;
int indexPosition=1; //index == cup position, human counts AKA start counting at #"1"


// this constant won't change:
const int  countTrigger = 5;    // the pin counts small parts
const int ledPin = 13;       // the pin that the LED is attached to
const int modeButton = 6;

// Variables will change:
int counter = 0;   // counter for the number of button presses
int buttonState = 0;         // current state of the button
int lastButtonState = 0;     // previous state of the button

void setup() {
  //encoder stuff
  pinMode(SW, INPUT_PULLUP);
  digitalWrite(SW, HIGH);
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  attachInterrupt(interrupt0, ClockChanged, CHANGE);//Set the interrupt 0 handler, trigger level change

  lcd.begin (20,4); // for 16 x 2 LCD module
  lcd.setBacklightPin(3,POSITIVE);
  lcd.setBacklight(HIGH);
  myStepper.setSpeed(60);

  pinMode(modeButton, INPUT_PULLUP);
  pinMode(countTrigger, INPUT_PULLUP);
  pinMode(motorPower, OUTPUT); // to relay
  pinMode(indexPower, OUTPUT); // to relay
  pinMode(startButton, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
 

   digitalWrite(motorPower, HIGH); // turn off motor power  
   digitalWrite(indexPower, HIGH);// turn off power to indexing motor
   lcd.setCursor(0,0); lcd.print("Parts Counter v2");
   lcd.setCursor(0,1); lcd.print("(c) 2018 Ryan Bates");
   delay(1500); lcd.clear();
}

//==============MASTER LOOP=========================================================================
void loop() {


//==================== Mode Check: INVENTORY=========================
 if ((digitalRead(modeButton)==LOW) && indexPosition <8){
  lcd.clear(); lcd.setCursor(0,0); lcd.print("Mode: Inventory");
  while ((digitalRead(modeButton)==LOW)){
    lcd.setCursor(0,1); lcd.print("COUNT=");
    count();
    countblink();
    digitalWrite(motorPower, LOW);

    
    
    if (digitalRead(SW)==LOW) {
       lcd.clear(); 
      lcd.setCursor(0,0); lcd.print("Mode: Inventory");
      
      digitalWrite(indexPower, LOW);// move index  
      delay(50); //wait for relay
      myStepper.step(200);
      delay(50);        
      digitalWrite(indexPower, HIGH);   //power off
      
     // int oldcount;
   // counter = oldcount;
    lcd.setCursor(0,2); lcd.print("Previous="); lcd.print(counter);
    counter=0; //reset counter when encoder switch pressed
      }   
  }
 }
 
//==================== Mode Check: INDEX SET COUNT=========================
  
 if ((digitalRead(modeButton)==HIGH)){
  lcd.setCursor(0,0); lcd.print("Mode: Index Count");
  lcd.setCursor(0,1); lcd.print("Turn knob to adjust");
  lcd.setCursor(0,2); lcd.print("Set Limit:");
  lcd.setCursor(8,3); lcd.print("start >>>");
   digitalWrite(motorPower, HIGH); // turn off motor power  
  if (!digitalRead(SW) && encount != 0){ //Read the button press and set count value to 0 when the counter reset
    lcd.clear();
    encount = 0;
    
    //encount = encount/4;
    lcd.print("encount:");
    lcd.print(encount);                  }
    
encount = constrain(encount, 0, 100); 
lcd.setCursor(11,2); leadingZero(); lcd.print(encount);
 }
  
  
   stoplimit = encount; //set count limit to encoder counter value
   buttonState = digitalRead(countTrigger);   //no idea why this is here
 




 //====================wait for start button============================================
  if ((digitalRead(startButton)==LOW) && (indexPosition <=8)){ //if Start is enables and index is less than 8
     counter=0; //reset parts counter to 0
    digitalWrite(motorPower, LOW);
    
   LCDclear();
    lcd.setCursor(0,0); lcd.print("Busy");
    lcd.setCursor(7,0); lcd.print("Counting...");
    lcd.setCursor(0,1); lcd.print("COUNT="); lcd.setCursor(12,1); lcd.print(counter);
    lcd.setCursor(12,1); lcd.print("/"); lcd.print(stoplimit); //print limit counter as " __/#limit"
    lcd.setCursor(0,2);  lcd.print("Index pos:"); lcd.print(indexPosition); lcd.print("/8");
    delay(50); 
    
  // after Start, count every time the beam breaks until the limit is reached  
  while (counter < stoplimit){
   count();
   countblink(); }
   
   digitalWrite(motorPower, HIGH);//stop motor 
   LCDclear();
   lcd.setCursor(0,0); lcd.print("Limit Active! "); 
   lcd.setCursor(0,1); lcd.print("Stopped @:"); lcd.setCursor(11,1); lcd.print(counter);
   lcd.setCursor(12,1); lcd.print("/"); lcd.print(stoplimit); //print limit counter as " __/#limit"
   lcd.setCursor(0,2); lcd.print("Index pos:"); lcd.print(indexPosition); lcd.print("/8");
   lcd.setCursor(0,3); lcd.print("Moving index...    "); 
   
   digitalWrite(indexPower, LOW);// move index  
   delay(50); //wait for relay
   myStepper.step(200);
   delay(50); indexPosition++;       
   digitalWrite(indexPower, HIGH);   //power off
  }

//==================== INDEX Counting Interrupted=========================
while ((digitalRead(startButton)==HIGH) && (indexPosition >1)){
lcd.setCursor(0,0); lcd.print("Limit Active!  "); 
   lcd.setCursor(0,1); lcd.print("Stopped @:"); lcd.setCursor(11,1); lcd.print(counter);
   lcd.setCursor(12,1); lcd.print("/"); lcd.print(stoplimit); //print limit counter as " __/#limit"
   lcd.setCursor(0,2); lcd.print("Index pos:"); lcd.print(indexPosition); lcd.print("/8");
  lcd.setCursor(0,3); lcd.print("   PAUSED ||      ");
 }


//==================== INDEXING COUNTING COMPLETE=========================
 while (indexPosition>=9){ //return back to start
    digitalWrite(indexPower, HIGH);// stop index 
    //LCDclear();
    lcd.setCursor(0,0); lcd.print("---Cycle Complete---");
    lcd.setCursor(0,1); lcd.print("Disable 'Run' and");
    lcd.setCursor(0,2); lcd.print("Press 'Reset' to   ");
    lcd.setCursor(0,3); lcd.print("restart.        "); 
    }
    
    
    if (indexPosition==1){ 
    /*myGLCD.setFont(MediumNumbers);  
   
   if (stoplimit<10) myGLCD.printNumI(stoplimit, 40, 10);
  if (stoplimit>=10) myGLCD.printNumI(stoplimit, CENTER, 10); 
   myGLCD.setFont(SmallFont); myGLCD.print("Push Start to ", 0, 30);
   myGLCD.print("begin count ", 0, 39);
   myGLCD.update();*/ }

   
  digitalWrite(indexPower, HIGH); //power off 
  
 
  
}

void count(){
     buttonState = digitalRead(countTrigger);
  if (buttonState != lastButtonState) {
    // if the state has changed, increment the counter
    if (buttonState == LOW) {
      // if the current state is HIGH then the button
      // wend from off to on:
      counter++;
      lcd.setCursor(10,1); lcd.print(counter);
   
    } 
    else {}
  }
  // save the current state as the last state, 
  //for next time through the loop
  lastButtonState = buttonState;
}

void countblink(){
  if (counter % 2 == 0) {
    digitalWrite(ledPin, HIGH);
  } else {
   digitalWrite(ledPin, LOW);
  } }

void leadingZero() 
{  if (encount <10){
  lcd.print("0");}
   if (encount <100){
  lcd.print("0");}}

void ClockChanged()
{
  int clkValue = digitalRead(CLK);//Read the CLK pin level
  int dtValue = digitalRead(DT);//Read the DT pin level
  if (lastCLK != clkValue)
  {
    lastCLK = clkValue;
    encount += (clkValue != dtValue ? 1 : -1);//CLK and inconsistent DT + 1, otherwise - 1
  }
}

void LCDclear(){
  lcd.setCursor(0,0);

    lcd.print("                                                                            "); //print a blank(space)
    } 


