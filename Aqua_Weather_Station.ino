#include "Aqua_Weather_Station.h"
#include <FixedQueueArray.h>
#include "RFreceiver.h"

#include <Ethernet.h>
#include <EthernetClient.h>
#include "EthernetWeather.h"

RF_reciever receiver; /** An instance of the RF_reciever class that processes signals from the radio */

EthernetWeather ethernetWeather; /** An instance of the ethernetWeather class that sends messages to a web server */

long heartbeatTimer; /** Timer used to send 'heartbeat' (e.g. I'm still here) messages to the web server every 5 minutes */

int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}


/**
 * Sets up the 433Mhz receiver, ethernet link and serial debugging message at the start of the sketch
 */
void setup()
{
	//control if debug messages should be sent to a serial port
	//set up this first so you can see if you've made the queues too big and have run out of RAM
	//disable this IF statement to save RAM and sketch size
	if(true)
	{
		//enable debugging for 433MHz receiver
		receiver.setPrinter(Serial1); //print error messages to Serial1 (pins 0 and 1 on Leonardo)
		receiver.turnDebugON(); //turn debug on for this class

		ethernetWeather.setPrinter(Serial1); //print ethernet messages to Serial1 port (pins 0 and 1 on Leonardo)
		ethernetWeather.turnDebugON();; //turn debug on for this class

		//start the serial port at 57600 baud
		Serial1.begin(57600);
		delay(100); //allow serial port 0.1 second to start
		Serial1.println(F("Starting..."));
	}

	Serial1.print("Before reciever setup, free ram is: ");
	Serial1.println(freeRam());
	//set up the 433MHz class on pin 7 and set the size of the queue to be 64 elements (too big and you run out of RAM)
	//set up 433Mhz receiver with debug on
	receiver.setup(150);

	Serial1.print("After reciever setup, free ram is: ");
	Serial1.println(freeRam());

	//setup ethernet connection
	ethernetWeather.setup();
	Serial1.print("After ethernet setup, free ram is: ");
	Serial1.println(freeRam());
	delay(10000);


	//initialise the heartbeat timer
	heartbeatTimer=millis();

}

/**
 * This function is called in an endless loop. It listens for messages from the weather transmitter and
 * sends a json package to a web server when a message is detected. It also sends a message to the web server every 5 minutes
 */
void loop()
{
	//start listening for messages
	if(receiver.checkForMessages())
	{
		//message was found

		//send the message as a json POST to the web server
		ethernetWeather.sendMessage();

		//check for repeat messages in the queue and send them too
		while(receiver.checkQueueTailForExtraMessages())
		{
			ethernetWeather.sendMessage();
		}

		//when the first message was found the hardware interrupt was stopped and so restart it now.
		receiver.start();

	}

	//every 5 minutes (300,000mS) send a 'heartbeat' message to let the server know the link is okay
	if(millis()-heartbeatTimer>300000 or millis()<heartbeatTimer)
	{
		//disable interrupt on pin 7
		receiver.stop(); // Not sure if this is needed? Does ethernet shield SPI interface screw up hardware interrupts?

		//send heartbeat message to web server
		ethernetWeather.sendHeartbeat(); //

		//reset hearbeat timer
		heartbeatTimer=millis();

		// Re-enable/restart hardware interrupts
		receiver.start(); // Needed?
	}

}




