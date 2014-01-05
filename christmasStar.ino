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
//-----------------------------------------------------------------------------

#include <stdint.h>

#define TEST 1

char pins_gnd[7] = {13,12,8,7,4,2, -1};
char pins_pwm[7] = {10,11,6,9,3,5, -1};

int speeds[5] = {1000,500,250,100, -1};
int cycleTime = 2000; //multiply by #states in the active lighting pattern (generally 6)

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

//-----------------------------------------------------------------------------

void setup() 
{
  // zero all pins
  pinModes(pins_gnd, OUTPUT);
  pinModes(pins_pwm, OUTPUT);
  digitalWrites(pins_gnd, LOW);
  digitalWrites(pins_pwm, LOW);
  
  // test lighting sequence 1-6
  #ifdef TEST
  for (char * p=pins_pwm; *p>-1; p++) {
    digitalWrite(*p, HIGH);
    delay(500);
    digitalWrite(*p, LOW);
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
		- make a set of bitmask arrays defining lighting states for various patterns
		- make a global char ** array containing all these patterns
		- make a function for executing a given pattern at a given speed
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
