/*
 * RFreceiver.cpp
 *
 *  Created on: 26/05/2014
 *      Author: andrew
 */
#include "Arduino.h"
#include "RFreceiver.h"
#include <FixedQueueArray.h>

//used to record signal duration into bits
#define TOO_SHORT 5
#define ZERO_BIT 0
#define ONE_BIT 1
#define LONG_GAP 4

//What pin the data wire from the 433MHz RF module is connected to
//Change this if you are not using a Leonardo arduino
const int INPUT_PIN=7; //used by digital read
const int INTERRUPT_NUMBER=4; //on leonardo, pin 7 is interrupt 4 (see http://arduino.cc/en/Reference/attachInterrupt)

//Forward declare static member objects

volatile unsigned long RF_reciever::lastTransitionTimeHigh=0;
volatile unsigned long RF_reciever::lastTransitionTimeLow=0;
volatile bool RF_reciever::interruptIsRunning=false;
bool  RF_reciever::debug=false;
int RF_reciever::preamble=0;
int RF_reciever::id=0;
bool RF_reciever::batteryOK=false;
bool RF_reciever::ManualTransmit=false;
int RF_reciever::channel=0;
int RF_reciever::temperature10=0;
bool RF_reciever::negativeTemperature=false;
int RF_reciever::relativeHumidity=0;
unsigned long RF_reciever::timeMicros=0;
unsigned int RF_reciever::noiseCount=0;
FixedQueueArray<unsigned long> RF_reciever::queueTime;
FixedQueueArray<bool> RF_reciever::queuePinState;

volatile int RF_reciever::bitNumber=0;

RF_reciever::RF_reciever() {

	//Set pin 7 to input so it can be used for interrupts
	pinMode(INPUT_PIN,INPUT);

	debug=false;
	resetAll();
	printer=NULL;
}

RF_reciever::~RF_reciever() {

}

void RF_reciever::setup(int queueSize)
{
	//set the size of the queues.
	//If you set it too large then the arduino will run out of ram,
	// too small and it won't collect a full message.

	queueTime.resize(queueSize);
	queuePinState.resize(queueSize);
}

void RF_reciever::turnDebugON()
{
	debug=true;
}

void RF_reciever::turnDebugOFF()
{
	debug=false;
}

void RF_reciever::setPrinter (HardwareSerial & p)
{

	//set which serial port to send debug messages to
	printer = &p;

	//pass the serial port to the QueueArrays so they cna print error messages
  queueTime.setPrinter(p);
  queuePinState.setPrinter(p);

}

void RF_reciever::resetAll()
{
	//reset the messages
	RF_reciever::bitNumber=0;
	resetMessage();

	//empty the queues
	while(!queueTime.isEmpty())
	{
		queueTime.dequeue();
	}

	while(!queuePinState.isEmpty())
	{
		queuePinState.dequeue();
	}

}


void RF_reciever::overflowCheck()
{

	//the micros timer overflows every 70 minutes so reset everything if this happens
	if(micros()<lastTransitionTimeHigh)
	{
		if(interruptIsRunning)
		{
			restart();
		}
		else
		{
			resetAll();
		}
	}

}

void RF_reciever::start()
{
	//reset messages and attach the interrupt to pin 7
	resetAll();
	lastTransitionTimeHigh=micros();
	lastTransitionTimeLow=lastTransitionTimeHigh;
	interruptIsRunning=true;
	attachInterrupt(INTERRUPT_NUMBER,RF_reciever::isrPinChange,CHANGE);
}

void RF_reciever::stop()
{
	//stop the interrupt
	detachInterrupt(INTERRUPT_NUMBER);
	interruptIsRunning=false;
}

void RF_reciever::restart()
{
	stop();
	start();
}


bool RF_reciever::checkForMessages()
{
	//looping tasks that need to be performed
	overflowCheck();
	if(!interruptIsRunning)
	{
		start();
	}

	if(convertPulses2Bits())
	{
		//have a valid message!

		delay(500); //allow half a second for a few more bits to arrive before stopping the interrupt
		stop(); //stop the interrupt from firing
		return true; //message found so return true
	}
	return false; //no message found so return false
}

bool RF_reciever::checkQueueTailForExtraMessages()
{
	return convertPulses2Bits();
}

bool RF_reciever::validateMessage()
{

		if(preamble==5 //preamble needs to be '0101'
			and temperature10<=600 //temperature is below 60 degrees C
			and relativeHumidity>=0 //humidity is valid
			and relativeHumidity<=100
			)
		{
				debugPrint("Message is valid\n");
				printMessage();
			return true;
		}
		else
		{

				debugPrint("Message is invalid\n");
				printMessage();
			return false;
		}

}

void RF_reciever::printMessage()
{

			debugPrint("Preamble= "+String(preamble)+"\n");
			debugPrint("ID=  "+String(id)+"\n");
			debugPrint("BatteryOK= "+String(batteryOK)+"\n");
			debugPrint("Manual Transmit= "+String(ManualTransmit)+"\n");
			debugPrint("Channel= "+String(channel)+"\n");
			debugPrint("TimeMillis= "+String(timeMicros)+"\n");
			debugPrint("Negative Temperature= "+String(negativeTemperature)+"\n");
			debugPrint("Temperature= "+String(temperature10)+"\n");
			debugPrint("RelativeHumidity= "+String(relativeHumidity)+"\n");
}




