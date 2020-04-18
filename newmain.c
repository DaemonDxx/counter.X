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


//Количестно миганий до передвигания механизма
#define LIGHT_LIMIT 3
//Ширина импульса мигуания
#define TIME_ON_LIGHT 17
//Время через которое наступает неоучет
#define UPGRADE_LIMIT 3600000

#define DEVICE_ID 120
#define INIT_BYTE 180
#define TIME_INIT_BYTE_DELAY 1000
#define TIMEOUT_RECIVE_BIT 5000

//Количестно прошедших морганий
char light_counter = 0;
//Количество импульсов от плат учета мощности
unsigned long interrupt_counter = 0;
//Количество прерываний 0 тамера
int timer0_conter = 0;
//Количество импульсов до моргания
unsigned long cr_limit = 437000;
//Количество прерываний таймера до недоучета
unsigned long upgrade_timer = 0;
//Время работы микроконтроллера
unsigned long time = 0;
unsigned long start_on_light = 0;
char light_flag = 0;
char buffer = 0;

char last_port_A = 0b00000000;
char current_port_A = 0b00000000;
char array_index = 0b00000000;
char status_interrupt_io = 0;

char i = 0;
char init_byte_receive = 0;
unsigned long receive_data = 0;
unsigned long start_init_byte_time = 0;
char receive_byte = 0;
unsigned long time_to_last_recived_bit = 0;
bit isReceivingData = 0;

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
    interrupt_weight[0b00000001] = 1000;
    interrupt_weight[0b00000010] = 1000;
    interrupt_weight[0b00000100] = 995;
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
        status_interrupt_io |= array_index&0b00000111;
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

bit getReceiveBit() {
    if (status_interrupt_io & 0b00000011) {
        return 1;
        } else {
        return 0;
        }
}

bit isReceivedData() {
   if (status_interrupt_io & 0b00000100) {
       return 1;
   } else {
       return 0;
   }
}

void resetReceivedData() {
    init_byte_receive = 0;
    i = 0;
    time_to_last_recived_bit = 0;
    isReceivingData = 0;
    receive_data = 0;
    receive_byte = 0;
}

char parseDevideId() {
    
}

unsigned int parseData() {
    
}

void setOption() {
    
}

void checkTimeoutReceiveBit() {
    if (time - time_to_last_recived_bit > TIMEOUT_RECIVE_BIT) {
        resetReceivedData();
    }
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
        //Проверяем, если данных не было более n секунд, то сбрасываем все принятые данные
        checkTimeoutReceiveBit();
        if (isReceivedData()) {
            
            //Если init_byte еще нет, то формируем его
            if (!init_byte_receive) {
                receive_byte |= getReceiveBit() << i;
                i++;
                if (i == 8) {
                    if (receive_byte == INIT_BYTE) {
                        init_byte_receive = receive_byte;
                    }
                    i = 0;
                }
            //Если init_byte есть, и чтение данных не началось, проверяем паузу
            } else if (!isReceivingData) {
                if (time - time_to_last_recived_bit > TIME_INIT_BYTE_DELAY) {
                    isReceivingData = 1;
                } else {
                    resetReceivedData();
                }
            }
            
            //Если идет чтение данных, то читаем;
            if (isReceivingData) {
                receive_data |= getReceiveBit() << i;
                i++;
                if (i == 24) {
                    if (parseDevideId() == DEVICE_ID) {
                        setOption();
                    }
                    resetReceivedData();
                }
            }
            
            time_to_last_recived_bit = time;
            __delay_ms(4);
            status_interrupt_io = 0;
        }
    }
}
