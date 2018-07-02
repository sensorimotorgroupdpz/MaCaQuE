/*
* Test file for MaCaQuE and serial_communication class
*
* This is a simple command line interface to communicate with MaCaQuE based on the serial_communication class.
* It is necessary to set the correct communication port in the code (see below).
* 
* author: Michael Berger
* German Primate Center
* mail: mberger@dpz.eu
* 
*
* related publication:
* Berger M, Gail A (2018) The Reach Cage environment for wireless neural recordings during structured goal-directed behavior of unrestrained monkeys, bioRxiv
* https://www.biorxiv.org/content/early/2018/04/24/305334
*/


//global include
#include <iostream>
#include <cstdio>
#include <string>
#include <thread>
#include <deque>
#include <future> 
#include <atomic>
#include <mutex>
#include <vector>
#include <chrono>

//project include
#include "serial_communication.h"

using namespace std;

// global vars
//deque<int> input_buffer(10);
//serial_communication mySerialComm;
//bool running = false;

mutex input_mutex;
mutex serial_mutex;
// number of targets
int n_targets = 16;
bool handshake_flag = false;

// thread call function
void input_thread_function(atomic<bool> & running, serial_communication & mySerialComm, atomic<int> & current_reward_input, atomic<int> & current_button_input, atomic<int> & bytes_received)//, vector<unsigned char> & p_input_vector) 
{

    
    double start_time;
    int time_to_finish;
    int current_button = 0;
    int current_reward = 0;

    while (running.load())
    {
        unsigned char input_buffer[128];

        // get start time

        int bytesReceived;
        {
            lock_guard<mutex> guard(serial_mutex);
            // thread get serial input
            //read
            bytesReceived = mySerialComm.read_data(input_buffer, 128);
        }

        for (int i=0; i < bytesReceived; i++)
        {
            if (input_buffer[i] == 'r')
            {
                current_reward = static_cast<int>(input_buffer[i+1]);
            }
            
            if (input_buffer[i] == 'b')
            {
				int first_byte = static_cast<int>(input_buffer[i + 1]);
				int second_byte = static_cast<int>(input_buffer[i + 2]);
				current_button = (first_byte << 8) | second_byte;
            }

			if (input_buffer[i] == 'd')
			{
				// Digital Inputs
				// first byte is the number if the digital input
				// second byte is the new state (1 or 0)
			}

			if (input_buffer[i] == 'h')
			{
				// Handshake
				handshake_flag = true;
				// first byte is the number if the digital input
				// second byte is the new state (1 or 0)
			}
        }
            
        // store most current input value
        {
            lock_guard<mutex> guard(input_mutex);

            current_button_input.store(current_button);
            current_reward_input.store(current_reward);
            bytes_received.store(bytesReceived);
            //p_input_vector.assign(begin(input_buffer), end(input_buffer));
        }

        // put in a deque

        // get end-time 
        // compute time_to_finish
        time_to_finish = 20;

        // pause 
        this_thread::sleep_for(chrono::milliseconds(time_to_finish));
    }

}