//parses the queue of bits and translates them into messages
//returns "true" if a valid message was found
bool RF_reciever::convertBitToMessage(byte currentBit)
{
	//increment noise counter (used by heartbeat function)
	noiseCount++;

			if(currentBit==LONG_GAP)
			{
				debugPrint(F("\nGap"));
				resetMessage();
				return false;
			}
			else
			{
				debugPrint(String(currentBit));
			}


			//process 'ones', 'zeros' should have been done by resetMessage() function
		if(currentBit==ONE_BIT) // so process it but only if bit==1
		{
			if(bitNumber<=3) //preamble
			{
				bitSet(preamble,3-bitNumber);
			}
			else if(bitNumber>3 and bitNumber<=11) //ID
			{
				//check preamble
				if(bitNumber==4 and preamble!=5)
				{
					debugPrint(F("\nBadPre")); //bad preamble so exit
					resetMessage();
					return false;
				}

				bitSet(id,11-bitNumber); //set ID
			}
			else if(bitNumber==12) //battery
			{
				batteryOK=true;
			}
			else if(bitNumber==13) //manual transmit
			{
				ManualTransmit=true;
			}
			else if(bitNumber>13 and bitNumber<=15) //channel
			{
				bitSet(channel,15-bitNumber);
			}
			else if(bitNumber==16) //negative temperature flag
			{
				negativeTemperature=true;
			}
			else if(bitNumber>16 and bitNumber<=27) //temperature10 number
			{
				bitSet(temperature10,27-bitNumber);
			}
			else if(bitNumber>27 and bitNumber<=35) //relative humidity number
			{
				bitSet(relativeHumidity,35-bitNumber);
			}
		}

		if(bitNumber<35)
		{
			RF_reciever::bitNumber++;
		}
		else
		{
			//end of message so timestamp and validate

			timeMicros=micros(); //approximate time of this message
			RF_reciever::bitNumber=0;

			debugPrint(F("\nNew Message!\n"));


			if(validateMessage())
			{
				//have a new valid message!
				return true;

			}
			else
			{
				//message was invalid, clear message and begin again
				resetMessage();
				return false;
			}

		}

	return false; //no complete and valid message yet

}

void RF_reciever::resetMessage()
{
	 bitNumber=0;
	 preamble=0;
	 id=0;
	 batteryOK=false;
	 ManualTransmit=false;
	 negativeTemperature=false;
	 channel=0;
	 temperature10=0;
	 relativeHumidity=0;
	 timeMicros=0;

}


//This function is called whenever pin7 (interrupt 4 on leonardo) has a rising or falling edge
// It's designed to be small and quick
void RF_reciever::isrPinChange()
{
	//Read if the pin is high or low and record the time
	queueTime.enqueue(micros());
	queuePinState.enqueue(digitalRead(INPUT_PIN));
}

//Classifies the short, long and very long pulses into bits
//and then processes any bits found
//Returns true if a valid message was found
bool RF_reciever::convertPulses2Bits()
{
	unsigned long timeNow; //approximate time interrupt fired
	unsigned long signal_duration; //microseconds since the last interrupt transition
	byte zero_or_one;
	int pinState;
	bool previousSignalExists; //if 'chirp (i.e. short transmit pulse) occurred before the current silence

	//Get the time and pinStates stored in the queues
	while(!queueTime.isEmpty() and !queuePinState.isEmpty() )
	{
		timeNow=queueTime.dequeue();
		pinState=queuePinState.dequeue();

		if(pinState==HIGH)
		{
			//Rising edge logic
			signal_duration=timeNow-RF_reciever::lastTransitionTimeLow; //how long silence was

			if(signal_duration<1500)
			{
				//too short (i.e. noise), so ignore
				continue; //go to next pulse in queue
			}
			else if(signal_duration<=2500)
			{
				//short silence
				zero_or_one=ZERO_BIT; //a "zero" was received
			}
			else if(signal_duration<5000)
			{
				//long silence
				zero_or_one=ONE_BIT; //a "one" was received
			}
			else
			{
				//very long silence
				zero_or_one=LONG_GAP; //a long gap at the start or end of a signal
			}


			RF_reciever::lastTransitionTimeHigh=timeNow;

			//if this silence was proceeded by a pulse of around 500uS long then the the bit is valid
			if(previousSignalExists)
			{
				//we have a valid bit

				previousSignalExists=false; //reset flag

				//send this bit to become part of a Message
				bool haveMessage=convertBitToMessage(zero_or_one);
				if(haveMessage){
					return true; //have a valid and complete message so call home
				}
			}

		}
		else
		{
			//Falling edge logic (check for around 500uS chirp)

			signal_duration=timeNow-RF_reciever::lastTransitionTimeHigh; //how long the chirp lasted
			if(signal_duration<350)
			{
				//too short since last went high so ignore (i.e. noise)
				continue; //process next item in queue
			}
			else if(signal_duration>650)
			{
				//stayed too long in the HIGH state (i.e. not a chirp, just noise on this frequency?)
				RF_reciever::lastTransitionTimeLow=timeNow;
			}
			else
			{
				//else there has been a valid chirp!
				previousSignalExists=true; //set flag to wait for a silent period now

				RF_reciever::lastTransitionTimeLow=timeNow;
			}

		}
	}
	//finished processing all items in the queue and no valid messages so return false
	return false;

}

void RF_reciever::debugPrint(String s)
{
	//if debug is enabled and a serial port is attached then print the string to the serial
	if(debug and printer)
	{
		//clear serial read buffer
		while(printer->available())
		{
			printer->read();
		}

		printer->print(s);
		printer->flush();

	}
}

void RF_reciever::debugPrint(const __FlashStringHelper * s)
{
	if(debug and printer)
		{
			//clear serial read buffer
			while(printer->available())
			{
				printer->read();
			}

			//clear serial write buffer
			printer->flush();

			printer->print(s);
			printer->flush();


		}
}
