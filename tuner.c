/*
 * Arduino Guitar Tuner
 * by Nicole Grimwood
 *
 * For more information please visit:
 * http://www.instructables.com/id/Arduino-Guitar-Tuner/
 *
 * Based upon:
 * Arduino Frequency Detection
 * created October 7, 2012
 * by Amanda Ghassaei
 *
 * This code is in the public domain. 
 * mod by Psyrax https://github.com/psyrax
*/


//data storage variables
byte newData = 0;
byte prevData = 0;
unsigned int time = 0;//keeps time and sends vales to store in timer[] occasionally
int timer[10];//storage for timing of events
int slope[10];//storage for slope of events
unsigned int totalTimer;//used to calculate period
unsigned int period;//storage for period of wave
byte index = 0;//current storage index
float frequency;//storage for frequency calculations
int maxSlope = 0;//used to calculate max slope as trigger point
int newSlope;//storage for incoming slope data

//variables for deciding whether you have a match
byte noMatch = 0;//counts how many non-matches you've received to reset variables if it's been too long
byte slopeTol = 3;//slope tolerance- adjust this if you need
int timerTol = 10;//timer tolerance- adjust this if you need

//variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 20;//raise if you have a very noisy signal

//variables for tuning
int correctFrequency;//the correct frequency for the string being played

int redPin = 3;
int greenPin = 5;
int bluePin = 6;
int redPin2 = 9;
int greenPin2 = 10;
int bluePin2 = 11;
int redPinTurn;
int greenPinTurn;
int bluePinTurn;
 
//uncomment this line if using a Common Anode LED
#define COMMON_ANODE

void setup(){
  
  Serial.begin(9600);
  
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(redPin2, OUTPUT);
  pinMode(greenPin2, OUTPUT);
  pinMode(bluePin2, OUTPUT);  
  
  
  
  cli();//disable interrupts
  
  //set up continuous sampling of analog pin 0 at 38.5kHz
 
  //clear ADCSRA and ADCSRB registers
  ADCSRA = 0;
  ADCSRB = 0;
  
  ADMUX |= (1 << REFS0); //set reference voltage
  ADMUX |= (1 << ADLAR); //left align the ADC value- so we can read highest 8 bits from ADCH register only
  
  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //set ADC clock with 32 prescaler- 16mHz/32=500kHz
  ADCSRA |= (1 << ADATE); //enabble auto trigger
  ADCSRA |= (1 << ADIE); //enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN); //enable ADC
  ADCSRA |= (1 << ADSC); //start ADC measurements
  
  sei();//enable interrupts
}

ISR(ADC_vect) {//when new ADC value ready
  
  PORTB &= B11101111;//set pin 12 low
  prevData = newData;//store previous value
  newData = ADCH;//get value from A0
  if (prevData < 127 && newData >=127){//if increasing and crossing midpoint
    newSlope = newData - prevData;//calculate slope
    if (abs(newSlope-maxSlope)<slopeTol){//if slopes are ==
      //record new data and reset time
      slope[index] = newSlope;
      timer[index] = time;
      time = 0;
      if (index == 0){//new max slope just reset
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
        index++;//increment index
      }
      else if (abs(timer[0]-timer[index])<timerTol && abs(slope[0]-newSlope)<slopeTol){//if timer duration and slopes match
        //sum timer values
        totalTimer = 0;
        for (byte i=0;i<index;i++){
          totalTimer+=timer[i];
        }
        period = totalTimer;//set period
        //reset new zero index values to compare with
        timer[0] = timer[index];
        slope[0] = slope[index];
        index = 1;//set index to 1
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
      }
      else{//crossing midpoint but not match
        index++;//increment index
        if (index > 9){
          reset();
        }
      }
    }
    else if (newSlope>maxSlope){//if new slope is much larger than max slope
      maxSlope = newSlope;
      time = 0;//reset clock
      noMatch = 0;
      index = 0;//reset index
    }
    else{//slope not steep enough
      noMatch++;//increment no match counter
      if (noMatch>9){
        reset();
      }
    }
  }
  
  time++;//increment timer at rate of 38.5kHz
  
  ampTimer++;//increment amplitude timer
  if (abs(127-ADCH)>maxAmp){
    maxAmp = abs(127-ADCH);
  }
  if (ampTimer==1000){
    ampTimer = 0;
    checkMaxAmp = maxAmp;
    maxAmp = 0;
  }
  
}

