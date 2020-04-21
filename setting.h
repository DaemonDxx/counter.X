//Количестно миганий до передвигания механизма
#define LIGHT_LIMIT 3
//Ширина импульса мигуания
#define TIME_ON_LIGHT 17
//Время через которое наступает неоучет
#define UPGRADE_LIMIT 3600000

#define DEVICE_ID 120
#define TIME_INIT_BYTE_DELAY 1000
#define TIMEOUT_RECIVE_BIT 5000

//Колибровочные константы по фазам
#define INTERRUPT_WEIGTH_PHASE_A 995
#define INTERRUPT_WEIGTH_PHASE_B 1000
#define INTERRUPT_WEIGTH_PHASE_C 1000


//Количество импульсов до моргания
unsigned long cr_limit = 437000;