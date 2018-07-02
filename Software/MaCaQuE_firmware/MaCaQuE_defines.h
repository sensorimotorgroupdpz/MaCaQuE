/*
 * Pins and other defines for MaCaQuE_firmware
 */
 
/*** PINS ***/
// Neopixel 
#define PIN_NEO           15


#define PIN_TOUCH         18
// Shift register for Touch
#define PIN_TOUCH_CLK     16
#define PIN_TOUCH_LATCH   17

#define PIN_REWARD        22
#define PIN_REWARD_LED    21 //21 or 13 for reward debugging
#define PIN_REWARD_SENSOR 23 
#define PIN_REWARD_2      20 //23 workaround for first version with 2 reward

// Breakout Pins
#define PIN_MANUAL_REWARD  7
#define PIN_SWITCH_MR      4 // MR is for manual reward. This switch controls which of the 2 reward is addressed by manual reward

#define PIN_DIGITAL_IN_1   8
#define PIN_DIGITAL_IN_2   9

#define PIN_MOTOR_1        2 
#define PIN_MOTOR_2        3 

#define PIN_BUTTON_1       6
#define PIN_BUTTON_2       5

// sync testing 
#define USE_TESTING_SETTING  // if defined the external buttons don't work anymore but it is possible to use PIN_TEST_OUT
#define PIN_TEST_OUT       5 // for sync testing we need a PWM output: use PIN 5

// delay testing -- only possible if BR communication is not used
#define PIN_LIGHT_TEST    11
#define PIN_VIB_TEST      12

// blackrock (BR) communication (mainly on breakout pins)
//#define USE_SPI              // use SPI library; needed for BR communication; if true don't use pins 10-13 apart from SPI; comment out if SPI is not used
#define PIN_BR_LATCH      10 
#define PIN_BR_CLK        13 // not a breakout pin; define not used but this is pin is used by SPI
#define PIN_BR_DATA       11 // define not used but this is pin is used by SPI
#define PIN_SPI_IN        12 // input stream reserved by SPI library; not used in code but this PIN should not be used when SPI is used
#define PIN_BR_STROBE      1 // correct?
#define PIN_BR_18          2
#define PIN_BR_19          3


/*** INPUT CHAR ***/

#define IN_HANDSHAKE      'h'
#define IN_TARGET         't'
#define IN_REWARD         'r'
#define IN_RESET          'c'
#define IN_START          'a'
#define IN_STOP           'z'
#define IN_OFF            'o'
#define IN_VIBRATION      'v'
#define IN_BR             'b'
#define IN_TEST_OUT       'e'
#define IN_TEST_DELAY     'f'

/*** OUTPUT CHAR ***/

#define OUT_MANUAL_REWARD 'm' // duration of manual reward pressed
#define OUT_BUTTON        'b'
#define OUT_DIGITAL       'd'
#define OUT_PUSH_BUTTON   'p' // additional pushbuttons
#define OUT_TEST_DELAY    'g'
