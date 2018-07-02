// MaCaQuE_firmware
//
// by Michael Berger
// German Primate Center
// mail: mberger@dpz.eu
//
// for more information see: 
// Berger M, Gail A (2018) The Reach Cage environment for wireless neural recordings during structured goal-directed behavior of unrestrained monkeys, bioRxiv
// https://www.biorxiv.org/content/early/2018/04/24/305334

/******************** includes********************/
#include "MaCaQuE_defines.h"
#include <Adafruit_NeoPixel.h>
#include <Bounce.h>
#ifdef USE_SPI
#include <SPI.h>
#endif

/******************** variables ********************/
// flags
boolean debug_prints = false;
boolean sensor_flag = false;
boolean send_data = false;

// timer variables
unsigned long loop_start;
unsigned long max_duration;
unsigned long reward_duration = 0; //ms
unsigned long reward_start_time; //ms
unsigned long vibration_duration = 0; //ms
unsigned long vibration_start_time; //ms
elapsedMillis read_interval; // ms interval between sensores are read
elapsedMicros delay_light; // us to test delays
elapsedMicros delay_vib; // us to test delays
unsigned int read_freq = 10; // ms
unsigned long manual_reward_duration; // ms
unsigned long button_1_duration; // ms
unsigned long button_2_duration; // ms


//Neopixel and Target States
int n_neo = 16;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(n_neo, PIN_NEO, NEO_GRB + NEO_KHZ800);

/*
  // some predefined states
  byte led_on   = B11111111;
  byte led_off  = B00000000;
  byte led_half = B11110000;
  byte led_2nd  = B01010101;
  byte led_array[4] = {led_off, led_on, led_half, led_2nd};*/

// temporary variables
byte sensor_output[64];
//byte touch_output;
byte touch_output_1;
byte touch_output_2;
//byte reward_output; - depracted
int sensor_index = 0;
int target_values[4];

// manual reward
boolean manual_reward_pressed = false;
Bounce manual_reward_button = Bounce(PIN_MANUAL_REWARD, 20); // 20 ms debounce
boolean first_reward_selected = true;

// other buttons
boolean button_1_pressed = false;
boolean button_2_pressed = false;
#ifndef USE_TESTING_SETTING
Bounce button_1 = Bounce(PIN_BUTTON_1, 50); // 50 ms debounce
Bounce button_2 = Bounce(PIN_BUTTON_2, 50); // 50 ms debounce
#endif


//----------- DEVELOP
// for testing: Pushbuttons in digital inputs
//Bounce digital_in_1 = Bounce(PIN_DIGITAL_IN_1, 1); // 10 ms debounce
//Bounce digital_in_2 = Bounce(PIN_DIGITAL_IN_2, 1); // 10 ms debounce
// alternative
int n_digital_ins = 8;
// do this better with arrays
int digital_state_1 = 0;
int digital_state_2 = 0;


/******************** setup function ********************/
void setup()
{
  /**** set pinmodes ****/
  //Neopixel
  pinMode(PIN_NEO, OUTPUT);

  // touch button
  pinMode(PIN_TOUCH, INPUT);
  pinMode(PIN_TOUCH_CLK, OUTPUT);
  pinMode(PIN_TOUCH_LATCH, OUTPUT);

  // reward
  pinMode(PIN_REWARD, OUTPUT);
  pinMode(PIN_REWARD_LED, OUTPUT);
  pinMode(PIN_REWARD_2, OUTPUT); // pinMode(PIN_REWARD_SENSOR,INPUT);
  //ATTENTION: as a hardware workaround in Version 1.1 of the MaCaQuE Interface, the Reward_Sensor circuit is utilized for the 2nd reward unit
  // reward sensor might not exist anymore in later versions

  pinMode(PIN_MANUAL_REWARD, INPUT);
  pinMode(PIN_SWITCH_MR, INPUT_PULLUP);

#ifdef USE_TESTING_SETTING
  pinMode(PIN_TEST_OUT, OUTPUT);
#else
  // buttons
  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);
