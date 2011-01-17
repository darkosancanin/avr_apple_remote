#include "avr_apple_remote.h"

void send_pulse(int duration_us){
  //the apple remote works by sending pulses at 38kHz, this equates to a pulse
  //every 26 microseconds (10 on / 16 off)
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

void avr_apple_remote_menu(){
  send_command(MENU_COMMAND);
}

void avr_apple_remote_playnpause(){
  send_command(PLAY_COMMAND);
}

void avr_apple_remote_left(){
  send_command(LEFT_COMMAND); 
}

void avr_apple_remote_right(){
  send_command(RIGHT_COMMAND);
}

void avr_apple_remote_up(){
  send_command(UP_COMMAND);
}

void avr_apple_remote_down(){
  send_command(DOWN_COMMAND);
}