int main(int argc, char* argv[])
{  
    /********** prepare Serial **********/
    // serial input buffer
    unsigned char input[32768] = "";

    // open communication
    serial_communication mySerialComm("COM6"); //<------------------------------- COMMUNICATION PORT - change accordingly!
    mySerialComm.connect();

    if (mySerialComm.get_connected())
    {
        cout << "Hello World!" << endl;

        // reward and LED test
        // turn on all targets
        for (int i = 0; i < n_targets; i++)
        {
            // output BYTE array
            BYTE state_on[5] = {'t', i, 255, 255, 255};
            // transmitt the array
            mySerialComm.transmit_bytes(state_on, 5);
        }

        // turn on reward
        // output BYTE array
        BYTE rew_state[4] = {'r', 1, 2, 0};
        // transmitt the array
        mySerialComm.transmit_bytes(rew_state, 4);

        // wait
        Sleep(500);

        // turn off all targets
        for (int i = 0; i < n_targets; i++)
        {
            // output BYTE array
            BYTE state_off[5] = {'t',i, 0, 0, 0};
            // transmitt the array
            mySerialComm.transmit_bytes(state_off, 5);
        }
    }

    // get status
    mySerialComm.get_status();
    mySerialComm.print_communication_state();


    /********** input thread **********/
    //deque<int> input_buffer(10);
    atomic<int> current_button_input;
    atomic<int> current_reward_input;
    atomic<bool> running(true);
    atomic<int> bytes_received;
    vector<unsigned char> input_vector;
	atomic<int> saved_time;

    // make a thread for getting input
    thread input_thread(&input_thread_function, ref(running), ref(mySerialComm), ref(current_reward_input), ref(current_button_input), ref(bytes_received));//, ref(input_vector));

    /********** main loop **********/

    // start teensy data collect
    BYTE on[1] = {'a'};
    mySerialComm.transmit_bytes(on, 1);


    while (true)
    {
        // purge data in in and output buffer
        //mySerialComm.purge(2); 
        //mySerialComm.purge(4);

        char order;

        cout << "************************\n Enter: k - get data || c - most current input || n - send data || r - set reward || s - status || h - handshake || b - send 16bit word to BR || v - vibration motor || e - test ouput signal || q - quit " << endl;
        // ask for a command (char)
        cin >> order;

        switch (order)
        {
            // quit
        case 'q':
            { 
                // msg
                cout << "Main Bye!" << endl;

                // input thread
                running.store(false);
                input_thread.join();

                // stop teensy data collect
                BYTE off[1] = {'z'};
                mySerialComm.transmit_bytes(off, 1);

                // shut off targets
                // output BYTE array
                BYTE state[3] = {'c', 'c', 'c'}; // a reset including turning off everything
                // transmitt the array
                mySerialComm.transmit_bytes(state, 3);


                return 0;
            }

            //input

        case 'k':
            {
                int duration;
                int i = 0;

                cout << " How many times do you want to read? Enter: ";
                // how long do yoo want to get input
                cin >> duration;


                while (i < duration)
                {
                    int display_input;
                    int display_received_bytes;
                    vector<unsigned char> display_vector;
                    vector<unsigned int> current_button;
                    vector<unsigned int> current_reward;
                    {
                        lock_guard<mutex> guard(input_mutex);
                        //get the data from the future deque
                        display_input = current_button_input.load();
                        display_received_bytes = bytes_received.load();
                        display_vector.swap(input_vector);
                    }

                    //print
                    /*cout <<  display_received_bytes << " bytes received // " 
                        << "uint: "  << static_cast<unsigned int>(display_input) << " -- "
                        << "noncon: "<< display_input                            << " -- "
                        << "int: "   << static_cast<int>(display_input)          << endl;*/
                    if (display_received_bytes > 0)
                    {
                        for (int j = 0; j < 128; j++)
                        {
                            if (display_vector.at(j) == 'b')
                            {
                                current_button.assign(&display_vector.at(j+1), &display_vector.at(j+31));
                            }
                            if (display_vector.at(j) == 'r')
                            {
                                current_reward.assign(&display_vector.at(j+1), &display_vector.at(j+31));
                            }
                        }

                        cout << "Button Values: | ";
                        for (int j = 0; j < 30; j++)
                        {
                            cout << current_button.at(j) << " | ";
                        }
                        cout << endl;
                        cout << "Reward Sensor Values: | ";
                        for (int j = 0; j < 30; j++)
                        {
                            cout << current_reward.at(j) << " | ";
                        }
                        cout << endl;
                    }
                    else
                    {
                        cout << "No bytes received!" << endl;
                    }

                    // pause
                    Sleep(100);

                    i++;
                }

                break;
            }

            // get most current input
            case 'c':
            {
                int duration;
                int i = 0;

                cout << " How many times do you want to read? Enter: ";
                // how long do yoo want to get input
                cin >> duration;

                while (i < duration)
                {
                    int display_button_input;
                    int display_reward_input;
                    int display_received_bytes;
					std::vector<bool> button_states;

					for (int j = 0; j < n_targets; j++)
					{
						button_states.push_back(false);
					}
                    

                    {
                        lock_guard<mutex> guard(input_mutex);
                        //get the data from the future deque
                        display_button_input = current_button_input.load();
                        display_reward_input = current_reward_input.load();
                        display_received_bytes = bytes_received.load();
                    }

                    // check the button byte for "0s" which indicate a button press
                    for (int j=0; j<n_targets; j++)
                    {
                        if ((display_button_input & ( 1 << j )) <= 0)
                        {
                            button_states.at(j) = true;
                        }
                    }

                    if (display_received_bytes > 0)
                    {
                        cout << "Bytes received: " << display_received_bytes <<  " Button: " << display_button_input << " Reward: " << display_reward_input << endl;
                        cout << "Button states: ";
                        for (int j=0; j<n_targets; j++)
                        {
                            if (button_states.at(j))
                            {
                                cout << "| ON |";
                            }
                            else
                            {
                                cout << "| OFF |";
                            }
                        }
                        cout << endl;
                    }
                    else
                    {
                        cout << "No input received" << endl;
                    }

                    // pause
                    Sleep(100);

                    i++;
                }

                break;
            }

            //output
        case 'o':
            {
                // define output
                string out;

                // the way of intepreting inputs could be definitely smarter
                // parameters
                int rVal;
                int gVal;
                int bVal;
                int tone;

                // Note 
                cout << "Set parameter" << endl;
                // set the parameter values
                cout << "R-Value: ";
                cin >> rVal;
                rVal = max(rVal, 0);
                rVal = min(rVal, 255);
                cout << "G-Value: ";
                cin >> gVal;
                gVal = max(gVal, 0);
                gVal = min(gVal, 255);
                cout << "B-Value: ";
                cin >> bVal;
                bVal = max(bVal, 0);
                bVal = min(bVal, 255);
                cout << "Tone: ";
                cin >> tone;

                // output string
                //out = static_cast<char>(rVal);// + " " + static_cast<char>(gVal)  + static_cast<char>(bVal) + static_cast<char>(tone);
                char temp[8];
                sprintf_s(temp, "%d ",rVal);
                out = temp;
                sprintf_s(temp, "%d ",gVal);
                out += temp;
                sprintf_s(temp, "%d ",bVal);
                out += temp ;
                sprintf_s(temp, "%d",tone);
                out += temp;

                // output length
                int outLen = sizeof(out)/sizeof(out[0]);

                cout << "OUTPUT: " << out.c_str() << "\n OUTLENGTH: " << outLen << endl;

                // transmitt the string
                //mySerialComm.transmit_bytes(out.c_str(), outLen);

                // output BYTE array
                BYTE state[4] = {rVal, gVal, bVal, tone};
                {
                    lock_guard<mutex> guard(serial_mutex);
                    // transmitt the array
                    mySerialComm.transmit_bytes(state, 4);
                }

                break;
            }

            // new protocol
            case 'n':
            {
                // define output
                string out;

                // the way of intepreting inputs could be definitely smarter
                // parameters
                unsigned int targetID;
                unsigned int rVal;
                unsigned int gVal;
                unsigned int bVal;
                //unsigned int tone;

                // Note 
                cout << "Set parameter" << endl;
                // set the parameter values
                cout << "Target Number: ";
                cin >> targetID;
                targetID = max(targetID, 0);
                targetID = min(targetID, n_targets);
                cout << "R-Value: ";
                cin >> rVal;
                rVal = max(rVal, 0);
                rVal = min(rVal, 255);
                cout << "G-Value: ";
                cin >> gVal;
                gVal = max(gVal, 0);
                gVal = min(gVal, 255);
                cout << "B-Value: ";
                cin >> bVal;
                bVal = max(bVal, 0);
                bVal = min(bVal, 255);
                //cout << "Tone: ";
                //cin >> tone;

                // output string
                //out = static_cast<char>(rVal);// + " " + static_cast<char>(gVal)  + static_cast<char>(bVal) + static_cast<char>(tone);
                char temp[6];
                sprintf_s(temp, "%d ",rVal);
                out = temp;
                sprintf_s(temp, "%d ",gVal);
                out += temp;
                sprintf_s(temp, "%d ",bVal);
                out += temp ;
                //sprintf_s(temp, "%d",tone);
                //out += temp;

                // output length
                int outLen = sizeof(out)/sizeof(out[0]);

                cout << " STRING OUTPUT: " << out.c_str() << "\n STRING OUTLENGTH: " << outLen << endl;

                // transmitt the string
                //mySerialComm.transmit_bytes(out.c_str(), outLen);

                // output BYTE array
                BYTE state[5] = {'t', targetID, rVal, gVal, bVal};
                {
                    lock_guard<mutex> guard(serial_mutex);
                    // transmitt the array
                    int transmitted_bytes = mySerialComm.transmit_bytes(state, 5);            
                
                    cout << "OUTPUT: " << state << " ";
                    cout << transmitted_bytes << " bytes transmitted." << endl;
                }

                break;
            }
            
            case 'r':
            {
				// ask for reward unit
				int reward_unit;

				cout << "reward unit (0 or 1): ";
				cin >> reward_unit;

				if (reward_unit > 0)
				{
					reward_unit = 1;
				}

				// set reward duration
                int reward_duration;
                
                cout << "reward duration: ";
                cin >> reward_duration;

                unsigned int times_255 = floor(reward_duration / 255);
                unsigned int rest = reward_duration - times_255;

				BYTE out[4] = { 'r', reward_unit, times_255, rest };

                {
                    lock_guard<mutex> guard(serial_mutex);
                    // transmitt the array
                    int transmitted_bytes = mySerialComm.transmit_bytes(out, 4);
                
					cout << "OUTPUT: " << out[0] << " , " << out[1] << "," << out[2] << "," << out[3] << endl;
                    cout << transmitted_bytes << " bytes transmitted." << endl;
                }

                break;
            }

			// BR cerebus word
			case 'b':
			{
				// ask for input to send
				WORD br_input;
				
				cout << "send to BR: ";
				cin >> br_input;

				// extract higher and lower byte
				BYTE low_byte = br_input & 0xff;
				BYTE high_byte = (br_input >> 8) & 0xff;
				//unsigned int times_255 = floor(br_input / 255);
				//unsigned int rest = br_input - times_255;

				// output
				BYTE out[3] = { 'b', high_byte, low_byte };

				{
					lock_guard<mutex> guard(serial_mutex);
					// transmitt the array
					int transmitted_bytes = mySerialComm.transmit_bytes(out, 3);

					cout << "OUTPUT: " << out[0] << " , " << out[1] << "," << out[2] << endl;
					cout << transmitted_bytes << " bytes transmitted." << endl;
				}

				break;
			}

			// vibration motor
			case 'v':
			{
				// ask for vibration unit
				int vibration_unit;

				cout << "vibration motor unit (0 or 1): ";
				cin >> vibration_unit;

				if (vibration_unit > 0)
				{
					vibration_unit = 1;
				}

				// set reward duration
				int vibration_duration;

				cout << "vibration duration: ";
				cin >> vibration_duration;

				unsigned int times_255 = floor(vibration_duration / 255);
				unsigned int rest = vibration_duration - times_255;

				BYTE out[4] = { 'v', vibration_unit, times_255, rest };

				{
					lock_guard<mutex> guard(serial_mutex);
					// transmitt the array
					int transmitted_bytes = mySerialComm.transmit_bytes(out, 4);

					cout << "OUTPUT: " << out[0] << " , " << out[1] << "," << out[2] << "," << out[3] << endl;
					cout << transmitted_bytes << " bytes transmitted." << endl;
				}

				break;
			}

			case 'e':
			{
				// set reward duration
				int output_signal;

				cout << "PWM signal from 0-255: ";
				cin >> output_signal;

				output_signal = fmin(255, output_signal);
				
				unsigned char out[2] = { 'e', output_signal };

				{
					lock_guard<mutex> guard(serial_mutex);
					// transmitt the array
					int transmitted_bytes = mySerialComm.transmit_bytes(out, 2);

					cout << "OUTPUT: " << out[0] << " , " << out[1] << endl;
					cout << transmitted_bytes << " bytes transmitted." << endl;
				}


				break;
			}

			case 'h':
			{
				std::chrono::steady_clock::time_point start;

				// output BYTE array
				BYTE state[1] = { 'h' };
				{
					lock_guard<mutex> guard(serial_mutex);
					// transmitt the array
					start = std::chrono::steady_clock::now();
					int transmitted_bytes = mySerialComm.transmit_bytes(state, 6);
				}

				while (!handshake_flag)
				{
					std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
					if (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() > 100000)
					{
						break;
					}
				}
				handshake_flag = false;

				std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
				std::cout << "Handshake duration "
					<< std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
					<< "us.\n";
			}

            case 's':
            {
                // display a status
                {
                    lock_guard<mutex> guard(serial_mutex);
                    mySerialComm.get_status();
                    mySerialComm.print_communication_state();
                }
                break;                
            }
        }
    }

    return 0;
}