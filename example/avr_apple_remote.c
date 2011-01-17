#include <avr/interrupt.h>
#include "../avr_apple_remote.h"

#define BUTTON_OUTPUT_DDR DDRB
#define BUTTON_OUTPUT_PORT PORTB
#define BUTTON_PIN PINB
#define MENU_BUTTON_OUTPUT_PIN PB0
#define LEFT_BUTTON_OUTPUT_PIN PB1
#define RIGHT_BUTTON_OUTPUT_PIN PB2
#define UP_BUTTON_OUTPUT_PIN PB3
#define DOWN_BUTTON_OUTPUT_PIN PB4
#define PLAY_BUTTON_OUTPUT_PIN PB5

#define PIN_CHANGE_INTERRUPT_ROUTINE_VECTOR SIG_PIN_CHANGE0
#define PIN_CHANGE_INTERRUPT_ENABLE_FLAG PCIE0
#define PIN_CHANGE_INTERRUPT_MASK_REGISTER PCMSK0
#define MENU_BUTTON_PIN_CHANGE_INTERRUPT_FLAG PCINT0
#define LEFT_BUTTON_PIN_CHANGE_INTERRUPT_FLAG PCINT1
#define RIGHT_BUTTON_PIN_CHANGE_INTERRUPT_FLAG PCINT2
#define UP_BUTTON_PIN_CHANGE_INTERRUPT_FLAG PCINT3
#define DOWN_BUTTON_PIN_CHANGE_INTERRUPT_FLAG PCINT4
#define PLAY_BUTTON_PIN_CHANGE_INTERRUPT_FLAG PCINT5

int main(){
  //set the led pins as output in the data direction register
  IR_LED_OUTPUT_DDR |= (1<<IR_LED_OUTPUT_PIN);
	
  //set the buttons as output in the data direction register
  BUTTON_OUTPUT_DDR |= (1<<MENU_BUTTON_OUTPUT_PIN) | (1<<PLAY_BUTTON_OUTPUT_PIN) | (1<<RIGHT_BUTTON_OUTPUT_PIN) | (1<<LEFT_BUTTON_OUTPUT_PIN) | (1<<UP_BUTTON_OUTPUT_PIN) | (1<<DOWN_BUTTON_OUTPUT_PIN);
	
  //turn on pull up registers for the buttons
  BUTTON_OUTPUT_PORT |= (1<<MENU_BUTTON_OUTPUT_PIN) | (1<<PLAY_BUTTON_OUTPUT_PIN) | (1<<RIGHT_BUTTON_OUTPUT_PIN) | (1<<LEFT_BUTTON_OUTPUT_PIN) | (1<<UP_BUTTON_OUTPUT_PIN) | (1<<DOWN_BUTTON_OUTPUT_PIN);
	
  //enable pin change interrupts
  PCICR |= (1<<PIN_CHANGE_INTERRUPT_ENABLE_FLAG);
	
  //enable pin change interrupts for specific pins via the masking register
  PCMSK0 |= (1<<MENU_BUTTON_PIN_CHANGE_INTERRUPT_FLAG) | (1<<PLAY_BUTTON_PIN_CHANGE_INTERRUPT_FLAG) | (1<<RIGHT_BUTTON_PIN_CHANGE_INTERRUPT_FLAG) | (1<<LEFT_BUTTON_PIN_CHANGE_INTERRUPT_FLAG) | (1<<UP_BUTTON_PIN_CHANGE_INTERRUPT_FLAG) | (1<<DOWN_BUTTON_PIN_CHANGE_INTERRUPT_FLAG);
	
  sei();
  
  while(1){}
  return 0;
}

//interrupt handler for the button presses
ISR(PIN_CHANGE_INTERRUPT_ROUTINE_VECTOR) {
  cli(); //disable interrupts
  if(!(BUTTON_PIN & (1<<MENU_BUTTON_OUTPUT_PIN))) {
    avr_apple_remote_menu();
  } else if(!(BUTTON_PIN & (1<<PLAY_BUTTON_OUTPUT_PIN))) { 
    avr_apple_remote_playnpause();
  } else if(!(BUTTON_PIN & (1<<RIGHT_BUTTON_OUTPUT_PIN))) {
    avr_apple_remote_right();
  } else if(!(BUTTON_PIN & (1<<LEFT_BUTTON_OUTPUT_PIN))) {
    avr_apple_remote_left();  
  } else if(!(BUTTON_PIN & (1<<UP_BUTTON_OUTPUT_PIN))) {
    avr_apple_remote_up();
  } else if(!(BUTTON_PIN & (1<<DOWN_BUTTON_OUTPUT_PIN))) {
    avr_apple_remote_down();
  }
  sei(); //enable interrupts
}
