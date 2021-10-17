/*
 * heisser-draht.c
 *
 * Created: 2020-10-19T09:50:40
 * Author : dschemp
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#define MAX_TRIES 3
#define SEVEN_SEG_REP_LENGTH 10
#define LED_REP_LENGTH 7

unsigned char seven_seg_rep[SEVEN_SEG_REP_LENGTH] = {
	0b00000001, // 0
	0b10011011, // 1
	0b00100010, // 2
	0b00001010, // 3
	0b10011000, // 4
	0b01001000, // 5
	0b01000000, // 6
	0b00011011, // 7
	0b00000000, // 8
	0b00001000  // 9
};

unsigned char led_level;
unsigned char seven_segment_level;
unsigned char display_selection;

void setup(void);
void init_timer(void);

void set_seven_seg(unsigned char);
void set_led_level(unsigned char);
void set_led_mask(unsigned char);
void set_buzzer(unsigned char);
void show_smiley(void);

int main(void)
{
	setup();
	
	/*
		Read values:			PINA / PINB
		Setting output pins:	PORTA / PORTB
	*/
	
	led_level = 0b11;
	
	while (1) {
		//set_led_level(led_level);
		//show_smiley();
	}
}

void setup(void) {
	/*
		Help:
			(o) => Output
			(i) => Input
			[7S] => 7-Segment-Pin
			(-) / (+) => state required for corresponding item to activate
	*/
	
	// ==================== PIN SETTINGS ====================
	
	/*	Configure PORTA:
		================
		PA0 - (o) [7S] g
		PA1 - (o) [7S] f
		PA2 - (i) INT1 / "The Hot Wire"
		PA3 - (o) [7S] e | LED1 (red)
		PA4 - (o) [7S] d | LED2 (yellow)
		PA5 - (o) [7S] c | LED3 (yellow)
		PA6 - (o) [7S] b | LED4 (green)
		PA7 - (o) [7S] a | LED5 (green)
	*/
	DDRA |= 0b11111011; // 1 = output, 0 = input
	PORTA = (1 << PA2); // Pull-Up for wire

	/*	Configure PORTB:
		================
		PB0 - ?
		PB1 - ?
		PB2 - ?
		PB3 - (o) Switcher LEDs (-) / 7-Segment (+)
		PB4 - (o) Buzzer
		PB5 - not connected
		PB6 - (i) Start Button / Pull-Up / Finish Wire
		PB7 - ?
	*/
	DDRB |= 0b00011000; // 1 = output, 0 = input
	PORTB = (1 << PB6); // set start button to pull-up
	
	// ===================== INTERRUPTS =====================
	
	// Enable interrupts for INT0 (Reset Button) and INT1 (wire?)
	MCUCR |= (1 << ISC01); // Set 'perform interrupt request' to 'falling edge' on INT0 or INT1
	GIMSK |= (1 << INT1) | (1 << INT0); // Enable interrupts INT0 and INT1
	sei(); // Global Interrupt Enable
	
	// ================== GLOBAL VARIABLES ==================
	
	led_level = 0;
	seven_segment_level = 0;
	
	// ======================= TIMERS =======================
	
	init_timer();
}

void init_timer(void) {
	// =============== DISPLAY UPDATE TIMER =================
	
	PLLCSR |= (1 << LSM); // set to low speed mode to reduce energy consumption
	TCCR1B |= (1 << CS12) | (1 << CS11); // Set prescaler to 32
	TIMSK |= (1 << TOIE1); // Enable Overflow Interrupt Enable for Timer1
	sei(); // Enable Global Interrupt Enable
}

void set_seven_seg(unsigned char n) {
	if (n > SEVEN_SEG_REP_LENGTH) {
		return;
	}
	
	// Switch to 7-Seg Mode
	PORTB |= (1 << PB3);
	// Switch outputs
	PORTA |= seven_seg_rep[n];
}

void set_led_level(unsigned char level) {
	if (level > 6) {
		return;
	}
	
	// Switch outputs
	if (level != 0) {
		led_level = (1 << level);
	}
}

void set_led_mask(unsigned char led_mask) {
	if (led_mask > 0b11111) {
		return;
	}
	
	// Switch to LED Mode
	PORTB &= ~(1 << PB3);
	// Switch outputs
	PORTA = (led_mask << 3) | (PORTA & 0b111);
}

void set_buzzer(unsigned char mode) {
	if (mode) {
		PORTB |= (1 << PB4);
	} else {
		PORTB &= ~(1 << PB4);
	}
}

void show_smiley(void) {
	// Switch to 7-Seg Mode
	PORTB |= (1 << PB3);
	// Reset all output signals
	PORTA &= (1 << PA2);
	// Smiley
	PORTA |= (1 << PA6) | (1 << PA1) | (1 << PA0);
}

// interrupt on reset / start button
ISR(INT0_vect) {
	display_selection ^= 1;
	
	if (display_selection) {
		led_level = 0b01010;
	} else {
		led_level = 0b10101;
	}
}

// interrupt on wire
ISR(INT1_vect) {
	led_level = 0b11001;
}

// update display
ISR(TIMER1_OVF_vect) {
	display_selection ^= 1; // switch between led and seven segment display
	if (display_selection) {
		// LEDs
		set_led_level(led_level);
	} else {
		// 7-Segment
		set_seven_seg(seven_segment_level);
	}
}