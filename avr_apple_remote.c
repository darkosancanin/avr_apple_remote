#define F_CPU 14745600

#include <inttypes.h>
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define IR_LED_OUTPUT_DDR DDRC
#define IR_LED_OUTPUT_PORT PORTC
#define IR_LED_OUTPUT_PIN PC5

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

#define APPLE_IDENTIFIER 0b0111100000010001
#define REMOTE_ID 0b1
#define MENU_COMMAND 0b11111100
#define PLAY_COMMAND 0b11111010
#define RIGHT_COMMAND 0b11111001
#define LEFT_COMMAND 0b11110110
#define UP_COMMAND 0b11110101
#define DOWN_COMMAND 0b11110011

void send_pulse(int duration_us){
  //the apple remote works by sending pulses at 38kHz, this equates to a pulse
  //every 26 microseconds (13 on / 13 off)
  while(duration_us > 0){		
    //turn led output pin on
    IR_LED_OUTPUT_PORT |= (1<<IR_LED_OUTPUT_PIN);
		
    //delay for 10 microseconds
    _delay_us(10);
		
    //turn led output pin off
    IR_LED_OUTPUT_PORT &= ~(1<<IR_LED_OUTPUT_PIN);
		
    //delay for 16 microseconds
    _delay_us(16);
		
    duration_us -= 26;
  }
}

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

//the NEC protocol that the remote uses expects commands in the following format (time in us): 
//leader: 9000 on, 4500 off
//0 bit: 560 on, 1690 off
//1 bit: 560 on, 565 off
//stop: 560 on
//commands are 32 bits long, in the format : remote id (8bits), command (8 bits), apple identifier (16 bits)
void send_command(uint8_t command){
  long data = REMOTE_ID;
  data = data << 8;
  data += command;
  data = data << 16;
  data += APPLE_IDENTIFIER;
 
  //send leader pulse
  send_pulse(9000);
  _delay_us(4500);
	
  //loop through the 32 bits and send 0 or 1 bit specific pulses
  int count = 0;
  while(count < 32){
    send_pulse(560);
    if(data & 0b1){
      _delay_us(565);
    }
    else{
      _delay_us(1690);
    }
    //right shift the data so we only have to compare the lsb
    data = data >> 1;
    count++;
  }
	
  //send stop pulse
  send_pulse(560);
}

//interrupt handler for the button presses
ISR(PIN_CHANGE_INTERRUPT_ROUTINE_VECTOR) {
  cli(); //disable interrupts
  if(!(BUTTON_PIN & (1<<MENU_BUTTON_OUTPUT_PIN))) {
    send_command(MENU_COMMAND);
  } else if(!(BUTTON_PIN & (1<<PLAY_BUTTON_OUTPUT_PIN))) { 
    send_command(PLAY_COMMAND);
  } else if(!(BUTTON_PIN & (1<<RIGHT_BUTTON_OUTPUT_PIN))) {
    send_command(RIGHT_COMMAND);
  } else if(!(BUTTON_PIN & (1<<LEFT_BUTTON_OUTPUT_PIN))) {
    send_command(LEFT_COMMAND);  
  } else if(!(BUTTON_PIN & (1<<UP_BUTTON_OUTPUT_PIN))) {
    send_command(UP_COMMAND);
  } else if(!(BUTTON_PIN & (1<<DOWN_BUTTON_OUTPUT_PIN))) {
    send_command(DOWN_COMMAND);
  }
  sei(); //enable interrupts
}
