/** MaCaQuE_test_processing
 *
 * Graphical user interface to test MaCaQuE hardware.
 *
 * by Michael Berger
 * German Primate Center
 * mail: mberger@dpz.eu
 *
 * related publication: 
 * Berger M, Gail A (2018) The Reach Cage environment for wireless neural recordings during structured goal-directed behavior of unrestrained monkeys, bioRxiv
 * https://www.biorxiv.org/content/early/2018/04/24/305334
 */

/********** dimports **********/
import processing.serial.*;

/********** variables **********/
Serial myPort;  // Create object from Serial class
int val;        // Data received from the serial port
int last_reward_val; // most current value of the reward sensor - depracted
//int last_sensor_state; // most current value of the sensor states
int last_sensor_state_2; // most current value of the sensor states - second 8
int last_sensor_state_1; // most current value of the sensor states - fist 8
int[] state = new int[3]; //current state of the LED/tone
int digi_input_source; // number of digital input source
int digi_input_value; // 1 or 0 dependet if digital input was set or unset
byte[] target_output = new byte[6];
byte[] reward_output = new byte[4];
byte[] vibration_output = new byte[4];
int light_delay_measured; // result from delay test in us
int vib_delay_measured; // result from delay test in us

// targets
byte tID = 0; // selected target
int n_targets = 16; // number of connected targets (have to be predefined)

// specific states
int[] off      = {  
  0, 0, 0
};
int[] redOn    = {
  255, 0, 0
};
int[] greenOn  = {  
  0, 255, 0
};
int[] blueOn   = {  
  0, 0, 255
};


// handle vals
int plotStart = 200;
int colorSet_xPos = 30;
int colorSet_height = 20;
int colorSet_width = 250;
int button_height = 50;
int button_width = 50;
int button_yPos = plotStart +20;

int rSet_yPos = plotStart+20;
int gSet_yPos = plotStart+45;
int bSet_yPos = plotStart+70;
int button1_xPos = 300;
int button2_xPos = 370;
int button3_xPos = 440;
int button4_xPos = 580;
int button5_xPos = 510;
int button6_xPos = 650;
int button7_xPos = 720;
int button8_xPos = 790;


int indicator_radius = 20;
int indicator_xPos = 50;
int indicator_yPos = 50;
int indicator_distance = 50;

int digi_indicator_radius = 10;
int digi_indicator_xPos = 50;
int digi_indicator_yPos = 350;
int digi_indicator_distance = 30;

