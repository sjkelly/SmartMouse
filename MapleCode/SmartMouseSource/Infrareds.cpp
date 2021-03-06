//
//  Infrareds.c
//  
//
//  Created by Alexander Ryan on 3/21/12.
//  Copyright 2012 Worcester Polytechnic Institute. All rights reserved.
//


#include "Infrareds.h"
#include "adc.h"
#include "wirish.h"

//maintain a buffer of 100 values sampled at 1 KHz, or 100 ms of data
#define INFRARED_BUFFER_SIZE 100
#define INFRARED_TIMER 3
#define TIMER_OUTPUT_COMPARE 777
#define INFRARED_TIMER_PRESCALER 72
#define INFRARED_TIMER_OVERFLOW 2000
#define INFRARED_TIMER_FREQUENCY 1000
#define VARIATION_THRESHOLD 20
#define MINIMUM_ADC_LINEAR_READ 150
#define MAXIMUM_ADC_LINEAR_READ 853

//ADC Channels for analog reading
#define IRONE_ADC_CHANNEL 3
#define IRTWO_ADC_CHANNEL 2
#define IRTHREE_ADC_CHANNEL 0
#define IRFOUR_ADC_CHANNEL 1
#define IRFIVE_ADC_CHANNEL 10
#define IRSIX_ADC_CHANNEL 11

//Holds the most recent readings of the IR sensors
short infraredOneBuffer[INFRARED_BUFFER_SIZE];
short* IROneBufferFront = &infraredOneBuffer[0];
short infraredTwoBuffer[INFRARED_BUFFER_SIZE];
short* IRTwoBufferFront = &infraredOneBuffer[0];
short infraredThreeBuffer[INFRARED_BUFFER_SIZE];
short* IRThreeBufferFront = &infraredOneBuffer[0];
short infraredFourBuffer[INFRARED_BUFFER_SIZE];
short* IRFourBufferFront = &infraredOneBuffer[0];
short infraredFiveBuffer[INFRARED_BUFFER_SIZE];
short* IRFiveBufferFront = &infraredFiveBuffer[0];
short infraredSixBuffer[INFRARED_BUFFER_SIZE];
short* IRSixBufferFront = &infraredSixBuffer[0];

/// NOTE TO SELF::: CREATE A METHOD TO INIT THE BUFFERS TO ZERO FOR THE START

int inline Linearize(short adc_value);

char OverSampleBits = 0;

HardwareTimer IRTimer(INFRARED_TIMER);

void sampleIR();


void setIROverSampleRate(int bitsOverSample){
    OverSampleBits = bitsOverSample;
}

void setupIRPins(){
    //Set all proper pins to analog read type
    pinMode(LEFTFRONTIR, INPUT_ANALOG);
    pinMode(RIGHTFRONTIR, INPUT_ANALOG);
    pinMode(FRONTLEFTIR, INPUT_ANALOG);
    pinMode(FRONTRIGHTIR, INPUT_ANALOG);
    pinMode(LEFTREARIR, INPUT_ANALOG);
    pinMode(RIGHTREARIR, INPUT_ANALOG);
}

void setupIRTimer(){
    IRTimer.pause();
    //Use channel 1 because I can >:D
    IRTimer.setCount(0);
    IRTimer.setMode(TIMER_CH1, (timer_mode)TIMER_OUTPUT_COMPARE);
    IRTimer.setPrescaleFactor(INFRARED_TIMER_PRESCALER);
    IRTimer.setOverflow(INFRARED_TIMER_OVERFLOW);
   //Setups up IR timer to go off at 1 ms intervals to sample all ADC's
    IRTimer.setCompare(TIMER_CH1, 1);
    IRTimer.attachCompare1Interrupt(sampleIR);
    IRTimer.refresh();
    IRTimer.resume();
}

void setupIRSensors(){
    setupIRPins();
    setupIRTimer();
}
/**
 To be used to sample all
 */
