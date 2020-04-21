#define _XTAL_FREQ 16000000 
// CONFIG1
#pragma config FOSC = INTOSCIO  // Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA4/CLKO pin, I/O function on RA5/CLKI)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON       // RA3/MCLR/VPP Pin Function Select bit (RA3/MCLR/VPP pin function is MCLR; Weak pull-up enabled.)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)
#pragma config BOREN = ON       // Brown-out Reset Enable bits (Brown-out Reset enabled)
#pragma config PLLEN = ON       // INTOSC PLLEN Enable Bit (INTOSC Frequency is 16 MHz (32x))

// CONFIG2
#pragma config WRTEN = OFF      // Flash memory self-write protection bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include "setting.h"

//Количестно прошедших морганий
char light_counter = 0;
//Количество импульсов от плат учета мощности
unsigned long interrupt_counter = 0;
//Время работы микроконтроллера
unsigned long time = 0;
unsigned long start_on_light = 0;
char light_flag = 0;
char buffer = 0;

//Состояние PORTA с прошлого прерывания
char last_port_A = 0b00000000;
//Текущее состояние PORTA
char current_port_A = 0b00000000;
//На какомм пине произошло прерывание
char array_index = 0b00000000;


unsigned int interrupt_weight[5];

//Функция поворота счетного механизма
void turn() {
    if (RC1) {
        PORTC = 0b00000000;
        __delay_us(1);
        PORTC = 0b00000101;
    } else {
        PORTC = 0b00000000;
        __delay_us(1);
        PORTC = 0b00000010;
    }
}


void initPIN() {
    TRISA = 0b00110100;
    TRISC = 0b00000000;
    ANSELC = 0x00;
    ANSELA = 0x00;
    PORTCbits.RC0 = 1;
}

void initInterrupt() {
    IOCA = 0b00110100;
    GIE = 1;
    PEIE = 1;
    RABIE = 1;
    interrupt_weight[0b00000001] = INTERRUPT_WEIGTH_PHASE_C;
    interrupt_weight[0b00000010] = INTERRUPT_WEIGTH_PHASE_B;
    interrupt_weight[0b00000100] = INTERRUPT_WEIGTH_PHASE_A;
}

void initTimer2() {
    T2CON = 0b00000111;
    PIE1bits.TMR2IE = 1;
    PR2 = 250;
}

void initTimer0() {
    OPTION_REG = 0b11010011;
}


void setup() {
    initPIN();
    initInterrupt();
    initTimer0();   
    initTimer2();
    OSCCONbits.IRCF0 = 1;
    OSCCONbits.IRCF1 = 1;
}


void reset_time() {
    time = 0;
}

void interrupt isr() {  
    //Обработка сигнала от модулей измерения мощности
    if (RABIF) {
        buffer = PORTA;
        current_port_A = ~(((((buffer << 1) & 0b00001000)|buffer)>>3)&0b00000111);
        array_index = current_port_A^last_port_A;
        if (current_port_A & array_index) {
            interrupt_counter += interrupt_weight[array_index];
        } 
        if (interrupt_counter >= cr_limit) {
            //ВКлючить диод
            PORTCbits.RC0 = 0;
            interrupt_counter = 0;
            light_flag = 1;
        }
        RABIF = 0;
        last_port_A = current_port_A;
    } else {
        time++;
        TMR2IF = 0;
    }
    
}

void isLightFlag() {
    if (light_flag) {
            start_on_light = time;
            light_counter++;
            if (light_counter == LIGHT_LIMIT) {
                turn();
                light_counter = 0;
            }
            light_flag = 0;
        }
}

void isLightOff() {
    if ((time - start_on_light >= TIME_ON_LIGHT) && !RC0) {
            PORTCbits.RC0 = 1;
        }
}

void isResetTime() {
   if (time > 4294967200) {
            reset_time();
        } 
}

void isTimeToUpgrade() {
    if (time > UPGRADE_LIMIT) {
            cr_limit = 12800;
        }
}

void setOption() {
    
}


void main(void) {
    setup();
    while (1) {
        //Если загорелся диод, то нужно засечь время когда он загорелся и повернуть счетный
        //если нужно
        isLightFlag();
        //Выключить диод, если время его горения истекло
        isLightOff();
        //Если счетчик времени переполнен - сбросить
        isResetTime();
        //Если прошло время с включения, то включить недоучет
        isTimeToUpgrade();
       
    }
}