/********** setup function **********/
void setup() {
  // print all available serial prots
  println(Serial.list());
  // get name of the serial port
  String portName = (Serial.list()[1]);
  // open this port
  myPort = new Serial(this, portName, 9600);

  // send something to the port
  // first state
  state = off;

  // using ints for connection
  myPort.write('t');
  myPort.write(1);
  for (int i = 0; i < 3; i++) { 
    myPort.write(state[i]);
  }

  // setup the display
  size(900, 400);
  background(255);
  //prepare Buttons
  stroke(0);
  noFill();

  // R-Set-Bar
  //fill(200,0,0);
  for (int i = 0; i <= colorSet_width; i++) {
    int c = round( map(i, 0, colorSet_width, 0, 255) );
    stroke( c, 0, 0 );
    line(colorSet_xPos+i, rSet_yPos, colorSet_xPos+i, rSet_yPos+colorSet_height);
  }
  stroke(0);
  rect(colorSet_xPos, rSet_yPos, colorSet_width, colorSet_height); // R-set

  // G-Set-Bar
  //fill(0,200,0);
  for (int i = 0; i <= colorSet_width; i++) {
    int c = round( map(i, 0, colorSet_width, 0, 255) );
    stroke( 0, c, 0 );
    line(colorSet_xPos+i, gSet_yPos, colorSet_xPos+i, gSet_yPos+colorSet_height);
  }
  stroke(0);
  rect(colorSet_xPos, gSet_yPos, colorSet_width, colorSet_height); // G-set

  // B-Set-Bar
  //fill(0,0,200);
  for (int i = 0; i <= colorSet_width; i++) {
    int c = round( map(i, 0, colorSet_width, 0, 255) );
    stroke( 0, 0, c );
    line(colorSet_xPos+i, bSet_yPos, colorSet_xPos+i, bSet_yPos+colorSet_height);
  }
  stroke(0);
  rect(colorSet_xPos, bSet_yPos, colorSet_width, colorSet_height); // b-set

  // Buttons
  fill(150);
  rect(button1_xPos, button_yPos, button_width, button_height); // Button_1 - turn neopixel off
  rect(button2_xPos, button_yPos, button_width, button_height); // Button_2 - handshake
  fill(0, 0, 150);
  rect(button3_xPos, button_yPos, button_width, button_height); // Button_3 - give reward
  rect(button5_xPos, button_yPos, button_width, button_height); // Button_5 - give reward at 2nd unit
  fill(0, 150, 0);
  rect(button4_xPos, button_yPos, button_width, button_height); // Button_4 - turn neopixel on
  fill(1500, 0, 0);
  rect(button6_xPos, button_yPos, button_width, button_height); // Button_6 - make vibration
  rect(button7_xPos, button_yPos, button_width, button_height); // Button_7 - make vibration at 2nd unit
  fill(150);
  rect(button8_xPos, button_yPos, button_width, button_height); // Button_8 - delay test

  //indicators for touch buttons
  // and small buttons setting the target indicator
  ellipseMode(RADIUS);
  for (int i = 0; i < n_targets; i++) {
    fill(150);
    ellipse(indicator_xPos+i*indicator_distance, indicator_yPos, indicator_radius, indicator_radius);
    if (i == tID)
    {
      fill(250, 0, 0);
    }
    rect(indicator_xPos+i*indicator_distance, indicator_yPos+indicator_distance, indicator_radius, indicator_radius);
  }

  // indicators for digital in
  ellipseMode(RADIUS);
  for (int i = 0; i < 4; i++) {
    ellipse(digi_indicator_xPos+i*digi_indicator_distance, digi_indicator_yPos, digi_indicator_radius, digi_indicator_radius);
  }


  // delay and clear serial income (if some data is in the serial stream)
  delay(500);
  myPort.clear();

  // start the teensy data-collection
  myPort.write('a');
}

/********** main loop **********/
void draw() {

  // get constantly data and print it
  //print("Incomming Bytes: ");
  //println(myPort.available());
  while (myPort.available () > 1) { //it only becomes interesting if there are at least 2 bytes in the pipeline: 1 index and 1 value
    val = myPort.read();

    if (val == 'b') {
      last_sensor_state_2 = byte(myPort.read()); // last 8 buttons
      last_sensor_state_1 = byte(myPort.read()); // first 8 buttons
      continue;
    }

    if (val == 'r') {
      last_reward_val = myPort.read();
      continue;
    }

    if (val == 'd') {
      digi_input_source = myPort.read();
      digi_input_source -= 1; // 
      digi_input_value = myPort.read();
      //       print("Digital Input From Source: ");
      //       print(digi_input_source);
      //       print(" - Value: ");
      //       println(digi_input_value);

      // update drawing
      if (digi_input_value > 0)
      {
        fill(0, 255, 0);
      } else
      {
        fill(150);
      }
      ellipse(digi_indicator_xPos+digi_input_source*digi_indicator_distance, digi_indicator_yPos, digi_indicator_radius, digi_indicator_radius);
    }

    if (val == 'p') {
      digi_input_source = myPort.read() + 2;
      digi_input_value = myPort.read();

      // update drawing
      if (digi_input_value > 0)
      {
        fill(0, 255, 0);
      } else
      {
        fill(150);
      }
      ellipse(digi_indicator_xPos+digi_input_source*digi_indicator_distance, digi_indicator_yPos, digi_indicator_radius, digi_indicator_radius);
    }

    if (val == 'm') {
      // get the 2 byte
      int LSB_manual_reward = myPort.read();
      int Byte2_manual_reward = myPort.read();
      // transform in int
      int reward_duration = ((Byte2_manual_reward << 8) | LSB_manual_reward); // is this correct?
      print("Manual Reward Triggered; Duration: ");
      println(reward_duration);
    }

    if (val == 'g') {
      // results from delay test
      // get 6 bytes
      int LSB_delay_light = myPort.read();
      int Byte2_delay_light = myPort.read();
      int Byte3_delay_light = myPort.read();
      int LSB_delay_vib = myPort.read();
      int Byte2_delay_vib = myPort.read();
      int Byte3_delay_vib = myPort.read();
      // make the 2 3-byte values out of it
      light_delay_measured = ((Byte3_delay_light << 16) | (Byte2_delay_light << 8) | LSB_delay_light);
      vib_delay_measured = ((Byte3_delay_vib << 16) | (Byte2_delay_vib << 8) | LSB_delay_vib);
      // print it
      print("Delay Test: Light: ");
      print(light_delay_measured);
      print("| Vibrationt: ");
      println(vib_delay_measured);
    }

    //print value vor debugging
    //println(val);
  }

  // set indicators according to touch button state
  for (int i = 0; i < min(8, n_targets); i++) {

    if ((last_sensor_state_1 & ( 1 << i )) <= 0)
    {
      fill(0, 255, 0);
    } else
    {
      fill(150);
    }
    ellipse(indicator_xPos+i*indicator_distance, indicator_yPos, indicator_radius, indicator_radius);
  }
  if (n_targets > 8)
  {
    for (int i = 0; i < min(8, n_targets-8); i++) {

      if ((last_sensor_state_2 & ( 1 << i )) <= 0)
      {
        fill(0, 255, 0);
      } else
      {
        fill(150);
      }
      ellipse(indicator_xPos+(i+8)*indicator_distance, indicator_yPos, indicator_radius, indicator_radius);
    }
  }

  // set digital in indicator according to digital input
}

