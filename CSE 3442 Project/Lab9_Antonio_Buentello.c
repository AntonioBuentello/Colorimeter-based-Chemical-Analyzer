//Antonio Buentello
//Project
// Hardware configuration:
// Red Backlight LED:
//   M0PWM2 (PB4) drives an NPN transistor that powers the red LED
// Green Backlight LED:
//   M1PWM2 (PA6) drives an NPN transistor that powers the green LED
// Blue Backlight LED:
//   M1PWM3 (PA7) drives an NPN transistor that powers the blue LED
//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "clock.h"
#include "tm4c123gh6pm.h"
#include "lab5call.h"
#include "uart0.h"
#include "wait.h"
#include "rgb_led.h"
#include "adc0.h"
#include "math.h"
#include <float.h>

// Bitband aliases
#define A1_PIN    (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 4*4)))//PE4 Black
#define A2_PIN    (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 5*4)))//PE5 White
#define A3_PIN    (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 2*4)))//PE2 Green
#define A4_PIN    (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 3*4)))//PE3 Yellow

// Port E masks
#define A1_PIN_MASK 16
#define A2_PIN_MASK 32
#define A3_PIN_MASK 4
#define A4_PIN_MASK 8

#define AIN11_MASK 32 //PB5

#define RED_LED_R      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define BLUE_LED_R      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))
#define GREEN_LED_R    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))

// Port F masks
#define RED_LED_MASK_R 2
#define BLUE_LED_MASK_R 8
#define GREEN_LED_MASK_R 4

// Port D masks
#define FREQ_IN_MASK 1

//Global variables
uint16_t r,g,b;
uint16_t pwm_r;
uint16_t pwm_g;
uint16_t pwm_b;
uint16_t analog_r=0;
uint16_t analog_g;
uint16_t analog_b;

uint8_t phase = 0;
uint8_t position;
uint16_t dc;

uint32_t time[50];
uint8_t count = 0;
uint8_t code;
uint8_t secondarycode;
bool valid = false;

char bnames[44][14] = {"Light Up","Light Down","Play","Power","Red","Green","Blue","White","Orange R3",
                        "Green R3","Blue R3","Pink R3","Orange R4","Green R4","Purple R4","Pink R4",
                        "Orange R5","Green R5","Purple R5","Light Blue R5","Yellow R6","Green R6",
                        "Dark Pink R6","Light Blue R6","Red Up","Green Up","Blue Up","Quick","Red Down",
                         "Green Down","Blue Down","Slow","DIY1","DIY2","DIY3","Auto","DIY4","DIY5",
                         "DIY6","Flash","Jump3","Jump7","Fade3","Fade7"};
uint8_t buttons[] = {92,93,65,64,88,89,69,68,84,85,73,72,80,81,77,76,28,29,30,31,24,25,26
                         ,27,20,21,22,23,16,17,18,19,12,13,14,15,8,9,10,11,4,5,6,7};

uint8_t addr[8];
uint8_t data[8];
uint8_t addri[8];
uint8_t datai[8];

bool checkError(void);
uint8_t bToI(uint8_t[ ]);
uint8_t getButton(void);
void parseBuffer(void);
uint8_t invertBit(uint8_t);
void goTube(uint8_t);

