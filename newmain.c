#define _XTAL_FREQ 16000000 
// CONFIG1
#pragma config FOSC = INTOSCIO  // Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA4/CLKO pin, I/O function on RA5/CLKI)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON       // RA3/MCLR/VPP Pin Function Select bit (RA3/MCLR/VPP pin function is MCLR; Weak pull-up enabled.)
#pragma config CP = ON         // Flash Program Memory Code Protection bit (Code protection off)
#pragma config BOREN = ON       // Brown-out Reset Enable bits (Brown-out Reset enabled)
#pragma config PLLEN = ON       // INTOSC PLLEN Enable Bit (INTOSC Frequency is 16 MHz (32x))

// CONFIG2
#pragma config WRTEN = OFF      // Flash memory self-write protection bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>


//Количестно миганий до передвигания механизма
#define LIGHT_LIMIT 3
//Ширина импульса мигуания
#define TIME_ON_LIGHT 17
//Время через которое наступает неоучет
#define UPGRADE_LIMIT 3600000
#define TIMER_PROG_MODE 30000

//Флаг передвинуть счетный механизм
char turn_flag = 0;
//Количестно прошедших морганий
char light_counter = 0;
//Количество импульсов от плат учета мощности
unsigned int interrupt_counter = 0;
//Количество прерываний 0 тамера
int timer0_conter = 0;
//Количество импульсов до моргания
unsigned int cr_limit = 1710;
//Количество прерываний таймера до недоучета
unsigned long upgrade_timer = 0;
//Время работы микроконтроллера
unsigned long time = 0;
unsigned long start_on_light = 0;
char start_remember_flag = 1;
unsigned long interrupt_time = 0;
char prog_mode = 0;
char prog_init_flag = 0;
unsigned long catch_prog_mode_time = 0;


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
    turn_flag = 0;
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
    interrupt_time = 0;
    start_on_light = 0;
}

void interrupt isr() {  
    //Обработка сигнала от модулей измерения мощности
    if (RABIF) {
        interrupt_counter++;
        if (interrupt_counter == cr_limit) {
            //ВКлючить диод
            PORTCbits.RC0 = 0;
            start_on_light = time;
            interrupt_counter = 0;
            light_counter++;
            if (light_counter == LIGHT_LIMIT) {
                turn_flag = 1;
                light_counter = 0;
            }
        }
        if (prog_mode && !prog_init_flag) {
            if (!catch_prog_mode_time) {
                catch_prog_mode_time = time;
            } else {
                unsigned long delta = time - catch_prog_mode_time;
                if ((delta > 57) && (delta < 90)) {
                    prog_init_flag = 1;
                } else {
                    prog_mode = 0;
                    catch_prog_mode_time = 0;
                }
            }
            
            
        }
        __delay_us(40);
        start_remember_flag = 1;
        
        RABIF = 0;
    } else {
        
    }
    if (TMR2IF) {
       time++;
       TMR2IF = 0; 
    }
}

void main(void) {
    setup();
    while (1) {
        if (turn_flag) {
            turn();
        }
        if ((time - start_on_light >= TIME_ON_LIGHT) && !RC0 && !prog_mode) {
            PORTCbits.RC0 = 1;
        }
        if (time > 4294967200) {
            reset_time();
        }
        if (start_remember_flag) {
            start_remember_flag = 0;
            interrupt_time = time;
        }
        if ((time - interrupt_time > TIMER_PROG_MODE) && !prog_mode) {
            prog_mode = 1;
            PORTCbits.RC0 = 0;
        }
    }
}
