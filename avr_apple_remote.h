#define F_CPU 14745600

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>

#define IR_LED_OUTPUT_DDR DDRC
#define IR_LED_OUTPUT_PORT PORTC
#define IR_LED_OUTPUT_PIN PC5

#define APPLE_IDENTIFIER 0b0111100000010001
#define REMOTE_ID 0b1
#define MENU_COMMAND 0b11111100
#define PLAY_COMMAND 0b11111010
#define RIGHT_COMMAND 0b11111001
#define LEFT_COMMAND 0b11110110
#define UP_COMMAND 0b11110101
#define DOWN_COMMAND 0b11110011

void avr_apple_remote_menu();
void avr_apple_remote_playnpause();
void avr_apple_remote_left();
void avr_apple_remote_right();
void avr_apple_remote_up();
void avr_apple_remote_down();
