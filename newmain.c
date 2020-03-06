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


#define CR_LIMIT 3320
#define LIGHT_LIMIT 3
#define TIMER0_LIMIT 17
#define UPGRADE_LIMIT 3600000

int turn_flag = 0;
int light_counter = 0;
long interrupt_counter = 0;
int timer0_conter = 0;
long timer2_counter = 0;
int prescaler = 1;
int cr_limit = 3400;
unsigned long upgrade_timer = 0;


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
}

void initTimer2() {
    T2CON = 0b00000111;
    PIE1bits.TMR2IE = 1;
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


void interrupt isr() {  
    if (RABIF) {
        interrupt_counter++;
        if (interrupt_counter == cr_limit) {
            PORTCbits.RC0 = 0;
            TMR0IE = 1;
            TMR0 = 0;
            interrupt_counter = 0;
            light_counter++;
            if (light_counter == LIGHT_LIMIT) {
                turn_flag = 1;
                light_counter = 0;
            }
        }
        RABIF = 0;
    }
    
    if (TMR0IF && TMR0IE) {
        timer0_conter++;
        if (timer0_conter == TIMER0_LIMIT) {
            PORTCbits.RC0 = 1;
            TMR0IE = 0;
            timer0_conter = 0;
        }
        TMR0IF = 0;
    }
    
    if (TMR2IF) {
        upgrade_timer++;
        if (upgrade_timer == UPGRADE_LIMIT) {
            cr_limit = 6800;
            T2CON = 0b00000000;
        }
        TMR2IF = 0;
    }
}

void main(void) {
    setup();
    while (1) {
        if (turn_flag) {
            turn();
            turn_flag = 0;
        }
    }
}