#endif

  // vibration motors
  pinMode(PIN_MOTOR_1, OUTPUT);
  pinMode(PIN_MOTOR_2, OUTPUT);

#ifdef USE_SPI
  // BR communication
  pinMode(PIN_BR_LATCH, OUTPUT);
  pinMode(PIN_BR_STROBE, OUTPUT);
  // add further BR pins?
#else
  // delay testing
  pinMode(PIN_LIGHT_TEST, INPUT);
  pinMode(PIN_VIB_TEST, INPUT);
#endif

  // digital inputs
  pinMode(PIN_DIGITAL_IN_1, INPUT);
  pinMode(PIN_DIGITAL_IN_2, INPUT);
  //  for (int i; i<n_digital_ins;i++)
  //  {
  //    int pin
  //  }

  /**** Starting ****/
  // Neopixel
  strip.begin();
  strip.show();

#ifdef USE_SPI
  // initialize SPI for communicationg with BR cerebus
  SPI.begin();
#endif

  // init serial port
  Serial.begin(9600); // baud rate is not considered with teensy
}

/******************** utility functions ********************/
//// input evaulation functions ////
// handshake function to measure latencies
int handshake(char p_in)
{
  // now just print data, but in the end receive input (= PC sent time) and send back
  if (p_in == IN_HANDSHAKE)
  {
    if (debug_prints)
    {
      Serial.println("handshake");
    }

    // read out handshake
    //    int handshake_value = Serial.read();

    // respond to it
    byte out[1] = {'h'};
    Serial.write(out, 1);
    Serial.send_now();

    return 1;
  }
  // wrong indicator
  return 0;
}

// external request to set the target
int set_target(char p_in)
{
  if (p_in == IN_TARGET)
  {
    if (Serial.available() >= 4)
    {
      //DEBUG
      if (debug_prints)
      {
        Serial.println("set_target");
      }

      // read in as char
      char target_values_char[4];
      Serial.readBytes(target_values_char, 4);

      // convert to integer
      for (int i = 0; i < 4; i++)
      {
        target_values[i] = int(target_values_char[i]);
      }

      set_target_state(target_values[0], target_values[1], target_values[2], target_values[3]);

      return 1;
    }
    // not enough input bytes received
    return -1;
  }
  // wrong indicator
  return 0;
}

// external request for reward
int set_reward(char p_in)
{
  if (p_in == IN_REWARD)
  {
    if (Serial.available() >= 3)
    {
      //DEBUG
      if (debug_prints)
      {
        Serial.println("set_reward");
      }

      // read number of reward unit
      unsigned int reward_unit = Serial.read();

      //char reward_duration_char[2];
      unsigned times_255, rest;

      // read reward duration
      times_255 = Serial.read();
      rest = Serial.read();

      reward_duration = times_255 * 255 + rest;

      // do reward if not manually pressed
      if (!manual_reward_pressed)
      {
        switch (reward_unit)
        {
          case 0:
            // Reward unit 1
            digitalWriteFast(PIN_REWARD, HIGH);
            //digitalWriteFast(PIN_REWARD_LED,HIGH);
            break;
          case 1:
            // Reward unit 2
            digitalWriteFast(PIN_REWARD_2, HIGH);
            break;
        }
      }

      reward_start_time = millis();


      return 1;
    }
    // not enough bytes received
    return -1;
  }
  // wrong indicator
  return 0;
}

// external request for vibration motor
int set_vibration(char p_in)
{
  if (p_in == IN_VIBRATION)
  {
    if (Serial.available() >= 3)
    {
      //DEBUG
      if (debug_prints)
      {
        Serial.println("set_reward");
      }

      // read number of reward unit
      unsigned int vibration_unit = Serial.read();

      //char reward_duration_char[2];
      unsigned times_255, rest;

      // read reward duration
      times_255 = Serial.read();
      rest = Serial.read();

      vibration_duration = times_255 * 255 + rest;

      // do reward if not manually pressed
      switch (vibration_unit)
      {
        case 0:
          // Reward unit 1
          digitalWriteFast(PIN_MOTOR_1, HIGH);
          //digitalWriteFast(PIN_REWARD_LED,HIGH);
          break;
        case 1:
          // Reward unit 2
          digitalWriteFast(PIN_MOTOR_2, HIGH);
          break;
      }


      vibration_start_time = millis();


      return 1;
    }
    // not enough bytes received
    return -1;
  }
  // wrong indicator
  return 0;
}

