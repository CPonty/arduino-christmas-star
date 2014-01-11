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
// TRIANGLE (2)
//     101010 ==> 0x2A
//     010101 ==> 0x15
// ERROR (1)
//     101010 ==> 0x2A
//     000000 ==> 0x00
//////////////////////
//     000000 ==> 0x00
//-----------------------------------------------------------------------------

#include <stdint.h>

void digitalWrites(char * array, byte state);
void pinModes(char * array, byte mode);
void stateSet(char bitmask);
void stateFadeTo(char bitmask, int tinterval);
void runPattern(char * pattern, int tinterval, bool fade);
bool runPatternId(char patternId, int tinterval, bool fade);
bool runModeId(char modeId);
void modeOff(void);
void modeOn(void);
void modePulse(void);
void modeSequential(void);
void modeRandom(void);
void modeFixed(void);
void msg(const char * message);
void err(const char * message);
int arrayLen(char * arr);
bool serialHandle(void);
void block(unsigned long ms);
bool modeChanged(void);

//-----------------------------------------------------------------------------

// #define TEST
#define PRINT

#define nLEDs 6 // 6 LEDs connected to the Arduino
#define nCYCLES nLEDs*4 // indicates #states a pattern should loop through
#define TFADE 8 // delay between LED fade levels (ms)

char pins_gnd[nLEDs +1] = {13,12,8,7,4,2, -1};
char pins_pwm[nLEDs +1] = {10,11,6,9,3,5, -1};

#define BPM_MIN 1 // "beats per minute" for PULSE mode (user can set on console)
#define BPM_DEFAULT 220
#define BPM_MAX 1000

#define MODE_OFF 0
#define MODE_ON 1
#define MODE_PULSE 2
#define MODE_SEQUENTIAL 3
#define MODE_RANDOM 4
#define MODE_FIXED 5
#define MODE_DEFAULT MODE_RANDOM
#define MODE_MAX 5

void (*modes[MODE_MAX +2])(void) = {modeOff, modeOn, modePulse, modeSequential,
		 					 		modeRandom, modeFixed, NULL};
									
#define PATTERN_TEST 0
#define PATTERN_ON 1
#define PATTERN_FLASH 2
#define PATTERN_ROTATE 3
#define PATTERN_COUNT 4
#define PATTERN_MEXICAN 5
#define PATTERN_TRIANGLE 6
#define PATTERN_DEFAULT PATTERN_TEST
#define PATTERN_MAX 6

// Bitmasks of 0b00xxxxxx, where the lower 6 bits indicate the active LEDs
// Signed value; negative value (MSB=1) indicates end-of-array
char pattern_test[nLEDs +1] = {0x20, 0x10, 0x08, 0x04, 0x02, 0x01, -1};
char pattern_error[2 +1] = {0x2A, 0x00, -1};//only for err(), no #def ID
char pattern_off[1 +1] = {0x00, -1}; 		//only for modeOff(), no #def ID
char pattern_on[1 +1] = {0x3F, -1};
char pattern_flash[2 +1] = {0x3F, 0x00, -1};
char pattern_rotate[3 +1] = {0x24, 0x12, 0x09, -1};
char pattern_count[nLEDs*2 +1] = {0x00, 0x20, 0x30, 0x38, 0x3C, 0x3E, 0x3F, 
	0x1F, 0x0F, 0x07, 0x03, 0x01, -1};
char pattern_mexican[nLEDs +1] = {0x38, 0x1C, 0x0E, 0x07, 0x23, 0x31, -1};
char pattern_triangle[2 +1] = {0x2A, 0x15, -1};

char * patterns[PATTERN_MAX +2] = {pattern_test, /*error, off, */
	 	pattern_on, pattern_flash, pattern_rotate, pattern_count,
		pattern_mexican, pattern_triangle, NULL};

#define TDELAY_MIN 64 // LED state time period (ms)
#define TDELAY_DEFAULT 256
#define TDELAY_MAX 1024
#define TDELAY_SET_MIN 1 // (user sets value 1..5, scales to 64*(2**(val-1)))
#define TDELAY_SET_MAX 5
		
#define FADE_DEFAULT true //toggles smoothly fading between LED states

//-----------------------------------------------------------------------------

char state = 0x0; //LED pattern bitmask

bool fade = FADE_DEFAULT;

int tdelay = TDELAY_DEFAULT;

int bpm = BPM_DEFAULT;

char pattern = PATTERN_DEFAULT;

char mode = MODE_DEFAULT; 

