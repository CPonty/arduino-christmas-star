// Christmas Light Star driver
//   Written by Chris Ponticello, 2013 
//   christopher.ponticello@uqconnect.edu.au
//
// Hardware: 
//   Arduino UNO
//   6 LEDs (PWM control)
//   USB-Serial connection
//
// LEDs, clockwise order:
//   LED Pin#
//   1   10
//   2   11
//   3    6
//   4    9
//   5    2
//   6    5
//
// Patterns:
// TEST PATTERN (6)
//     100000 ==> 0x20
//     010000 ==> 0x10
//     001000 ==> 0x08
//     000100 ==> 0x04
//     000010 ==> 0x02
//     000001 ==> 0x01
// OFF (1)
//     000000 ==> 0x00
// ON (1)
//     111111 ==> 0x3F
// FADE/FLASH (2)
//     111111 ==> 0x3F
//     000000 ==> 0x00
// ROTATING LINE (3)
//     100100 ==> 0x24
//     010010 ==> 0x12
//     001001 ==> 0x09
// COUNT UP/DN (12)
//     000000 ==> 0x00
//     100000 ==> 0x20
//     110000 ==> 0x30
//     111000 ==> 0x38
//     111100 ==> 0x3C
//     111110 ==> 0x3E
//     111111 ==> 0x3F
//     011111 ==> 0x1F
//     001111 ==> 0x0F
//     000111 ==> 0x07
//     000011 ==> 0x03
//     000001 ==> 0x01
// REVERSED ROTATION (6) --doesn't look that good, don't use
//     100001 ==> 0x24
//     010010 ==> 0x12
//     001100 ==> 0x0C
//     001100 ==> 0x0C
//     010010 ==> 0x12
//     100001 ==> 0x24
// MEXICAN WAVE (6)
//     111000 ==> 0x38
//     011100 ==> 0x1C
//     001110 ==> 0x0E
//     000111 ==> 0x07
//     100011 ==> 0x23
//     110001 ==> 0x31
//////////////////////
//     000000 ==> 0x00
//-----------------------------------------------------------------------------

#include <stdint.h>

#define TEST 1

#define nLEDs 6 // 6 LEDs connected to the Arduino
#define TCYCLE 2000; // multiply TCYLCE by the #states in a lighting pattern 
					 // indicates how long the pattern should run for

char pins_gnd[nLEDs +1] = {13,12,8,7,4,2, -1};
char pins_pwm[nLEDs +1] = {10,11,6,9,3,5, -1};

// Bitmasks of 0b00xxxxxx, where the lower 6 bits indicate the active LEDs
// Signed value; negative value (MSB=1) indicates end-of-array
char pattern_off[1 +1] = {0x00, -1};
char pattern_on[1 +1] = {0x3F, -1};
char pattern_flash[2 +1] = {0x3F, 0x00, -1};
char pattern_rotate[3 +1] = {0x24, 0x12, 0x09, -1};
char pattern_count[nLEDs*2 +1] = {0x00, 0x20, 0x30, 0x38, 0x3C, 0x3E, 0x3F, 
	0x1F, 0x0F, 0x07, 0x03, 0x01, -1};
// char pattern_reverse[nLEDs +1] = {0x24, 0x12, 0x0C, 0x0C, 0x12, 0x24, -1};
char pattern_mexican[nLEDs +1] = {0x38, 0x1C, 0x0E, 0x07, 0x23, 0x31, -1};
char pattern_test[nLEDs +1] = {0x20, 0x10, 0x08, 0x04, 0x02, 0x01, -1};

 char * patterns[7 +1] = {pattern_test, pattern_off, pattern_on, pattern_flash,
	  	pattern_rotate, pattern_count, /*pattern_reverse,*/ pattern_mexican};

int delays[4 +1] = {1000,500,250,100, -1}; //tdelay during a state

//-----------------------------------------------------------------------------

/* digitalWrites(array, state)
 *   action: sets the digital pins in ARRAY to STATE.
 *   preconditions: ARRAY is terminated by a negative value
 */
void digitalWrites(char * array, byte state)
{
  for (char * p=array; *p>-1; p++)
    digitalWrite((byte)*p, state);
}

/* pinModes(array, mode)
 *   action: sets the mode of pins in ARRAY to MODE.
 *   preconditions: ARRAY is terminated by a negative value
 */
void pinModes(char * array, byte mode)
{
  for (char * p=array; *p>-1; p++)
    pinMode((byte)*p, mode);
}

/* stateSet(bitmask)
 *   action: sets the state of each LED to that specified by BITMASK.
 *           if BITMASK has a negative sign, do nothing.
 *   preconditions: pinModes() has already been called on all the LED pins
 */
void stateSet(char bitmask)
{
	if (bitmask<0) return;
	for (int i=0; i<nLEDs; i++)
        digitalWrite(pins_pwm[nLEDs-i-1], ((byte)(bitmask) & (1<<i)?HIGH:LOW));
}

/* runPattern(pattern, tinterval)
 *   action: cycle through LED states in the pattern, spaced at TINTERVAL ms
 */
void runPattern(char * pattern, int tinterval) {
	for (char *p=pattern; *p>-1; p++) {
		stateSet(*p);
		delay(tinterval);
	}
	stateSet(0);
}

//-----------------------------------------------------------------------------

void setup() 
{
  // zero all pins
  pinModes(pins_gnd, OUTPUT);
  pinModes(pins_pwm, OUTPUT);
  digitalWrites(pins_gnd, LOW);
  digitalWrites(pins_pwm, LOW);
  
#ifdef TEST
  //A :: manual test, just lighting sequence 1-6 
  // runPattern(pattern_test, 330);
  
  //B :: automatic test, all lighting sequences
  for (char **p=patterns; *p!=NULL; p++) {
	  runPattern(*p, 330);
	  delay(1000);
  }
  #endif
  
}

void loop() 
{
  delay(1000);
}

//-----------------------------------------------------------------------------
/*
	Todo list:
		X- make a set of bitmask arrays defining lighting states for various patterns
		X- make a global char ** array containing all these patterns
		X- make a function for executing a given pattern at a given speed
		- make a function for executing a given pattern with fade
		- make a higher-level function for executing all patterns at various speeds
			> modes: sequential, random, basic/slow, crazy/fast, pulse only, on only
		- add a parallel serial console
		- add a parser to the serial console to recognise commands:
			bpm ___ (e.g. 240bpm = 1000 * 60 / 240 = 250ms cycle)
			mode ___ (1=sequential, 2=random, 3=basic/slow, 4=crazy/fast, 5=pulse, 6=on
			off
			help (print usable commands)
		- implement all the commands

	problem of parallel console & operations:
		- every time delay(x) is called, instead call block(x)
		- block is a custom function which does serial_handle() and then delay(x)
*/