void enableTimerMode(){
    // Input Edge Time Mode
    WTIMER2_CTL_R &= ~TIMER_CTL_TAEN;                // turn-off counter before reconfiguring
    WTIMER2_CFG_R = 4;                               // configure as 32-bit counter (A only)
    WTIMER2_TAMR_R = TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR; // configure for edge time mode, count up
    WTIMER2_CTL_R = TIMER_CTL_TAEVENT_NEG;
    WTIMER2_IMR_R = TIMER_IMR_CAEIM;                 // turn-on interrupts
    WTIMER2_TAV_R = 0;
    WTIMER2_CTL_R |= TIMER_CTL_TAEN;                 // turn-on counter
    NVIC_EN3_R |= 1 << (INT_WTIMER2A-16-96);         // turn-on interrupt 114 (WTIMER2A)
}
uint8_t bToI(uint8_t byte[8]){
    //convert to integer
    uint8_t i;
    uint8_t mult = 1;
    uint8_t res = 0;
    for(i =0;i<8;i++){
        res += byte[7-i] * mult;
        mult *= 2;
    }
    return res;
}
uint8_t invertBit(uint8_t bit){
    if(bit==0)
        return 1;
    if(bit==1)
        return 0;

    return -1;
}
bool checkComp(){
    uint8_t i;
    for(i = 0;i<8;i++){
        // Check correct bit
        if(!(addr[i]==0||addr[i]==1))
            return true;
        if(!(addri[i]==0||addri[i]==1))
            return true;
        if(!(data[i]==0||data[i]==1))
            return true;
        if(!(datai[i]==0||datai[i]==1))
            return true;
        // Check the complement
        if(addr[i]!=invertBit(addri[i]))
            return true;
        if(data[i]!=invertBit(datai[i]))
            return true;

        valid = true;
    }
    return false;
}
uint8_t getButton(){
    //Associate button with data integer
    uint8_t buttons[] = {92,93,65,64,88,89,69,68,84,85,73,72,80,81,77,76,28,29,30,31,24,25,26
                         ,27,20,21,22,23,16,17,18,19,12,13,14,15,8,9,10,11,4,5,6,7};
    uint8_t i;
    if(bToI(addr) == 0){
        for(i=0;i<45;i++){
            if(bToI(data)==buttons[i])
                return i+1;
        }
        return 250;//invalid
    }
    else
        return 251;// invalid
}
void putiUart0(uint8_t num){
    uint8_t countValue = num;
    uint8_t digits = 0;
    while(countValue > 0){
        countValue /= 10;
        digits++;
    }
    if(digits == 0){
        putcUart0('0');
    }
    else{
        char ans[4] ="\0\0\0\0";
        for (digits=digits;digits > 0;digits--) {
            ans[digits-1]=(num%10) + '0';
            num /= 10;
        }
        putsUart0(ans);
    }
}
void putintUart0(uint8_t n[8]){
    int k;
    for(k = 0;k <= 7 ;k++){
        putiUart0(n[k]);
    }
}
void dectoHex(uint8_t c){

    char hex[8] = {};
    int i = 0;
    int hexnum = code;
    while(hexnum!=0){
        int temp = 0;
        temp = hexnum % 16;
        if(temp < 10){
            hex[i] = temp + 48;
            i++;
        }
        else{
            hex[i] = temp + 55;
            i++;
        }
        hexnum = hexnum/16;
    }
    char temp1;
    char temp2;
    temp1 = hex[0];
    temp2 = hex[1];
    hex[1] = temp1;
    hex[0] = temp2;

    putcUart0(hex[0]);
    putcUart0(hex[1]);
}
void sepCheck(){//sort through 32 bits and separate into separate arrays for comparison

    uint8_t bitNum;
    uint8_t l = 0;
    for(bitNum = 33; bitNum >=2;bitNum--){

        if(l==8)
            l=0;

        if(bitNum <=33 && bitNum >=26)
            datai[l] = time[bitNum];    //separate data inverse
        if(bitNum <= 25 && bitNum >= 18)
            data[l] = time[bitNum];     //separate data or code
        if(bitNum <= 17 && bitNum >= 10)
            addri[l] = time[bitNum];    //separate address inverse
        if(bitNum <= 9 && bitNum >= 2)
            addr[l] = time[bitNum];     //separate address

        l++;
    }
    int s;
    for(s = 0; s < 8; ++s ){
          uint8_t shiftAmount = 8 - s - 1;
          uint8_t shifted = data[s] << shiftAmount; // bitwise shift data into shifted variable
          code |= shifted;                          // add(OR) shifted variables into code variable
    }
}
void gpioIsr(){ // placed in ccs file - calls once every second
    // Configure Wide Timer 2 as counter of external events on WT2CCP0 pin
    time[count] = WTIMER2_TAV_R;//read counter input
    WTIMER2_TAV_R = 0;//reset counter for next period

    uint16_t T = (1.125*40000)/2;
    if(count == 0){
        WTIMER2_TAV_R = 0;//set counter to 0
        time[count] = 0;//set first time variable to zero
        count++; // increment time array
    }
    else{
        time[count] = WTIMER2_TAR_R;//set next counter variable in time array
        if(count == 1){
            uint16_t time1 = (time[1]/40000);
            uint16_t time0 = (time[0]/40000);
            if((abs(time1 - time0)) >= 13 && (abs(time1- time0)) <= 14){//if count is 1 continue
                count++;
            }
        }
        else if(count > 1){ //if count is above one, continue
            uint16_t t = ((time[count]/40000)-(time[count-1]/40000));
            if((t >= (1.5 * T) && t <= (2.5 * T)) || (t >= (3.5 * T) && t <= (4.5 * T))){
                //1.5T = 33750
                //2.5T = 56250  between 1.5T and 2.5T is '0'
                //3.5T = 78750
                //4.5T = 101250 between 3.5T and 4.5T is '1'
             }
            count++;
        }
    }
    if(count>33){ //set everything else to zero, then change counter times to 1's or 0's
        time[count] = 0;
        count = 0;
        int i = 0;
        for(i = 33;i >=2; i--){
            if(time[i] >= (1.5 * T) && time[i] <= (2.5 * T)){
                time[i] = 0;
            }
            else{
                time[i] = 1;
            }
        }
        sepCheck();//verify bit address and data
    }
    GREEN_LED_R ^= 1;                              // status
    WTIMER2_ICR_R = TIMER_ICR_CAECINT;           // clear interrupt flag
}
void applyPhase(uint8_t phaseIn){
    waitMicrosecond(13000);
    switch(phaseIn){

        case 0:
            A1_PIN = 0;
            A2_PIN = 0;
            A3_PIN = 0;
            A4_PIN = 1;//Yellow
            break;
        case 1:
            A1_PIN = 0;
            A2_PIN = 1;//White
            A3_PIN = 0;
            A4_PIN = 0;
            break;
        case 2:
            A1_PIN = 0;
            A2_PIN = 0;
            A3_PIN = 1;//Green
            A4_PIN = 0;
            break;
        case 3:
            A1_PIN = 1;//Black
            A2_PIN = 0;
            A3_PIN = 0;
            A4_PIN = 0;
            break;
    }
    phase = phaseIn;
}
void stepCW(){
    position++;
    phase = (phase + 1) % 4;
    applyPhase(phase);
}
void stepCCW(){
    position--;
    phase = (phase - 1) % 4;
    if(phase==255){ //safety net for when phase counts back to -1, then reverts to 255
        phase = 3;
    }
    applyPhase(phase);
}
void setPosition(uint8_t positionIn){
    position = positionIn;
}
void home(){

    uint8_t q;
    for(q = 0; q < 200;q++){
        stepCW();
    }
    stepCCW();
    stepCCW();
    stepCCW();
    stepCCW();
    stepCCW();
    stepCCW();
    setPosition(194);
    valid = true;
}
void calibrate(){
    uint16_t cal_count = 0;
    if(cal_count == 0){
        home();
        count++;
    }
    if(cal_count>0){
        goTube(0);
    }
    uint16_t i,g,h;
    uint16_t raw =0;
    char cal_temp[100];
    char c_temp[20];
    //Red
    for(i = 0; raw < 3072;i = (i+1) % 1024){
        setRgbColor(i,0,0);
        waitMicrosecond(12000);
        raw = readAdc0Ss3();
    }
    analog_r = raw;
    pwm_r = i;
    raw = 0;
    //Green
    for(g = 0; raw < 3072;g =(g+1) % 1024){
        setRgbColor(0,g,0);
        waitMicrosecond(12000);
        raw = readAdc0Ss3();
    }
    analog_g = raw;
    pwm_g = g;
    raw = 0;
    //Blue
    for(h = 0;raw < 3072;h =(h+1) % 1024){
        setRgbColor(0,0,h);
        waitMicrosecond(12000);
        raw = readAdc0Ss3();
    }
    pwm_b = h;
    analog_b = raw;
    raw = 0;
    sprintf(cal_temp, "Raw Analog Measurement: (%d,%d,%d)\nRaw PWM Measurement: (%d,%d,%d)\n"
    ,analog_r,analog_g,analog_b,pwm_r,pwm_g,pwm_b);
    putsUart0(cal_temp);

    setRgbColor(0,0,0);
}
void calpos(uint8_t setpos){

    uint8_t stepccwResult = position - setpos;
    uint8_t stepcwResult = setpos - position;
    uint8_t c=0;

    if(position < setpos){
        for(c = 0; c < stepcwResult;c++){
            stepCW();
        }
    }
    if(position > setpos){
        for(c = 0; c < stepccwResult;c++){
            stepCCW();
        }
    }
}
void goTube(uint8_t tube){ // tube positions based off of

    switch(tube){
            case 0:
                calpos(194);
                setPosition(194);
                break;
            case 1:
                calpos(28);
                setPosition(28);
                break;
            case 2:
                calpos(62);
                setPosition(62);
                break;
            case 3:
                calpos(95);
                setPosition(95);
                break;
            case 4:
                calpos(128);
                setPosition(128);
                break;
            case 5:
                calpos(161);
                setPosition(161);
                break;
         }
    code =0;
    valid = true;
}
void measure(uint8_t tube, uint16_t *r, uint16_t *g, uint16_t *b){

    goTube(tube);
    waitMicrosecond(220000);
    uint16_t raw = 0;
    dc = 0;

    for(dc = 0; dc < pwm_r;dc++){
        setRgbColor(dc,0,0);
        waitMicrosecond(12000);
        raw = readAdc0Ss3();
        *r = raw;
    }
    raw = 0;
    dc = 0;
    for(dc = 0; dc < pwm_g;dc++){
        setRgbColor(0,dc,0);
        waitMicrosecond(12000);
        raw = readAdc0Ss3();
        *g = raw;
    }
    raw = 0;
    dc = 0;
    for(dc = 0; dc < pwm_b;dc++){
            setRgbColor(0,0,dc);
            waitMicrosecond(12000);
            raw = readAdc0Ss3();
            *b = raw;
        }
    raw = 0;
    dc = 0;
    setRgbColor(0,0,0);
}
//Calculate pH formula using Linear Regression. Line created by averages of data points.
//Line is a rough approximation of data
float pH_Calc(uint16_t *r, uint16_t *g,uint16_t *b){ //Calculate pH
    float result;
    float k, sum_Rgb,log_Rgb,dif_g,dif_b;

    dif_g = (float)*g/(float)analog_g;
    dif_b = (float)*b/(float)analog_b;

    k = (dif_g*dif_g) + (dif_b*dif_g)*(dif_b*dif_g);
    sum_Rgb = (k)/(dif_g+dif_b);
    log_Rgb = 10*log10(sum_Rgb);

    result = (-0.12890218*log_Rgb)+6.56413806;
    return result;
}
float Cl_Calc(uint16_t *r, uint16_t *g,uint16_t *b){ //Calculate Cl
    float result;
    float dif_g_Cl,dif_b_Cl,sum_Rgb_Cl,m,log_Rgb_Cl;

    dif_g_Cl = (float)*g/(float)analog_g;
    dif_b_Cl = (float)*b/(float)analog_b;

    m = (dif_g_Cl*dif_g_Cl) + (dif_b_Cl*dif_g_Cl)*(dif_b_Cl*dif_g_Cl);
    sum_Rgb_Cl = (m)/(dif_g_Cl+dif_b_Cl);
    log_Rgb_Cl = 10*log10(sum_Rgb_Cl);
    result = (-2.63365815*log_Rgb_Cl)-2.99025546;
    //result = (-0.913242*log_Rgb_Cl)-0.26803652;
    return result;
}
void initHw(){
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();
    // Enable clocks
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R2; // Enable timer 2
    SYSCTL_RCGCGPIO_R = SYSCTL_RCGCGPIO_R4 | SYSCTL_RCGCGPIO_R1 | SYSCTL_RCGCGPIO_R5 | SYSCTL_RCGCGPIO_R3;
    _delay_cycles(3);
    // Configure Motor pins
    GPIO_PORTE_DIR_R |= A1_PIN_MASK | A2_PIN_MASK| A3_PIN_MASK| A4_PIN_MASK;  // outputs
    GPIO_PORTE_DR2R_R |= A1_PIN_MASK | A2_PIN_MASK| A3_PIN_MASK| A4_PIN_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTE_DEN_R |= A1_PIN_MASK | A2_PIN_MASK| A3_PIN_MASK| A4_PIN_MASK;  // enable LEDs

    //configure AIN1 as an analog input
    GPIO_PORTB_AFSEL_R |= AIN11_MASK; //select alternative functions for AN1 (PB5)
    GPIO_PORTB_DEN_R &= ~AIN11_MASK;  //turn off digital operation on pin PB5
    GPIO_PORTB_AMSEL_R |= AIN11_MASK; //turn on analog operation on

    // Configure LED and Freq_in_mask
    GPIO_PORTF_DIR_R |= GREEN_LED_MASK_R |RED_LED_MASK_R|BLUE_LED_MASK_R;  //
    GPIO_PORTF_DR2R_R |= GREEN_LED_MASK_R |RED_LED_MASK_R|BLUE_LED_MASK_R;
    GPIO_PORTF_DEN_R |= GREEN_LED_MASK_R |RED_LED_MASK_R|BLUE_LED_MASK_R;
    GPIO_PORTD_AFSEL_R |= FREQ_IN_MASK;          // select alternative functions for SIGNAL_IN pin
    GPIO_PORTD_PCTL_R &= ~GPIO_PCTL_PD0_M;       // map alt fns to SIGNAL_IN
    GPIO_PORTD_PCTL_R |= GPIO_PCTL_PD0_WT2CCP0;
    GPIO_PORTD_DEN_R |= FREQ_IN_MASK;            //Enable digital operation
}
void print_Welcome(){

    putsUart0("\n");
    putsUart0("Welcome to Antonio's Spring 2022 Embedded I Project: \n");
    putsUart0("This Project measures the pH or Chlorine balance in a test tube\n");
    putsUart0("Use the remote to calibrate the sensor, measure the Raw analog or pH values: \n");
}
void help(){

    putsUart0("The default is to calibrate measurements before every test\n\n");
    putsUart0("Press button once, then wait until command is complete. No queue available.\n\n");
    putsUart0("\tButtons:\n");
    putsUart0("\t\tMeasure pH-\n");
    putsUart0("\t\t\tR -  Measure Tube 1 pH \n");
    putsUart0("\t\t\tG -  Measure Tube 2 pH \n");
    putsUart0("\t\t\tB -  Measure Tube 3 pH \n");
    putsUart0("\t\t\tW -  Measure Tube 4 pH \n");
    putsUart0("\t\t\tO3 - Measure Tube 5 pH \n\n");
    putsUart0("\t\tGo Tube-\n");
    putsUart0("\t\t\tUp Red -     Go to tube 0\n");
    putsUart0("\t\t\tUp Green -   Go to tube 1\n");
    putsUart0("\t\t\tUp Blue -    Go to tube 2\n");
    putsUart0("\t\t\tDown Red -   Go to tube 3\n");
    putsUart0("\t\t\tDown Green - Go to tube 4\n");
    putsUart0("\t\t\tDown Blue -  Go to tube 5\n\n");
    putsUart0("\t\tMeasure Raw-\n");
    putsUart0("\t\t\tDIY1 - Measure Tube 0 Raw \n");
    putsUart0("\t\t\tDIY2 - Measure Tube 1 Raw \n");
    putsUart0("\t\t\tDIY3 - Measure Tube 2 Raw \n");
    putsUart0("\t\t\tDIY4 - Measure Tube 3 Raw \n");
    putsUart0("\t\t\tDIY5 - Measure Tube 4 Raw \n");
    putsUart0("\t\t\tDIY6 - Measure Tube 5 Raw \n\n");
    putsUart0("\t\tDefault Buttons-\n");
    putsUart0("\t\t\tLight Up - Calibrate\n");
    putsUart0("\t\t\tLight Down - Home\n");
    putsUart0("\tKeyboard Commands:\n");
    putsUart0("\tAll keyboard commands are the same except for ""turret""\n");
    putsUart0("\tEntering turret 0-5 takes turret like Go Tube\n");

}
int main(void){
    char temp_rgb[60];
    char temp_pH[60];
    char temp_Cl[60];
    float pH = 0.0;
    float Cl = 0.0;
    uint8_t tubenum;
    initHw();
    initRgb();
    initUart0();
    initAdc0Ss3();
    enableTimerMode();
    setUart0BaudRate(115200,40e6);
    // Use AIN3 input with N=4 hardware sampling
    setAdc0Ss3Mux(11);
    setAdc0Ss3Log2AverageCount(2);
    print_Welcome();


    calibrate();
    while(true){
        //uint8_t buttons[] = {92,93,65,64,88,89,69,68,84,85,73,72,80,81,77,76,28,29,30,31,24,25,26
        //                         ,27,20,21,22,23,16,17,18,19,12,13,14,15,8,9,10,11,4,5,6,7};
        if(valid){
            // switch statement allows commands to be processed through remote
            switch(code){

            case 93:
                home();valid = true;code = 0;break;
            case 92:
                calibrate();valid = true;code = 0;break;
            //Measure Raw
            case 12:
                measure(0,&r,&g,&b);
                sprintf(temp_rgb, "Measured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n",r,g,b,pwm_r,pwm_g,pwm_b);
                putsUart0(temp_rgb);
                valid = true;code = 0;break;
            case 13:
                measure(1,&r,&g,&b);
                sprintf(temp_rgb, "Measured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n",r,g,b,pwm_r,pwm_g,pwm_b);
                putsUart0(temp_rgb);
                valid = true;code = 0;break;
            case 14:
                measure(2,&r,&g,&b);
                sprintf(temp_rgb, "Measured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n",r,g,b,pwm_r,pwm_g,pwm_b);
                putsUart0(temp_rgb);
                valid = true;code = 0;break;
            case 8:
                measure(3,&r,&g,&b);
                sprintf(temp_rgb, "Measured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n",r,g,b,pwm_r,pwm_g,pwm_b);
                putsUart0(temp_rgb);
                valid = true;code = 0;break;
            case 9:
                measure(4,&r,&g,&b);
                sprintf(temp_rgb, "Measured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n",r,g,b,pwm_r,pwm_g,pwm_b);
                putsUart0(temp_rgb);
                valid = true;code = 0;break;
            case 10:
                measure(5,&r,&g,&b);
                sprintf(temp_rgb, "Measured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n",r,g,b,pwm_r,pwm_g,pwm_b);
                putsUart0(temp_rgb);
                valid = true;code = 0;break;
            //Go Tubes
            case 20:goTube(0);valid = true;code = 0;break;
            case 21:goTube(1);valid = true;code = 0;break;
            case 22:goTube(2);valid = true;code = 0;break;
            case 16:goTube(3);valid = true;code = 0;break;
            case 17:goTube(4);valid = true;code = 0;break;
            case 18:goTube(5);valid = true;code = 0;break;
            //Measure pH
            case 88:
                measure(1,&r,&g,&b);
                pH = pH_Calc(&r,&g,&b);
                sprintf(temp_pH,"pH: %.1f\nMeasured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n", pH,r,g,b,pwm_r,pwm_g,pwm_b);
                putsUart0(temp_pH);
                valid = true;code = 0;break;
            case 89:
                measure(2,&r,&g,&b);
                pH = pH_Calc(&r,&g,&b);
                sprintf(temp_pH,"pH: %.1f\nMeasured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n", pH,r,g,b,pwm_r,pwm_g,pwm_b);
                putsUart0(temp_pH);
                valid = true;code = 0;break;
            case 69:
                measure(3,&r,&g,&b);
                pH = pH_Calc(&r,&g,&b);
                sprintf(temp_pH,"pH: %.1f\nMeasured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n", pH,r,g,b,pwm_r,pwm_g,pwm_b);
                putsUart0(temp_pH);
                valid = true;code = 0;break;
            case 68:
                measure(4,&r,&g,&b);
                pH = pH_Calc(&r,&g,&b);
                sprintf(temp_pH,"pH: %.1f\nMeasured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n", pH,r,g,b,pwm_r,pwm_g,pwm_b);
                putsUart0(temp_pH);
                valid = true;code = 0;break;
            case 84:
                measure(5,&r,&g,&b);
                pH = pH_Calc(&r,&g,&b);
                sprintf(temp_pH,"pH: %.1f\nMeasured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n", pH,r,g,b,pwm_r,pwm_g,pwm_b);;
                putsUart0(temp_pH);valid = true;code = 0;break;
            //Measure Cl
            case 80:
                measure(1,&r,&g,&b);
                Cl = Cl_Calc(&r,&g,&b);
                sprintf(temp_Cl,"Cl: %.1f\nMeasured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n", Cl,r,g,b,pwm_r,pwm_g,pwm_b);;
                putsUart0(temp_Cl);valid = true;code = 0;break;
            case 81:
                measure(2,&r,&g,&b);
                Cl = Cl_Calc(&r,&g,&b);
                sprintf(temp_Cl,"Cl: %.1f\nMeasured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n", Cl,r,g,b,pwm_r,pwm_g,pwm_b);;
                putsUart0(temp_Cl);valid = true;code = 0;break;
            case 77:
                measure(3,&r,&g,&b);
                Cl = Cl_Calc(&r,&g,&b);
                sprintf(temp_Cl,"Cl: %.1f\nMeasured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n", Cl,r,g,b,pwm_r,pwm_g,pwm_b);;
                putsUart0(temp_Cl);valid = true;code = 0;break;
            case 76:
                measure(4,&r,&g,&b);
                Cl = Cl_Calc(&r,&g,&b);
                sprintf(temp_Cl,"Cl: %.1f\nMeasured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n", Cl,r,g,b,pwm_r,pwm_g,pwm_b);;
                putsUart0(temp_Cl);valid = true;code = 0;break;
            case 28:
                measure(5,&r,&g,&b);
                Cl = Cl_Calc(&r,&g,&b);
                sprintf(temp_Cl,"Cl: %.1f\nMeasured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n", Cl,r,g,b,pwm_r,pwm_g,pwm_b);;
                putsUart0(temp_Cl);valid = true;code = 0;break;
            case 23:
                help();code = 0;valid = true;
                break;
            }
            // Below allows user to enter commands in terminal
            if(kbhitUart0()){
                if(valid){
                    getsUart0(&data);
                    //putsUart0(data.buffer);
                    putcUart0('\n');
                    parseFields(&data);
                    if(isCommand(&data,"home",0)){
                        home();
                        valid = true;
                    }
                    if(isCommand(&data,"calibrate",0)){
                        calibrate();
                        valid = true;
                    }
                    if(isCommand(&data,"turret",1)){
                        tubenum = getFieldInteger(&data, 1); // grabs integer for specific tube
                        goTube(tubenum);
                    }
                    if(isCommand(&data,"measure",2)){
                        tubenum = getFieldInteger(&data, 1); // grabs integer for specific tube
                        char *str = getFieldString(&data, 2);
                        if(myComp(str,"Raw")){
                            measure(tubenum,&r,&g,&b);
                            sprintf(temp_rgb, "Measured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n",r,g,b,pwm_r,pwm_g,pwm_b);
                            putsUart0(temp_rgb);
                            valid = true;
                        }
                        if(myComp(str,"pH")){
                            if(tubenum > 0 && tubenum < 6){
                                measure(tubenum,&r,&g,&b);
                                pH = pH_Calc(&r,&g,&b);
                                //pH = (-0.09890916*log_Rgb)+6.56413798;
                                sprintf(temp_pH,"pH: %.1f\nMeasured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n", pH,r,g,b,pwm_r,pwm_g,pwm_b);
                                putsUart0(temp_pH);
                                valid = true;
                            }
                            else{
                                valid = false;
                            }
                        }
                        if(myComp(str,"Cl")){
                            if(tubenum < 6){
                                measure(tubenum,&r,&g,&b);
                                Cl = Cl_Calc(&r,&g,&b);
                                sprintf(temp_Cl,"Cl: %.1f\nMeasured Raw: (%d,%d,%d)\nRaw PWM: (%d,%d,%d)\n", Cl,r,g,b,pwm_r,pwm_g,pwm_b);;
                                putsUart0(temp_Cl);
                                valid = true;
                            }
                            else{
                                valid = false;
                            }
                        }

                    }
                    if(!valid){
                        putsUart0("Invalid command\n");
                    }
                }

            }
        }
    }
    return 0;
}
