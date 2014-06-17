/*
 * EthernetWeather.h
 *
 *  Created on: 26/05/2014
 *      Author: andrew
 */

#ifndef ETHERNETWEATHER_H_
#define ETHERNETWEATHER_H_


#include "Arduino.h"
#include <Ethernet.h>

/**
 * This class sends temperature and humidity data to a web server as a JSON POST packet.
 */
class EthernetWeather {
public:
	/**
	 * Constructor
	 */
	EthernetWeather();
	/**
	 * Unused destructor
	 */
	virtual ~EthernetWeather();

	/**
	 * Sets up the ethernet sheild by assigning it a MAC address and static IP (change these if you need)
	 */
	void setup();
	/**
	 * Print debug messages to the serial port set by the setPrinter() function
	 */
	void turnDebugON();
	/**
	 * Don't print debug messages to the serial port. Faster and saves RAM
	 */
	void turnDebugOFF();
	/**
	 * Set which hardware serial port to print debug messages to
	 * @param p a hardware serial port to print debug messages to
	 */
	void setPrinter (HardwareSerial & p);
	/**
	 * Gets the temperature and humidity data from the RF_reciever class and sends a JSON POST message to a web server
	 */
	void sendMessage();
	/**
	 * Sends a JSON POST packet to a web server saying "OK" and listing how many bits the 433MHz receiver has
	 * recieved (including noise) since the last heartbeat.
	 */
	void sendHeartbeat();

	/**
	 * The MAC address of this arduino
	 */
	static uint8_t mac[];

	/**
	 * The static IP address of this Arduino
	 */
	static IPAddress myIP;
	/**
	 * The IP address of the web server to send messages to
	 */
	//static IPAddress serverIP;

	/**
	 * The IP address of your local DNS server (if you have one)
	 */
	static IPAddress dnsIP;

	/**
	 * Internet gateway IP address
	 */
	static IPAddress gateway; //internet router gateway

	/**
	 * URL of webserver where temperature should be logged. Use this or serverIP
	 */
	static const char webServerURL[];

	/**
	 * The port the web server is running on (usually 80)
	 */
	static uint16_t port;


private:
	/**
	 * Generates a string in the form "IP address:port"
	 * @param ip The IP address
	 * @param dest the string that is passed back
	 * @param port the port number
	 */
	void stringIPandPort(IPAddress ip, String &dest,uint16_t port=80);

	/**
	 * Generates a string in the form url:port"
	 * @param url The url
	 * @param dest the string that is passed back
	 * @param portNum the port number
	 */
	void stringURLandPort(const char * url, String &dest,uint16_t portNum=80);

	/**
	 * Makes a JSON string containing temperature and humidity that will be sent to the server
	 */
	void makeJSONstring();
	/**
	 * Prints a debug messages to the serial port
	 * @param s the message to be printed
	 */
	void debugPrint(String s);
	/**
	 * Overloaded debugPrint that uses the PROGMEM F() macro to be more RAM efficient
	 * @param s A string wrapped in the F() macro
	 */
	void debugPrint(const __FlashStringHelper * s);
	/**
	 * Attempts to open a connection to the web server
	 */
	void connectToserver();

	/**
	 * A class used to send messages to the web server
	 */
	static EthernetClient client;

	static String jsonData; /** the JSON string that is sent to the web server */

	static bool debug; /** If debug messages should be printed to the serial port */
	HardwareSerial * printer; /** Indirect reference to the serial port */


};

#endif /* ETHERNETWEATHER_H_ */