/********** main functions **********/
void mousePressed() {
  // check if mouse is inside one of the buttons

  // set color
  if (inRSet()) {
    // compute color val from the relative position in the button field
    int setVal = round( map(mouseX-colorSet_xPos, 0, colorSet_width, 0, 255) );
    //println(setVal);

    // adjust the new state and send it to serial port
    state[0] = setVal;

    send_target(state);

    //DEBUG
    print("Data sent: ");
    println(target_output);
  }

  if (inGSet()) {
    // compute color val from the relative position in the button field
    int setVal = round( map(mouseX-colorSet_xPos, 0, colorSet_width, 0, 255) );

    // adjust the new state and send it to serial port
    state[1] = setVal;

    send_target(state);
  }

  if (inBSet()) {
    // compute color val from the relative position in the button field
    int setVal = round( map(mouseX-colorSet_xPos, 0, colorSet_width, 0, 255) );

    // adjust the new state and send it to serial port
    state[2] = setVal;

    send_target(state);
  }

  // turn off
  if (inButton1()) {
    state[0] = 0;
    state[1] = 0;
    state[2] = 0;

    byte old_tID = tID;

    for (byte i = 0; i<n_targets; i++) {
      tID = i;
      send_target(state);
    }

    tID = old_tID;
  }

  // handshake
  if (inButton2()) {

    // using ints for connection
    myPort.write('h');

    // request from teensy not implemented yet
  }

  // do reward one sec / reward unit 1
  if (inButton3()) {
    // duration
    int dur = 1000;

    // split in two bytes
    int dur_1stByte = floor(dur / 255);
    int dur_2ndByte = dur - dur_1stByte;

    int[] reward_state = {0, dur_1stByte, dur_2ndByte};

    // send protocol
    send_reward(reward_state);
  }

  // do reward one sec / reward unit 2
  if (inButton5()) {
    // duration
    int dur = 1000;

    // split in two bytes
    int dur_1stByte = floor(dur / 255);
    int dur_2ndByte = dur - dur_1stByte;

    int[] reward_state = {1, dur_1stByte, dur_2ndByte};

    // send protocol
    send_reward(reward_state);
  }

  // turn LEDs on 100 grey level
  if (inButton4()) {
    state[0] = 100;
    state[1] = 100;
    state[2] = 100;

    byte old_tID = tID;

    for (byte i = 0; i<n_targets; i++) {
      tID = i;
      send_target(state);
    }

    tID = old_tID;
  }

  // do vibration one sec / vibration motor unit 1
  if (inButton6()) {
    // duration
    int dur = 1000;

    // split in two bytes
    int dur_1stByte = floor(dur / 255);
    int dur_2ndByte = dur - dur_1stByte;

    int[] reward_state = {0, dur_1stByte, dur_2ndByte};

    // send protocol
    send_vibration(reward_state);
  }

  // do vibration one sec / vibration motor unit 2
  if (inButton7()) {
    // duration
    int dur = 1000;

    // split in two bytes
    int dur_1stByte = floor(dur / 255);
    int dur_2ndByte = dur - dur_1stByte;

    int[] reward_state = {1, dur_1stByte, dur_2ndByte};

    // send protocol
    send_vibration(reward_state);
  }  

  // let the teensy do the delay test
  if (inButton7()) {
    // code is an 'f', no additional input
    myPort.write('f');
    // MaCaQuE will now stop everything, make the test, 
    // send the result with the protocol: 'g'; 3-byie light delay; 3-byte vibration delay 
    // and starts it's normal routine again
    // here: the results will be handled in the input loop
    // max duration is arround 30s 
    // MaCaQuE will not do anything when Blackrock communication is used
  } 

  // set target id
  for (byte i = 0; i<n_targets; i++) {
    if ((mouseX >= indicator_xPos+i*indicator_distance) && (mouseX <= indicator_xPos+i*indicator_distance+indicator_radius) && (mouseY >= indicator_yPos+indicator_distance) && (mouseY <= indicator_yPos+indicator_distance+indicator_radius)) {
      // make old button grey
      fill(150);
      rect(indicator_xPos+tID*indicator_distance, indicator_yPos+indicator_distance, indicator_radius, indicator_radius);
      // set to new id
      tID = i;
      // make new button red
      fill(255, 0, 0);
      rect(indicator_xPos+tID*indicator_distance, indicator_yPos+indicator_distance, indicator_radius, indicator_radius);
    }
  }
}

