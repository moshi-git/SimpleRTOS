# SimpleRTOS
Simple RTOS for Arduino UNO and NANO

# Compile using avr-gcc:
1. avr-gcc -Os -DF_CPU=16000000UL -mmcu=atmega328p -c -o rtos.o rtos.c
2. avr-gcc -Os -DF_CPU=16000000UL -mmcu=atmega328p -c -o main.o main.c
3. avr-gcc -mmcu=atmega328p main.o rtos.o -o rtos
4. avr-objcopy -O ihex -R .eeprom rtos rtos.hex
