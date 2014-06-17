/*
 * RFreceiver.h
 *
 *  Created on: 26/05/2014
 *      Author: andrew
 */

#ifndef RFRECEIVER_H_
#define RFRECEIVER_H_

#include <FixedQueueArray.h>

/**
 * Processes messages coming from the 433MHz receiver
 * which is listening for signals coming from the Aqua
 * weather station.
 */
class RF_reciever {
public:
	/**
	 * Constructor
	 */
	RF_reciever();
	/**
	 * Unused destructor
	 */
	virtual ~RF_reciever();

	/**
	 * Sets the size of the signal queues that the interrupts write to.
	 * If you set these too high then the arduino runs out of RAM,
	 * too low and a full message won't be received. 64 works ok.
	 * @param queueSize how large to set the queues to be
	 */
	void setup(int queueSize=64);

	/**
	 * Starts the interrupt listening on pin 7
	 */
	void start();
	/**
	 *  Stops the interrupt listening on pin 7
	 */
	void stop();

	/**
	 * The main loop function that checks for new messages. Call this often.
	 * @return true if a valid message has been found, false otherwise
	 */
	bool checkForMessages();

	/**
	 * Once the function checkForMessages() has found a valid message there may be
	 * repeats of the same messages in the queue that haven't been processed yet.
	 * This function allow the remainder of the queue to checked for repeat messages
	 * @return true if another message has been found in the queue
	 */
	bool checkQueueTailForExtraMessages();

	/**
	 * Turns debug ON so that messages are printer to the serial device defined by printer.
	 */
	void turnDebugON();
	/**
	* Turns debug OFF so that no messages are printer to the serial device defined by printer.
	* Set this to make the program run faster.
	*/
	void turnDebugOFF();
	 /**
	  * Allows you to set which serial device to print debug messages to
	  * @param p a hardware serial port on your arduino (e.g. Serial1)
	  */
	 void setPrinter (HardwareSerial & p);

	 /**
	  * Prints the temperature, humidity, etc. to the serial port
	  */
	void printMessage();

	/*Message data */
	// TODO: convert message data into a struct

		 static int preamble; /** Message preamble, should be 0101 or 5 */
		 static	int id; /** ID of the weather station */
		 static	bool batteryOK; /** Is the battery in the station OK */
		 static	bool ManualTransmit; /** If true then someone has pressed the TX button on the weather station */
		 static	int channel; /** Channel unit is transmitting on (zero to three) */
		 static	int temperature10; /** The temperature in celsius multiplied by 10 (e.g. 21.5 degrees =215) */
		 static	bool negativeTemperature; /** flag to indicate temperature is less than zero */
		 static	int relativeHumidity; /** Relative humidity */
		 static	unsigned long timeMicros; /** Milliseconds since arduino started */
		 static unsigned int noiseCount; /** How many bits have been received by the 433MHz rx between heartbeats */

private:
	/** This function is called every time pin 7 changes.
	 * It records the time of the change and the pin polarity to queues *
	 */
	 static void isrPinChange();
	 /**
	  * Clears messages and queues
	  */
	 void resetAll();
	 /**
	  * Restarts the interrupt. Called if the micros() timer overflows
	  */
	 void restart();
	 /**
	  * Checks if the micros() timer has overflown (happens every 70min)
	  */
	 void overflowCheck();
	 /**
	  * Clears all the message data
	  */
	 void resetMessage();
	 /**
	  * Validates the message (e.g. checks temperate valid etc.)
	  * @return true if the message is valid
	  */
	 bool validateMessage();
	 /**
	  * Converts the raw data from the receiver into zeros, ones and long gaps if a pulse has been detected.
	  * @return true if a valid message has been found, false otherwise.
	  */
	 bool convertPulses2Bits();
	 /**
	  *
	  * @param currentBit a zero, one or long gap created by the convertPulses2Bits() function
	  * @return true if a valid message has been found, false otherwise
	  */
	 bool convertBitToMessage(byte currentBit);


	 /**
	  * Prints the string s to the serial port if debug is enabled and a
	  * serial port has been attached by the setPrinter() function.
	  * @param s A string to print to the serial port
	  */
	 void debugPrint(String s);
		/**
		 * Overloaded debugPrint that uses the PROGMEM F() macro to be more RAM efficient
		 * @param s A string wrapped in the F() macro
		 */
		void debugPrint(const __FlashStringHelper * s);

	 /*Object members*/

	 HardwareSerial * printer; /** A serial port on the arduino to print messages to */
	 static FixedQueueArray<unsigned long>  queueTime; /** A queue to store what time the interrupt function was fired */
	 static FixedQueueArray<bool>  queuePinState; /** A queue to store the polarity of the interrupt pin when it fired */

	 /*members*/

	 static bool debug; /** If printing messages to the serial is enabled or not */
	 static volatile unsigned long lastTransitionTimeHigh; /** Last time the interrupt pin was HIGH */
	 static volatile unsigned long lastTransitionTimeLow;  /** Last time the interrupt pin was LOW */
	 static volatile int bitNumber; /** The bit number of the current signal (0 to 35) - 36bits in message */
	 static volatile bool interruptIsRunning; /** Flag to indicate if interrupt is enabled */


};

#endif /* RFRECEIVER_H_ */