/********** additional functions **********/
boolean inRSet() {
  return ((mouseX >= colorSet_xPos) && (mouseX <= colorSet_xPos+colorSet_width) && (mouseY >= rSet_yPos) && (mouseY <= rSet_yPos+colorSet_height));
}

boolean inGSet() {
  return ((mouseX >= colorSet_xPos) && (mouseX <= colorSet_xPos+colorSet_width) && (mouseY >= gSet_yPos) && (mouseY <= gSet_yPos+colorSet_height));
}

boolean inBSet() {
  return ((mouseX >= colorSet_xPos) && (mouseX <= colorSet_xPos+colorSet_width) && (mouseY >= bSet_yPos) && (mouseY <= bSet_yPos+colorSet_height));
}

boolean inButton1() {
  return ((mouseX >= button1_xPos) && (mouseX <= button1_xPos+button_width) && (mouseY >= button_yPos) && (mouseY <= button_yPos+button_height));
}

boolean inButton2() {
  return ((mouseX >= button2_xPos) && (mouseX <= button2_xPos+button_width) && (mouseY >= button_yPos) && (mouseY <= button_yPos+button_height));
}

boolean inButton3() {
  return ((mouseX >= button3_xPos) && (mouseX <= button3_xPos+button_width) && (mouseY >= button_yPos) && (mouseY <= button_yPos+button_height));
}

boolean inButton4() {
  return ((mouseX >= button4_xPos) && (mouseX <= button4_xPos+button_width) && (mouseY >= button_yPos) && (mouseY <= button_yPos+button_height));
}

boolean inButton5() {
  return ((mouseX >= button5_xPos) && (mouseX <= button5_xPos+button_width) && (mouseY >= button_yPos) && (mouseY <= button_yPos+button_height));
}

boolean inButton6() {
  return ((mouseX >= button6_xPos) && (mouseX <= button6_xPos+button_width) && (mouseY >= button_yPos) && (mouseY <= button_yPos+button_height));
}

boolean inButton7() {
  return ((mouseX >= button7_xPos) && (mouseX <= button7_xPos+button_width) && (mouseY >= button_yPos) && (mouseY <= button_yPos+button_height));
}

boolean inButton8() {
  return ((mouseX >= button8_xPos) && (mouseX <= button8_xPos+button_width) && (mouseY >= button_yPos) && (mouseY <= button_yPos+button_height));
}

void send_target(int[] p_state)
{
  target_output[0] = 't';
  target_output[1] = tID;
  for (int i = 0; i < 3; i++) { 
    target_output[i+2] = (byte)p_state[i];
  }
  myPort.write(target_output);
}

void send_reward(int [] p_state)
{
  reward_output[0] = 'r';
  for (int i = 0; i < 3; i++) 
  { 
    reward_output[i+1] = (byte)p_state[i];
  }
  myPort.write(reward_output);
}

void send_vibration(int [] p_state)
{
  vibration_output[0] = 'v';
  for (int i = 0; i < 3; i++) 
  { 
    vibration_output[i+1] = (byte)p_state[i];
  }
  myPort.write(vibration_output);
}