// external request: set sensor flag == start sending the sensor data
int start_sensors(char p_in)
{
  if (p_in == IN_START)
  {
    //DEBUG
    if (debug_prints)
    {
      Serial.println("start_sensors");
    }

    sensor_flag = true;
    return 1;
  }
  // wrong indicator
  return 0;
}

// unset sensor flag == stop sending the sensor data
int stop_sensors(char p_in)
{
  if (p_in == IN_STOP)
  {
    //DEBUG
    if (debug_prints)
    {
      Serial.println("stop_sensors");
    }

    sensor_flag = false;
    return 1;
  }

  // wrong indicator
  return 0;
}

// external request: turn off neopixels
int turn_off(char p_in)
{
  if (p_in == IN_OFF)
  {
    //DEBUG
    if (debug_prints)
    {
      Serial.println("Turn Neopixel Off");
    }

    // turn of Neopixel
    for (int i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();

    return 1;
  }
  // wrong indicator
  return 0;
}

// external request: send 16-bit word to BR
int send_BR(char p_in)
{
  if (p_in == IN_BR)
  {
    //DEBUG
    if (debug_prints)
    {
      Serial.println("send_BR");
    }

    if (Serial.available() >= 2)
    {
      // inputs
      uint8_t high_byte = Serial.read();
      uint8_t low_byte = Serial.read();

      //send to BR cerebus system
      send_BR_16bit(high_byte, low_byte);

      return 1;
    }
    // not enough bytes received
    return -1;
  }
  // wrong indicator
  return 0;
}

// external request: reset the system (flush serial port + turn of neopixels)
int do_reset(char p_in)
{
  if (p_in == IN_RESET)
  {
    if (Serial.available() >= 2)
    {
      // double and triple check reset
      if (Serial.read() == IN_RESET)
      {
        if (Serial.read() == IN_RESET)
        {
          //DEBUG
          if (debug_prints)
          {
            Serial.println("DO_RESET");
          }
          // flush serial port
          Serial.flush();
          // turn Neopixel off
          for (int i = 0; i < strip.numPixels(); i++)
          {
            strip.setPixelColor(i, strip.Color(0, 0, 0));
          }
          strip.show();
          
          // set shiftOut register to 0
          shift_out_SPI_16bit(0, 0);

          //in test mode turn off test output
          #ifdef USE_TESTING_SETTING
            analogWrite(PIN_TEST_OUT,0);
          #endif

          return 1;
        }
      }
    }
    // either not enough bytes received or consecutive bytes were not correct
    return -1;
  }
  // wrong indicator
  return 0;
}

// external request: to set the test output; only works if USE_TESTING_SETTING is defined
int set_testing_output(char p_in)
{
  // this function does nothing if USE_TESTING_SETTING is not defined
  #ifdef USE_TESTING_SETTING
  if (p_in == IN_TEST_OUT)
  {
    if (Serial.available() >= 1)
    { 
      // input value
      int test_out_signal = Serial.read();
      test_out_signal = constrain(test_out_signal,0,255);

      // send the 8bit signal as PWM signal
      analogWrite(PIN_TEST_OUT, test_out_signal);
      
      return 1;
    }
    // not enough bytes received
    return -1;
  }
  // wrong indicator
  return 0;
  #endif
}

// external request: test delyas. Only possible without BR communication
int do_delay_testing(char p_in)
{
#ifndef USE_SPI
  if (p_in == IN_TEST_DELAY)
  {
    //DEBUG
    if (debug_prints)
    {
      Serial.println("Test Vibration and MCT delay");
    }
    
    // test light
    // set counter to 0 (uS)
    delay_light = 0;
    // turn MCTs on
    for (int i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, strip.Color(255, 255, 255));
    }
    strip.show();
    // wait for light sensor to respond
    while (!digitalRead(PIN_LIGHT_TEST))
    {
      // abort condition if it takes to long
      // 15s
      if (delay_light >= 1000000)
      {
        break;
      }
    }
    // calculate delay
    int32_t measured_delay_light = delay_light;
    // turn MCTs off
    for (int i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();

    // test vibration
    // set counter to 0 (uS)
    delay_vib = 0;
    // turn all vibrations motors on
    digitalWriteFast(PIN_MOTOR_1, HIGH);
    digitalWriteFast(PIN_MOTOR_2, HIGH);
    // wait for vibration sensor to respond
    int last_vib_read = digitalRead(PIN_VIB_TEST);
    while (last_vib_read == digitalRead(PIN_VIB_TEST))
    {
      // abort condition if it takes to long
      // 15s
      if (delay_vib >= 1000000)
      {
        break;
      }
    }
    // calculate delay
    int32_t measured_delay_vib = delay_vib;
    // turn all vibtration motors off
    digitalWriteFast(PIN_MOTOR_1, LOW);
    digitalWriteFast(PIN_MOTOR_2, LOW);

    // send the two delay values
    byte out[7];
    out[0] = OUT_TEST_DELAY;
    // transform duration in 3 bytes
    out[1] = (byte) (measured_delay_light & 0xFF); // LSB
    out[2] = (byte) ((measured_delay_light >> 8) & 0xFF);
    out[3] = (byte) ((measured_delay_light >> 16) & 0xFF);
    out[4] = (byte) (measured_delay_vib & 0xFF); // LSB
    out[5] = (byte) ((measured_delay_vib >> 8) & 0xFF);
    out[6] = (byte) ((measured_delay_vib >> 16) & 0xFF);
    // send it
    Serial.write(out, 7);
    Serial.send_now();
    
  }
  #endif
  // wrong indicator
  return 0;
}

//// further functions ////
// set target function
void set_target_state(int p_ID, int p_red, int p_green, int p_blue)
{
  // set neopixel color
  if (p_ID < n_neo)
  {
    // ATTENTION THIS IS A WORKAROUND SINCE THE FIRST PIXEL HAD TO BE REMOVED
    //if (p_ID > 0) //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //{//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //  p_ID -= 1;
    strip.setPixelColor(p_ID, strip.Color(p_red, p_green, p_blue));
    //}//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //-----------------------------------------------------------------------
  }
  else
  {
    // Pixel is out of range, do something??
  }

  // update strip
  strip.show();
}

// send 16bit word to shift register (using SPI)
void shift_out_SPI_16bit(uint8_t p_high_byte, uint8_t p_low_byte)
{
  // this function doesn't do anything if SPI is not used
#ifdef USE_SPI
  // take the latch low
  digitalWriteFast(PIN_BR_LATCH, LOW);
  //  send consecutively the higher and lower byte
  SPI.transfer(p_high_byte);
  SPI.transfer(p_low_byte);
  delayMicroseconds(20); // should not be need, only if a 1uF Cap is between latch and GND
  // take the latch high to make the input of the shift register visible on the output
  digitalWriteFast(PIN_BR_LATCH, HIGH);
#endif
}

// send 16bit word to BR cerebus system
void send_BR_16bit(uint8_t p_high_byte, uint8_t p_low_byte)
{
#ifdef USE_SPI
  // set the signal to the parallel input
  shift_out_SPI_16bit(p_high_byte, p_low_byte);
  delayMicroseconds(100); // timing is taken out of Pierres code

  // pulse on strobe that BR stores its input
  digitalWriteFast(PIN_BR_STROBE, HIGH);
  delayMicroseconds(500); // timing is taken out of Pierres code
  digitalWriteFast(PIN_BR_STROBE, LOW);
  delayMicroseconds(100);

  // set the signal to 0 again
  //shift_out_SPI_16bit(0,0)
#endif
}

/******************** loop function ********************/
void loop()
{
  // set loop start time
  loop_start = millis();

  // check reward if it has to be turned down
  if ( (reward_duration < (millis() - reward_start_time)) && (manual_reward_pressed == false) )
  {
    // stop reward
    if (digitalRead(PIN_REWARD) == HIGH)
    {
      // reward unit 1
      digitalWriteFast(PIN_REWARD, LOW);
      //digitalWriteFast(PIN_REWARD_LED, LOW);
    }
    if (digitalRead(PIN_REWARD_2) == HIGH)
    {
      // reward unit 1
      digitalWriteFast(PIN_REWARD_2, LOW);
    }
  }

  // check vibration motor if it has to be stopped
  if ( vibration_duration < (millis() - vibration_start_time) )
  {
    // stop motor
    if (digitalRead(PIN_MOTOR_1) == HIGH)
    {
      // reward unit 1
      digitalWriteFast(PIN_MOTOR_1, LOW);
      //digitalWriteFast(PIN_REWARD_LED, LOW);
    }
    if (digitalRead(PIN_MOTOR_2) == HIGH)
    {
      // reward unit 1
      digitalWriteFast(PIN_MOTOR_2, LOW);
    }
  }

  // check manual reward button
  if (manual_reward_button.update())
  {
    // button was pressed
    if (manual_reward_button.risingEdge())
    {
      // start reward
      if (digitalRead(PIN_SWITCH_MR))
      {
        first_reward_selected = true;
        digitalWriteFast(PIN_REWARD, HIGH);
        //digitalWriteFast(PIN_REWARD_LED,HIGH);
      }
      else
      {
        first_reward_selected = false;
        digitalWriteFast(PIN_REWARD_2, HIGH);
      }

      // set manual reward flag
      manual_reward_pressed = true;

      // start timer
      manual_reward_duration = millis();
    }
    // button was released
    if (manual_reward_button.fallingEdge())
    {

      // stop reward
      digitalWriteFast(PIN_REWARD, LOW);
      digitalWriteFast(PIN_REWARD_2, LOW);
      //digitalWriteFast(PIN_REWARD_LED, LOW);

      // unset manual reward flag
      manual_reward_pressed = false;

      // stop timer
      manual_reward_duration = millis() - manual_reward_duration;

      // send time via serial port
      // only if below 1 min
      if ((manual_reward_duration > 50) && (manual_reward_duration < 60000))
      {
        // output byte array
        byte out[3];
        out[0] = OUT_MANUAL_REWARD;
        // transform duration in 2 bytes
        out[1] = (byte) (manual_reward_duration & 0xFF); // LSB
        out[2] = (byte) ((manual_reward_duration >> 8) & 0xFF);
        // send it
        Serial.write(out, 3);
        Serial.send_now();
      }
    }
  }

  // check buttons
#ifndef USE_TESTING_SETTING
  // button 1
  if (button_1.update())
  {
    if (button_1.fallingEdge())
    {
      // button released

      // send a 0
      // output byte array
      byte out[3];
      out[0] = OUT_PUSH_BUTTON;
      out[1] = (byte) 0;
      out[2] = (byte) 0;
      // send it
      Serial.write(out, 3);
      Serial.send_now();
    }


    if (button_1.risingEdge())
    {
      // button pressed

      // send a 1
      // output byte array
      byte out[3];
      out[0] = OUT_PUSH_BUTTON;
      out[1] = (byte) 0;
      out[2] = (byte) 1;
      // send it
      Serial.write(out, 3);
      Serial.send_now();
    }
  }

  // button 2
  if (button_2.update())
  {
    if (button_2.fallingEdge())
    {
      // button released

      // send a 0
      // output byte array
      byte out[3];
      out[0] = OUT_PUSH_BUTTON;
      out[1] = (byte) 1;
      out[2] = (byte) 0;
      // send it
      Serial.write(out, 3);
      Serial.send_now();
    }


    if (button_2.risingEdge())
    {
      // button pressed

      // send a 1
      // output byte array
      byte out[3];
      out[0] = OUT_PUSH_BUTTON;
      out[1] = (byte) 1;
      out[2] = (byte) 1;
      // send it
      Serial.write(out, 3);
      Serial.send_now();
    }
  }
#endif

  // look for available input
  if (Serial.available() > 0)
  {
    char indicator = Serial.read();

    // parse the input
    stop_sensors(indicator);
    handshake(indicator);
    set_target(indicator);
    set_reward(indicator);
    set_vibration(indicator);
    start_sensors(indicator);
    turn_off(indicator);
    send_BR(indicator);
    do_reset(indicator);
    set_testing_output(indicator);
    do_delay_testing(indicator);
  }

  // check digital in and send event if triggered
  // digital 1
  // read digital state
  int cur_dig_state_1 = digitalRead(PIN_DIGITAL_IN_1);
  //if (digital_in_1.update())
  if (digital_state_1 != cur_dig_state_1)
  {
    // signal set
    //if (digital_in_1.fallingEdge()) // falling edge == set due to pullup
    if (digital_state_1 < cur_dig_state_1)
    {
      byte out[3] = {OUT_DIGITAL, 1, 1};
      Serial.write(out, 3);
      Serial.send_now();
    }

    // signal unset
    //if (digital_in_1.risingEdge()) // rising edge == unset due to pullup
    if (digital_state_1 > cur_dig_state_1)
    {
      byte out[3] = {OUT_DIGITAL, 1, 0};
      Serial.write(out, 3);
      Serial.send_now();
    }

    digital_state_1 = cur_dig_state_1;
  }

  // digital 2
  // read digital state
  int cur_dig_state_2 = digitalRead(PIN_DIGITAL_IN_2);
  //if (digital_in_2.update())
  if (digital_state_2 != cur_dig_state_2)
  {
    // signal set
    //if (digital_in_2.fallingEdge()) // falling edge == set due to pullup
    if (digital_state_2 < cur_dig_state_2)
    {
      byte out[3] = {OUT_DIGITAL, 2, 1};
      Serial.write(out, 3);
      Serial.send_now();
    }

    // signal unset
    //if (digital_in_2.risingEdge()) // rising edge == unset due to pullup
    if (digital_state_2 > cur_dig_state_2)
    {
      byte out[3] = {OUT_DIGITAL, 2, 0};
      Serial.write(out, 3);
      Serial.send_now();
    }

    digital_state_2 = cur_dig_state_2;
  }

  // get sensors if flag is active and send data via serial port
  if ( (sensor_flag) && (read_interval >= read_freq) )
  {
    //DEBUG
    if (debug_prints)
    {
      Serial.println("read sensor data");
    }

    // reset timing function
    read_interval = 0;

    //pulse latch pin to update the shift register
    digitalWriteFast(PIN_TOUCH_LATCH, HIGH);
    delayMicroseconds(20);
    digitalWriteFast(PIN_TOUCH_LATCH, LOW);

    // read sensors
    // for 16 button
    touch_output_1 = shiftIn(PIN_TOUCH, PIN_TOUCH_CLK, MSBFIRST);
    touch_output_2 = shiftIn(PIN_TOUCH, PIN_TOUCH_CLK, MSBFIRST);
    byte out[3] = {OUT_BUTTON, touch_output_2, touch_output_1};
    Serial.write(out, 3);
    Serial.send_now();



    if (debug_prints)
    {
      //Serial.print(' Sensor States: '); // this line produced an error??
      //Serial.println(out[1], BIN);
    }

    //    //check if output array is full
    //    if (sensor_index >= 30)
    //    {
    //      sensor_index = 0;
    //      send_data = true;
    //    }
    //    else
    //    {
    //      sensor_index++;
    //    }

  }

  //  // send data if buffer is full
  //  if (send_data)
  //  {
  //    // send data
  //   Serial.write(sensor_output,64);
  //   Serial.send_now();
  //   send_data = false;
  //  }

  if (debug_prints)
  {
    unsigned long loop_duration = millis() - loop_start;
    max_duration = max(max_duration, loop_duration);
    Serial.print("Loop Duration: ");
    Serial.print(loop_duration);
    Serial.print("Max Duration: ");
    Serial.println(max_duration);
  }

}