void reset(){//clean out some variables
  index = 0;//reset index
  noMatch = 0;//reset match couner
  maxSlope = 0;//reset slope
}

//Determine the correct frequency and light up 
//the appropriate LED for the string being played 


void setColor(int ledPick,int red, int green, int blue)
{
  if ( ledPick == 1 ){
    redPinTurn = redPin;
    greenPinTurn = greenPin;
    bluePinTurn = bluePin;
  } else if ( ledPick == 2 ) {
    redPinTurn = redPin2;
    greenPinTurn = greenPin2;
    bluePinTurn = bluePin2;
  }
  #ifdef COMMON_ANODE
    red = 255 - red;
    green = 255 - green;
    blue = 255 - blue;
  #endif
  analogWrite(redPinTurn, red);
  analogWrite(greenPinTurn, green);
  analogWrite(bluePinTurn, blue);  
}
void tuningFeedback(int status){
  switch (status){
    case 1:
      setColor(1, 0, 171, 181);
    break;
    case 2:
      setColor(1, 244, 28, 84);
    break;
    case 3:
    setColor(1, 193,201,35);
    break;
  }
}

void setString(int string){
  switch(string){
    case 1:
      setColor(2, 230, 34, 64);
    break;
    case 2:
      setColor(2, 240, 161, 29);
    break;
    case 3:
      setColor(2, 135, 149, 22);
    break;
    case 4:
      setColor(2, 87, 25, 22);
    break;
    case 5:
      setColor(2, 9, 101, 105);
    break;
    case 6:
      setColor(2, 49, 39, 39);
    break;
  }
}

//E - 82.4 Hz
//A - 110 Hz
//D - 146.8 Hz
//G - 196 Hz
//B - 246.9 Hz
//E - 329.6 Hz


void stringCheck(){
  if(frequency>70&frequency<90){
    setString(1);
    correctFrequency = 82.4;
  }
  if(frequency>100&frequency<120){
    setString(2);
    correctFrequency = 110;
  }
  if(frequency>135&frequency<155){
    setString(3);
    correctFrequency = 146.8;
  }
  if(frequency>186&frequency<205){
    setString(4);
    correctFrequency = 196;
  }
  if(frequency>235&frequency<255){
    setString(5);
    correctFrequency = 246.9;
  }
  if(frequency>320&frequency<340){
    setString(6);
    correctFrequency = 329.6;
  }
}

//Compare the frequency input to the correct 
//frequency and light up the appropriate LEDS
void frequencyCheck(){
  if(frequency>correctFrequency+1){
    tuningFeedback(1);
  }
  if(frequency>correctFrequency+4){
    tuningFeedback(1);
  }
  if(frequency>correctFrequency+6){
    tuningFeedback(1);
  }
  if(frequency<correctFrequency-1){
    tuningFeedback(2);
  }
  if(frequency<correctFrequency-4){
    tuningFeedback(2);
  }
  if(frequency<correctFrequency-6){
    tuningFeedback(2);
  }
  if(frequency>correctFrequency-1&frequency<correctFrequency+1){
    tuningFeedback(3);
  }
}



void loop(){

  
  if (checkMaxAmp>ampThreshold){
    frequency = 38462/float(period);//calculate frequency timer rate/period
  }
  
  stringCheck();
  frequencyCheck();
  
  delay(100);
 
}



