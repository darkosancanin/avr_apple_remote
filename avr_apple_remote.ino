#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <inttypes.h>
#include <util/delay.h>

#define F_CPU 8000000UL // Set CPU frequency to 8Mhz

// Apple remote specific commands
#define APPLE_IDENTIFIER 0b0111100000010001
#define REMOTE_ID 0b00110001
#define MENU_COMMAND 0b11111100
#define PLAY_COMMAND 0b10100000
#define RIGHT_COMMAND 0b11111001
#define LEFT_COMMAND 0b11110110
#define UP_COMMAND 0b11110101
#define DOWN_COMMAND 0b11110011
#define SELECT_COMMAND 0b10100011

#define ENABLE_IR_LED  TCCR0A |= (1<<COM0A0); // Toggle OC0A/OC0B on Compare Match [Page 78];
#define DISABLE_IR_LED TCCR0A &= ~(1<<COM0A0); // Normal port operation, OC0A/OC0B disconnected [Page 78].

// ADC voltage values of each button on PB4
#define RIGHT_BUTTON_ADC_VALUE 0
#define DOWN_BUTTON_ADC_VALUE 158
#define MENU_BUTTON_ADC_VALUE 367
// ADC voltage values of each button on PB3
#define UP_BUTTON_ADC_VALUE 0
#define LEFT_BUTTON_ADC_VALUE 158
#define SELECT_BUTTON_ADC_VALUE 367
#define BUTTON_ADC_VARIANCE_ALLOWED 50 // Sets the variance allowed for the ADC conversion matches for the button presses

// Pulse length manual adjustment. Manual adjustment made if internal clock is not accurate. 
// This needs to be set back to standard values when a external crystal is used.
#define PULSE_LENGTH_9600 9200
#define PULSE_LENGTH_4500 4600
#define PULSE_LENGTH_560 570
#define PULSE_LENGTH_565 580
#define PULSE_LENGTH_1690 1750

int main(){
    DDRB |= (1 << DDB1) | (1 << DDB0); // Set pin PB0 as output. OC0A is on PB0. Setting PB1 as output for status LED.
    TCNT0 = 0; // Set the counter to 0
    TCCR0A = 0; // Initialize the Timer/Counter Control Register A
    TCCR0B = 0; // Initialize the Timer/Counter Control Register B

    TCCR0A |=(1 << WGM01); // Set Waveform Generation Mode to be Clear Timer on Compare Match (CTC) mode [Page 79]
    TCCR0B |= (1 << CS00); // Set the Prescaler to be 'clkI/O/(No prescaling)' [Page 80]
    OCR0A = 104; // Set the CTC compare value at which to toggle the OC0A/PB0 pin.
    
    // ADC Prescaler needs to be set so that the ADC input frequency is between 50 - 200kHz. [Page 125]
    ADCSRA |= (1 << ADPS1) | (1 << ADPS2); // Set the prescalar to be 64 which is 125kHz with a 8Mhz clock [Page 136] 
    ADCSRA |= (1 << ADEN); // Enable the ADC [Page 136]
    
    GIMSK |= (1 << PCIE); // Enable pin change interrupts on the General Interrupt Mask Register [Page 51]
    PCMSK |= (1 << PCINT4) | (1 << PCINT3); // Enable interrupts on PB3 and PB4 on the Pin Change Mask Register
    
    MCUCR |= (1 << SM1); // Sets the sleep mode to the most efficient 'Power-down'. [Page 35]
    MCUCR |= (1 << SE); // Sleep Enable. Must be enable the sleep instruction to be executed. [Page 37]
    WDTCR = 0; // Turn off the unused watchdog timer to reduce power consumption. [Page 37, 46]
    PRR &= ~(1 << PRUSI); // Power Reduction USI. Shuts down the unused USI module. [Page 38]
    
    sei(); // Enables interrupts
    
    while(1){
        ADCSRA &= ~(1 << ADEN); // Disable ADC, saves ~230uA, it gets re-enabled in the interrupt handler
        sleep_bod_disable(); // Soft disable the brown out detector, it automatically gets enabled on wake up [Page 35]
        sleep_cpu();
    }
    return 0;
}

