// ************* Implementation Overview ************* 
// The Apple Remote uses an NEC IR protocol which consists of a differential PPM 
// encoding on a 1:3 duty cycle 38 kHz 950 nm infrared carrier. [http://en.wikipedia.org/wiki/Apple_Remote]
// This code uses a 50% duty cycle for simplicity which still works.
// PWM is done using 'Clear Timer on Compare Match (CTC) Mode'. [Page 72] 11.7.2 describes CTC mode.
// Output is on 0C0A which is PB0. The internal clock is set to run at 8Mhz.
// No prescaler is used. @8Mhz 1 tick = .000000125 secs, 38khz is a cycle every .000026 secs. 
// This means there is 208 ticks required for a full cycle (.000026/.000000125). OCR0A is set to half of this at 104.
// This is half a full cycle, meaning it will be off for a half cycle, toggled due to COM0A0, and then on for a half cycle.
// We enable/disable PWM by enabling toggle on compare mode and then disconnecting OC0A and assuming normal port operation. [Page 78]

#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>

#define F_CPU 8000000 // Set CPU frequency to 8Mhz

// Apple remote specific commands
#define APPLE_IDENTIFIER 0b0111100000010001
#define REMOTE_ID 0b1
#define MENU_COMMAND 0b11111100
#define PLAY_COMMAND 0b11111010
#define RIGHT_COMMAND 0b11111001
#define LEFT_COMMAND 0b11110110
#define UP_COMMAND 0b11110101
#define DOWN_COMMAND 0b11110011

#define ENABLE_PWM	TCCR0A |= (1<<COM0A0) // Toggle OC0A/OC0B on Compare Match [Page 78]
#define DISABLE_PWM	TCCR0A &= ~(1<<COM0A0) // Normal port operation, OC0A/OC0B disconnected [Page 78]

void setup(){
	DDRB |= (1 << DDB0); // Set pin PB0 as output. OC0A is on PB0
	TCNT0 = 0; // Set the counter to 0
	TCCR0A = 0; // Initialize the Timer/Counter Control Register A
	TCCR0B = 0; // Initialize the Timer/Counter Control Register B

	TCCR0A |=(1 << WGM01); // Set Waveform Generation Mode to be Clear Timer on Compare Match (CTC) mode [Page 79]
	TCCR0B |= (1 << CS00); // Set the Prescaler to be 'clkI/O/(No prescaling)' [Page 80]
	OCR0A = 104; // Set the CTC compare value at which to toggle the OC0A/PB0 pin.
	
	GIMSK |= (1 << PCIE); // Enable pin change interrupts on the General Interrupt Mask Register [Page 51]
	PCMSK |= (1 << PCINT4); // Enable interrupts on PB4 on the Pin Change Mask Register
        sei(); // Enables interrupts
}

void loop(){
	send_command(MENU_COMMAND);
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
	ENABLE_PWM;
	_delay_us(9000);
	DISABLE_PWM;
	_delay_us(4500);
	
	//loop through the 32 bits and send 0 or 1 bit specific pulses
	int count = 0;
	while(count < 32){
		ENABLE_PWM;
		_delay_us(560);
		DISABLE_PWM;
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
	ENABLE_PWM;
	_delay_us(560);
	DISABLE_PWM;
}

// Pin change interrupt handler
ISR(PCINT0_vect)
{
	cli(); // Disable interrupts
	send_command(RIGHT_COMMAND);
    sei(); // Enable interrupts
}