void sampleIR(void){
    //Low level call to adc_read
    int count;
    short IROne = 0, IRTwo = 0, IRThree = 0, IRFour = 0, IRFive = 0, IRSix = 0;
    
    for(count = 0; count < (1 << OverSampleBits); count++){
        IROne += analogRead(INFRARED_ONE_PIN);
        IRTwo += analogRead(INFRARED_TWO_PIN);
        IRThree += analogRead(INFRARED_THREE_PIN);
        IRFour += adc_read(ADC1, IRFOUR_ADC_CHANNEL);
        IRFive += adc_read(ADC1, IRFIVE_ADC_CHANNEL);
        IRSix += adc_read(ADC1, IRSIX_ADC_CHANNEL);
    }
    
    if(!OverSampleBits){
        IROne >>= OverSampleBits;
        IRTwo >>= OverSampleBits;
        IRThree >>= OverSampleBits;
        IRFour >>= OverSampleBits;
        IRFive >>= OverSampleBits;
        IRSix >>= OverSampleBits;
    }
    
    *IROneBufferFront = IROne;
    *IRTwoBufferFront = IRTwo;
    *IRThreeBufferFront = IRThree;
    *IRFourBufferFront = IRFour;
    *IRFiveBufferFront = IRFive;
    *IRSixBufferFront = IRSix;
    
    IROneBufferFront++;
    IRTwoBufferFront++;
    IRThreeBufferFront++;
    IRFourBufferFront++;
    IRFiveBufferFront++;
    IRSixBufferFront++;
    
    if(IROneBufferFront == &infraredOneBuffer[INFRARED_BUFFER_SIZE]) IROneBufferFront = &infraredOneBuffer[0];
    if(IRTwoBufferFront == &infraredTwoBuffer[INFRARED_BUFFER_SIZE]) IRTwoBufferFront = &infraredTwoBuffer[0];
    if(IRThreeBufferFront == &infraredThreeBuffer[INFRARED_BUFFER_SIZE]) IRThreeBufferFront = &infraredThreeBuffer[0];
    if(IRFourBufferFront == &infraredFourBuffer[INFRARED_BUFFER_SIZE]) IRFourBufferFront = &infraredFourBuffer[0];
    if(IRFiveBufferFront == &infraredFiveBuffer[INFRARED_BUFFER_SIZE]) IRFiveBufferFront = &infraredFiveBuffer[0];
    if(IRSixBufferFront == &infraredSixBuffer[INFRARED_BUFFER_SIZE]) IRSixBufferFront = &infraredSixBuffer[0];
       
}

//IR One
int getLeftFrontIRRange(){
    return Linearize(*IROneBufferFront);
}
//IR Two
int getLeftRearIRRange(){
    return Linearize(*IRTwoBufferFront);
}
//IR Three
int getFrontLeftIRRange(){
    return Linearize(*IRThreeBufferFront);
}
// IR Four
int getFrontRightIRRange(){
    return Linearize(*IRFourBufferFront);
}
// IRFive
int getRightFrontIRRange(){
    return Linearize(*IRFiveBufferFront);
}
//IR Six
int getRightRearIRRange(){
    return Linearize(*IRSixBufferFront);
}
       

int isValidLeftFront(){
    if(*IROneBufferFront > MAXIMUM_ADC_LINEAR_READ || *IROneBufferFront < MINIMUM_ADC_LINEAR_READ) return false;
    else return true;
}

int isValidLeftRear(){
    if(*IRTwoBufferFront > MAXIMUM_ADC_LINEAR_READ || *IRTwoBufferFront < MINIMUM_ADC_LINEAR_READ) return false;
    else return true;
}

int isValidFrontLeft(){
    if(*IRThreeBufferFront > MAXIMUM_ADC_LINEAR_READ || *IRThreeBufferFront < MINIMUM_ADC_LINEAR_READ) return false;
    else return true;
}

int isValidFrontRight(){
    if(*IRFourBufferFront > MAXIMUM_ADC_LINEAR_READ || *IRFourBufferFront < MINIMUM_ADC_LINEAR_READ) return false;
    else return true;
}

int isValidRightFront(){
    if(*IRFiveBufferFront > MAXIMUM_ADC_LINEAR_READ || *IRFiveBufferFront < MINIMUM_ADC_LINEAR_READ) return false;
    else return true;
}

int isValidRightRear(){
    if(*IRSixBufferFront > MAXIMUM_ADC_LINEAR_READ || *IRSixBufferFront < MINIMUM_ADC_LINEAR_READ) return false;
    else return true;
}
//Value in mm of the range for the sharp IR
int inline Linearize(short adc_value){
    if(adc_value < MINIMUM_ADC_LINEAR_READ){
        return 400; //Less than linear, no equation or guarantees
    }
    if(adc_value > MAXIMUM_ADC_LINEAR_READ){
        return 40; //more than linear, no equation
    }
    else{
        return 5000 / (((int)adc_value * 25) / 606 - 21);
    }
}