bool modeChange = false; // flag set by serialHandle to trigger mode changing

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
	state = bitmask;
	for (int i=0; i<nLEDs; i++)
        digitalWrite(pins_pwm[nLEDs-i-1], ((byte)(bitmask) & (1<<i)?HIGH:LOW));
}

/* stateFadeTo(bitmask, tinterval)
 *   action: uses PWM to fade the LEDs between the current state
 *            and that of BITMASK, over TINTERVAL ms.
 *   preconditions: TINTERVAL % 2 == 0
 *                  TINTERVAL > 8
 *                  pinModes() has already been called on all the LED pins
 */
void stateFadeTo(char bitmask, int tinterval) 
{
	byte low = 255, high = 0;
	char diffmask = bitmask ^ state; // bitmap of LEDs that need to change
	int nticks = tinterval/TFADE;
	int ticksize = 256/nticks;
	
	if (nticks == 0 || tinterval < 0) return;
	for (; nticks>0; nticks--) {
		if (high+ticksize >= 255) {
			low = 0;
			high = 255;
		} else {
			low -= ticksize;
			high += ticksize;
		}
		for (int i=0; i<nLEDs; i++) {
			if ((byte)(diffmask) & (1<<i)) {
            	analogWrite(pins_pwm[nLEDs-i-1], 
							((byte)(bitmask) & (1<<i)?high:low));
			}
		}
		delay(TFADE);
	}
	
	//delay(tinterval);
	stateSet(bitmask);
} 

/* runPattern(pattern, tinterval, fade)
 *   action: cycle through LED states in the pattern, spaced at TINTERVAL ms
 *           use PWM fading if specified by FADE
 *   preconditions: TINTERVAL % 2 == 0
 *                  TINTERVAL > 8
 */
void runPattern(char * pattern, int tinterval, bool fade) 
{
	if (serialHandle()) return;
	for (char *p=pattern; *p>-1; p++) {
		if (fade) {
			stateFadeTo(*p, tinterval);
		} else {
			stateSet(*p);
			delay(tinterval);
		}
		if (serialHandle()) return;
	}
	//stateSet(0);
}

/* runPatternId(patternId, tinterval, fade)
 *   action: execute runPattern for the PATTERNS entry matching PATTERNID.
 *   returns: true  if 0 <= PATTERNID <= PATTERN_MAX
 *            false otherwise
 */
bool runPatternId(char patternId, int tinterval, bool fade) 
{
	if (patternId<0 || patternId>PATTERN_MAX)
		return false;
	runPattern(patterns[patternId], tinterval, fade);
	return true;
}

/* runModeId(modeId)
 *   action: execute the function in the MODES entry matching MODEID.
 *   returns: true  if 0 <= MODEID <= MODE_MAX
 *            false otherwise
 */
bool runModeId(char modeId) 
{
	if (modeId<0 || modeId>MODE_MAX)
		return false;
	(*modes[modeId])();
	return true;
}

/* msg(message)
 *  action: print a serial MESSAGE.
 *          If MESSAGE is null, nothing is printed.
 */
void msg(const char * message) 
{
#ifdef PRINT
	if (message != NULL)
		Serial.println(message);
#endif
}

/* err(message)
 *  action: print a serial MESSAGE, run pattern_error() and halt. 
 *          User sees blinking red lights.
 *          If MESSAGE is null, nothing is printed.
 */
void err(const char * message) 
{
#ifdef PRINT
	if (message != NULL)
		Serial.println(message);
#endif
	while(1) 
		runPattern(pattern_error, 256, true);
}

//-----------------------------------------------------------------------------

/* arrayLen(arr)
 *   action: return the length of a negative-terminated array of characters
 *   preconditions: block of bytes at ARR is negative-terminated
 */
int arrayLen(char * arr)
{
	int ret;
	for (ret=0; arr[ret]>=0; ret++);
	return ret;
}

//TODO string to int
//TODO set____() functions for each applicable global

//TODO write it properly, this just acknowledges a character.
bool serialHandle(void) 
{
    while (Serial.available() > 0) {
		int incomingByte = Serial.read();
		Serial.print("I received: ");
        Serial.println(incomingByte, DEC);
    }
}

/* block(ms)
 *   action: wait for MS milliseconds. Handle serial connections afterwards
 */
void block(unsigned long ms) 
{
	delay(ms);
	serialHandle();
}

/* modeChanged()
 *   action: if modeChange flag is set, clear it and return true.
 */