// The NEC protocol that the remote uses expects commands in the following format (time in us): 
// leader: 9000 on, 4500 off
// 0 bit: 560 on, 1690 off
// 1 bit: 560 on, 565 off
// stop: 560 on
// commands are 32 bits long, in the format : remote id (8bits), command (8 bits), apple identifier (16 bits)
void send_command(uint8_t command){
    PORTB |= (1 << PORTB1); // Turn on status LED
  
    long data = REMOTE_ID;
    data = data << 8;
    data += command;
    data = data << 16;
    data += APPLE_IDENTIFIER;

    // Send leader pulse
    ENABLE_IR_LED;
    _delay_us(PULSE_LENGTH_9600);
    DISABLE_IR_LED;
    _delay_us(PULSE_LENGTH_4500);
    
    // Loop through the 32 bits and send 0 or 1 bit specific pulses
    int count = 0;
    while(count < 32){
        ENABLE_IR_LED;
        _delay_us(PULSE_LENGTH_560);
        DISABLE_IR_LED;
        if(data & 0b1){
            _delay_us(PULSE_LENGTH_565);
        }
        else{
            _delay_us(PULSE_LENGTH_1690);
        }
        // Right shift the data so we only have to compare the LSB
        data = data >> 1;
        count++;
    }
    
    // Send stop pulse
    ENABLE_IR_LED; 
    _delay_us(PULSE_LENGTH_560);
    DISABLE_IR_LED;
    
    PORTB &= ~(1 << PORTB1); // Turn off status LED
}

// Pin change interrupt handler
ISR(PCINT0_vect)
{
    ADCSRA |= (1<<ADEN); //Enable ADC

    if (!(PINB & (1 << PINB4))) { // If PB4 is low then check if its buttons 1 to 3.
        ADMUX = (1 << MUX1); // Connect PB4/ADC2 to the ADC [Page 135]
        ADCSRA |= (1 << ADSC); // Start the ADC measurement
        while (ADCSRA & (1 << ADSC)); // Wait until the conversion completes
        uint16_t adc_value = ADC; // Get the digital value
    
        if ( adc_value < (RIGHT_BUTTON_ADC_VALUE + BUTTON_ADC_VARIANCE_ALLOWED) ){
            send_command(RIGHT_COMMAND);
        } else if ( (adc_value > (DOWN_BUTTON_ADC_VALUE - BUTTON_ADC_VARIANCE_ALLOWED)) && (adc_value < (DOWN_BUTTON_ADC_VALUE + BUTTON_ADC_VARIANCE_ALLOWED)) ) {
            send_command(DOWN_COMMAND);
        } else if ( (adc_value > (MENU_BUTTON_ADC_VALUE - BUTTON_ADC_VARIANCE_ALLOWED)) && (adc_value < (MENU_BUTTON_ADC_VALUE + BUTTON_ADC_VARIANCE_ALLOWED)) ) {
            send_command(MENU_COMMAND);
        }
    } else { // Otherwise check if its button 4 to 6
        ADMUX = (1 << MUX1) | (1 << MUX0); // Connect PB3/ADC3 to the ADC [Page 135]
        ADCSRA |= (1 << ADSC); // Start the ADC measurement
        while (ADCSRA & (1 << ADSC)); // Wait until the conversion completes
        uint16_t adc_value = ADC; // Get the digital value
        
        if ( adc_value < (UP_BUTTON_ADC_VALUE + BUTTON_ADC_VARIANCE_ALLOWED) ) {
            send_command(UP_COMMAND);
        } else if ( (adc_value > (LEFT_BUTTON_ADC_VALUE - BUTTON_ADC_VARIANCE_ALLOWED)) && (adc_value < (LEFT_BUTTON_ADC_VALUE + BUTTON_ADC_VARIANCE_ALLOWED)) ) {
            send_command(LEFT_COMMAND);
        } else if ( (adc_value > (SELECT_BUTTON_ADC_VALUE - BUTTON_ADC_VARIANCE_ALLOWED)) && (adc_value < (SELECT_BUTTON_ADC_VALUE + BUTTON_ADC_VARIANCE_ALLOWED)) ) {
            send_command(SELECT_COMMAND);
        }
    }
    
    _delay_us(100000); // Delay in place to stop multiple unintended presses
}