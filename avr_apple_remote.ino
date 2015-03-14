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
// The buttons are setup in a voltage ladder (http://en.wikipedia.org/wiki/Voltage_ladder). They are triggered via interrupts on PB4 
// and each button is identified via a ADC conversion.

// ATtiny85, running @ 8MHZ
//                             +-\/-+
//                       PB5  1|    |8   VCC
//   Buttons 4-5 (ADC3)  PB3  2|    |7   PB2
//   Buttons 1-3 (ADC2)  PB4  3|    |6   PB1
//                       GND  4|    |5   PB0  (OC0A)   IR LED
//                             +----+

// Voltage ladder values (using 10 bit ADC resolution) on PB4
// UP:    0.00V @ 5V / 0.00V @ 3V / 0 (1.8K)
// RIGHT: 0.77V @ 5V / 0.46V @ 3V / 158 (330R)
// DOWN:  1.78V @ 5V / 1.08V @ 3V / 367 (680R)

// Voltage ladder values (using 10 bit ADC resolution) on PB3
// LEFT:  0.00V @ 5V / 0.00V @ 3V / 0 (1.8K)
// MENU:  0.77V @ 5V / 0.46V @ 3V / 158 (330R)

#include <avr/interrupt.h>
#include <avr/io.h>
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

#define ENABLE_IR_LED  TCCR0A |= (1<<COM0A0) // Toggle OC0A/OC0B on Compare Match [Page 78]
#define DISABLE_IR_LED TCCR0A &= ~(1<<COM0A0) // Normal port operation, OC0A/OC0B disconnected [Page 78]

// ADC voltage values of each button on PB4
#define UP_BUTTON_ADC_VALUE 0
#define RIGHT_BUTTON_ADC_VALUE 158
#define DOWN_BUTTON_ADC_VALUE 367
// ADC voltage values of each button on PB3
#define LEFT_BUTTON_ADC_VALUE 0
#define SELECT_BUTTON_ADC_VALUE 158
#define MENU_BUTTON_ADC_VALUE 367
#define BUTTON_ADC_VARIANCE_ALLOWED 50 // Sets the variance allowed for the ADC conversion matches for the button presses

// Pulse length manual adjustment. Manual adjustment made if internal clock is not accurate. 
// This needs to be set back to standard values when a external crystal is used.
#define PULSE_LENGTH_9600 9200
#define PULSE_LENGTH_4500 4700
#define PULSE_LENGTH_560 570
#define PULSE_LENGTH_565 580
#define PULSE_LENGTH_1690 1750

int main(){
    DDRB |= (1 << DDB0); // Set pin PB0 as output. OC0A is on PB0
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
    
    //PORTB |= (1 << PINB3) | (1 << PINB4); // Turn on pull up resistors for both PB3 and PB4
    sei(); // Enables interrupts
    
    while(1){}
    return 0;
}

// The NEC protocol that the remote uses expects commands in the following format (time in us): 
// leader: 9000 on, 4500 off
// 0 bit: 560 on, 1690 off
// 1 bit: 560 on, 565 off
// stop: 560 on
// commands are 32 bits long, in the format : remote id (8bits), command (8 bits), apple identifier (16 bits)
void send_command(uint8_t command){
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
}

// Pin change interrupt handler
ISR(PCINT0_vect)
{
    GIMSK &= ~(1 << PCIE); // Disable interrupts while handling the interrupt
    
    if (!(PINB & (1 << PINB4))) { // If PB4 is low
        ADMUX = (1 << MUX1); // Connect PB4/ADC2 to the ADC [Page 135]
        ADCSRA |= (1 << ADSC); // Start the ADC measurement
        while (ADCSRA & (1 << ADSC)); // Wait until the conversion completes
        uint16_t adc_value = ADC; // Get the digital value
    
        if ( adc_value < (UP_BUTTON_ADC_VALUE + BUTTON_ADC_VARIANCE_ALLOWED) ){
            send_command(UP_COMMAND);
        } else if ( (adc_value > (RIGHT_BUTTON_ADC_VALUE - BUTTON_ADC_VARIANCE_ALLOWED)) && (adc_value < (RIGHT_BUTTON_ADC_VALUE + BUTTON_ADC_VARIANCE_ALLOWED)) ) {
            send_command(RIGHT_COMMAND);
        } else if ( (adc_value > (DOWN_BUTTON_ADC_VALUE - BUTTON_ADC_VARIANCE_ALLOWED)) && (adc_value < (DOWN_BUTTON_ADC_VALUE + BUTTON_ADC_VARIANCE_ALLOWED)) ) {
            send_command(DOWN_COMMAND);
        }
    } else {
        ADMUX = (1 << MUX1) | (1 << MUX0); // Connect PB3/ADC3 to the ADC [Page 135]
        ADCSRA |= (1 << ADSC); // Start the ADC measurement
        while (ADCSRA & (1 << ADSC)); // Wait until the conversion completes
        uint16_t adc_value = ADC; // Get the digital value
        
        if ( adc_value < (LEFT_BUTTON_ADC_VALUE + BUTTON_ADC_VARIANCE_ALLOWED) ) {
            send_command(LEFT_COMMAND);
        } else if ( (adc_value > (SELECT_BUTTON_ADC_VALUE - BUTTON_ADC_VARIANCE_ALLOWED)) && (adc_value < (SELECT_BUTTON_ADC_VALUE + BUTTON_ADC_VARIANCE_ALLOWED)) ) {
            send_command(SELECT_COMMAND);
        } else if ( (adc_value > (MENU_BUTTON_ADC_VALUE - BUTTON_ADC_VARIANCE_ALLOWED)) && (adc_value < (MENU_BUTTON_ADC_VALUE + BUTTON_ADC_VARIANCE_ALLOWED)) ) {
            send_command(MENU_COMMAND);
        }
    }
    
    _delay_us(100000); // Delay in place to stop multiple unintended presses
    GIMSK |= (1 << PCIE); // Enable interrupts
    GIFR = (1<<PCIF); // Clear pin change interrupt flag. [Page 52]
}