bool modeChanged(void)
{
	if (modeChange) {
		modeChange = false;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------

void modeOff(void) /* 0 */
{
	msg(" > MODE: 0 (Off)");
	while(1) {
		runPattern(pattern_off,64,false);
		if (modeChanged()) return;
	}
}

void modeOn(void) /* 1 */
{
	msg(" > MODE: 1 (On)");
	while (1) {
		runPattern(pattern_on,64,false);
		if (modeChanged()) return;
	}
}

void modePulse(void) /* 2 */
{
	msg(" > MODE: 2 (Pulsating)");
	while (1) {
		runPattern(pattern_flash, ((unsigned long)1000*60)/bpm, fade);
		if (modeChanged()) return;
	}
}

void modeSequential(void) /* 3 */
{
	msg(" > MODE: 3 (Sequential Patterns)");
	while (1) {
	    for (char **p=patterns; *p!=NULL; p++) {
			// Calculate the #repeats. 
			//  equivalent to ceil(totalStates/#patternStates)
			int nrepeats = ((nCYCLES + arrayLen(*p) -1) / arrayLen(*p));
			for (; nrepeats>0; nrepeats--) {
				runPattern(*p, tdelay, true);
				if (modeChanged()) return;
			}
	  		block(tdelay);
			if (modeChanged()) return;
	    }
	}
}

void modeRandom(void) /* 4 */
{
	msg(" > MODE: 4 (Random Patterns)");
	while (1) {
		// Locally randomized parameters (time, #repeats, pattern, fade).
		//  We don't want the randomizer's selections to overwrite globals
		//  set by defaults / console input.
		int ttime = TDELAY_MIN * pow(2,random(TDELAY_SET_MIN-1, TDELAY_SET_MAX-1));
		char patternId = random(0, PATTERN_MAX +1);
		int patLen = arrayLen(patterns[patternId]);
		int nrepeats = ((random(nCYCLES/2, nCYCLES) + patLen -1) / patLen);
		bool doFade = (bool)random(0, 1 +1);
		for (; nrepeats>0; nrepeats--) {
			runPatternId(patternId, ttime, doFade);
			if (modeChanged()) return;
		}
  		block(tdelay);
		if (modeChanged()) return;
	}
}

void modeFixed(void) /* 5 */
{
	msg(" > MODE: 5 (Fixed Pattern)");
	while (1) {
		runPatternId(pattern, tdelay, fade);
		if (modeChanged()) return;
	}
}

//-----------------------------------------------------------------------------

void setup() 
{
  // initialise & zero all pins
  pinModes(pins_gnd, OUTPUT);
  pinModes(pins_pwm, OUTPUT);
  digitalWrites(pins_gnd, LOW);
  digitalWrites(pins_pwm, LOW);
  
  // serial setup
  Serial.begin(9600);
  msg("\n ~~ Chris Ponticello's Arduino Christmas Star ~~\n");
  
#ifdef TEST
  //A :: fixed test, just lighting sequence 1-6 
  // runPattern(pattern_test, 500);
#endif
  
}

void loop() 
{

#ifdef TEST
  //B :: looping test, all lighting sequences
  for (char **p=patterns; *p!=NULL; p++) {
	  runPattern(*p, TDELAY_DEFAULT, true);
	  delay(TDELAY_DEFAULT);
  }
#else
  
  //err("Error: no error. :P");
  runModeId(mode);
	
#endif
}

//-----------------------------------------------------------------------------
/*
	Todo list:
		X- make a set of bitmask arrays defining lighting states for various patterns
		X- make a global char ** array containing all these patterns
		X- make a function for executing a given pattern at a given speed
		X- make a function for executing a given pattern with fade
		X- extend err() to print to the console
		X- implement all the modes
		X	> higher-level functions for executing patterns
		X	> modes: off, on, pulse, sequential, random, fixed pattern
		- add a parallel serial console line reader
		- add a parser to the serial console to recognise commands:
			mode ___ (0=off, 1=on, 2=pulse, 3=sequential, 4=random, 5=fixed)
			bpm ___ (e.g. 240bpm = 1000 * 60 / 240 = 250ms cycle)
				> only valid on pulse mode
			speed ___ (1..5, scales to 64,128,256,512,1024)
			pattern ___ ()
			help (print usable commands)
		- implement all the commands console commands
		- ensure error hooks & "restart" hooks are correct

	Later:
		- simplify - combine "pulse" and "fixed", such that "fixed" starts on
					  the pulsing pattern. Enables BPM to be set for any pattern.

	//
	problem of parallel console & operations:
		- every time delay(x) is called, instead call block(x)
		- block is a custom function which does serial_handle() and then delay(x)
		- if serial_handle() makes changes to the global state it returns true,
		  and the calling MODE function should exit
*/
