#include "rtos.h"

#define MOTOR_TASK_DELAY (1500)
#define MOTOR_TASK_SEQUENCE_DELAY (100)

#define DISPLAY_REFRESH_FREQ (5)
#define DISPLAY_NUM (4)
#define MSB_MASK (0x80)

#define UART_BAUD_REGISTER_VALUE (103)
#define UPER_BYTE_SHIFT_RIGHT_AMOUNT (8)
#define UPER_BYTE_MASK (0xFF)

#define ADC_ADMUX (0x00)
#define ADC_ADCSRA (0x87)
#define ADC_ADSC_BIT (0x40)
#define SENSOR_TASK_DELAY (250)

#define TEMP_HIGH_THRESH (30)
#define TEMP_LOW_THRESH (10)

static uint16_t motorTurnCount = 0;
static uint16_t temperatureCelsius = 0;
static uint8_t displayIndex = 0;

const uint8_t numberCodesFor7SEG[] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };

uint8_t number[DISPLAY_NUM];

void MotorTask(void) {
    
    for(;;) {
        
        PORTB |= _BV(PORTB3); PORTB &= ~(_BV(PORTB2)); PORTB &= ~(_BV(PORTB1)); PORTB &= ~(_BV(PORTB0));
        
        SimpleRTOS_DelayTask(MOTOR_TASK_SEQUENCE_DELAY);
        
        PORTB &= ~(_BV(PORTB3)); PORTB &= ~(_BV(PORTB2)); PORTB |= _BV(PORTB1); PORTB &= ~(_BV(PORTB0));
        
        SimpleRTOS_DelayTask(MOTOR_TASK_SEQUENCE_DELAY);
        
        PORTB &= ~(_BV(PORTB3)); PORTB |= _BV(PORTB2); PORTB &= ~(_BV(PORTB1)); PORTB &= ~(_BV(PORTB0));
        
        SimpleRTOS_DelayTask(MOTOR_TASK_SEQUENCE_DELAY);
        
        PORTB &= ~(_BV(PORTB3)); PORTB &= ~(_BV(PORTB2)); PORTB &= ~(_BV(PORTB1)); PORTB |= _BV(PORTB0);

        ++motorTurnCount;

        SimpleRTOS_DelayTask(MOTOR_TASK_DELAY);
        
    }
    
}

void displayDigit(uint8_t digit)
{
    PORTD &= ~(_BV(PORTD5)); 
    
    for(int i = 0; i < 8; ++i) {

        PORTD &= ~(_BV(PORTD7));

	    if(digit & MSB_MASK)
	        PORTD |= _BV(PORTD6);
	    else
	        PORTD &= ~(_BV(PORTD6));
	
	    digit = digit << 1;
 
               
        PORTD |= _BV(PORTD7);
    } 
            
    PORTD |= _BV(PORTD5);     
}


void DisplayTask(void) {
    
    for(;;) {
        
        ++displayIndex;
	    if(displayIndex > DISPLAY_NUM)
            displayIndex = 1;

        number[0] = motorTurnCount / 1000;
        number[1] = (motorTurnCount / 100) % 10;
        number[2] = (motorTurnCount / 10) % 10;
        number[3] = motorTurnCount % 10;

        switch (displayIndex) {
            case 1:
                PORTD &= ~(_BV(PORTD4)); // A
                PORTD &= ~(_BV(PORTD3)); // B
                displayDigit(numberCodesFor7SEG[number[0]]);
            break;
            case 2:
                PORTD |= _BV(PORTD4);    // A
                PORTD &= ~(_BV(PORTD3)); // B
                displayDigit(numberCodesFor7SEG[number[1]]);
            break;
            case 3:
                PORTD &= ~(_BV(PORTD4)); // A
                PORTD |= _BV(PORTD3);    // B
                displayDigit(numberCodesFor7SEG[number[2]]);
            break;
            case 4:
                PORTD |= _BV(PORTD4);    // A
                PORTD |= _BV(PORTD3);    // B
                displayDigit(numberCodesFor7SEG[number[3]]);     
            break;    
        }
        
        SimpleRTOS_DelayTask(DISPLAY_REFRESH_FREQ);
        
    }
    
}

void SensorTask(void) {
    for(;;) {

        ADCSRA |= ADC_ADSC_BIT;

        while(ADCSRA & ADC_ADSC_BIT) {
            ;
        }

        temperatureCelsius = ADCL | (ADCH << 8);

        temperatureCelsius = (temperatureCelsius / 1024.0) * 500;

        if(temperatureCelsius > TEMP_HIGH_THRESH)
            PORTB |= _BV(PORTB5);
        else
            PORTB &= ~(_BV(PORTB5));

        if(temperatureCelsius < TEMP_LOW_THRESH)
            PORTD |= _BV(PORTD2);
        else
            PORTD &= ~(_BV(PORTD2));
        
        SimpleRTOS_DelayTask(SENSOR_TASK_DELAY);
    }
}

void InitPins(void) {
    DDRB |= _BV(DDB5); // PIN 13 for LED
    DDRB |= _BV(DDB4); // PIN 12 for step motor enable
    DDRB |= _BV(DDB3); // PIN 11 for step motor in1
    DDRB |= _BV(DDB2); //  PIN 10 for step motor in2
    DDRB |= _BV(DDB1); //  PIN 9 for step motor in3
    DDRB |= _BV(DDB0); //  PIN 8 for step motor in4
 
    DDRD |= _BV(DDD7); // PIN 7 for shift register clock
    DDRD |= _BV(DDD6); // PIN 6 for shift register data
    DDRD |= _BV(DDD5); // PIN 5 for shift register latch
    DDRD |= _BV(DDD4); // PIN 4 for decoder input A, segment select for 7SEG
    DDRD |= _BV(DDD3); // PIN 3 for decoder input B, segment select for 7SEG
    DDRD |= _BV(DDD2); // PIN 2 for LED
 
    PORTB |= _BV(PORTB4); // motor driver enable
    
    ADMUX &= ADC_ADMUX; // init adc

    ADCSRA |= ADC_ADCSRA; // enable adc

}

int main(int argc, char *argv[]) {
    
    InitPins();

    SimpleRTOS_Init();
    
    SimpleRTOS_CreateTask("SensorTask", 3, SensorTask);

    SimpleRTOS_CreateTask("MotorTask", 2, MotorTask);
    
    SimpleRTOS_CreateTask("DisplayTask",1, DisplayTask);
  
    SimpleRTOS_Start();
    
    return 0;
}
