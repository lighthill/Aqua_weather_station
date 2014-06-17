/*
 * EthernetWeather.cpp
 *
 *  Created on: 26/05/2014
 *      Author: andrew
 */


#include "Arduino.h"
#include <Ethernet.h>
#include <EthernetClient.h>
#include "EthernetWeather.h"
#include "RFreceiver.h"

//Forward declarations

//generated MAC address from http://www.miniwebtool.com/mac-address-generator/
//Some forums say to only use ASCII printable characters
uint8_t EthernetWeather::mac[]={0x4E,0x50,0x48,0x5A,0x43,0x59};

IPAddress EthernetWeather::myIP(192, 168, 1, 177); //arduino static IP

//Comment out one of the follow sections so ether the webserver is accessed using it's IP or URL
//IPAddress EthernetWeather::serverIP(192,168,1,105); //

IPAddress EthernetWeather::dnsIP(192,168,1,130); //local DNS server (if you have one)
IPAddress EthernetWeather::gateway(192,168,1,254); //internet router gateway
const char EthernetWeather::webServerURL[]="arduinoTemperatureWebserver.local";

uint16_t EthernetWeather::port=12781; //web server port (usually 80)

String EthernetWeather::jsonData;
bool  EthernetWeather::debug=false; //prints output to serial if true

EthernetClient EthernetWeather::client;

EthernetWeather::EthernetWeather() {

	EthernetWeather::debug=false;
	printer=NULL;
	jsonData.reserve(150);
}

EthernetWeather::~EthernetWeather() {

}

void EthernetWeather::setup()
{
	//set up ethernet connection
	//Ethernet.begin(mac,myIP );
	Ethernet.begin(mac,myIP,dnsIP,gateway);
	delay(2000); //give the shield 2 seconds to set up

	//print MAC address and IP to serial if enabled
	debugPrint(F("Arduino MAC: "));
		for(int i=0; i<6; i++)
		{
			debugPrint(String(mac[i],HEX));
			debugPrint("-");
		}
		debugPrint(F("\nArduino static IP: "));
		String dest;
		stringIPandPort(myIP,dest);
		debugPrint(dest);
		debugPrint(F("\n"));


}

void  EthernetWeather::turnDebugON()
{
	debug=true;
}

void EthernetWeather::turnDebugOFF()
{
	debug=false;
}

void EthernetWeather::setPrinter (HardwareSerial & p)
{
  printer = &p;

}

void EthernetWeather::connectToserver() {
	//See if we are already connected to the server (shouldn't happen but just being safe)
	if (!client.connected()) {
		//Not connected so connect
		if (client.connect(webServerURL, port)) {
			//Connected Okay so print the IP
			//of server we just connected to
			debugPrint(F("Connected to "));
			String serverIPstring;
			//stringIPandPort(serverIP, serverIPstring, port);
			stringURLandPort(webServerURL, serverIPstring, port);
			debugPrint(serverIPstring);
			debugPrint(F("\n"));
		} else {
			//Can't connect to web server
			debugPrint(F("Failed to connect to"));
			String serverIPstring;
			//stringIPandPort(serverIP, serverIPstring, port);
			stringURLandPort(webServerURL, serverIPstring, port);
			debugPrint(serverIPstring);
			debugPrint(F("\n"));
		}
	}
}

void EthernetWeather::sendHeartbeat()
{

	//See if we are already connected to the server (shouldn't happen but just being safe)
	connectToserver();

		//Check if previous connection worked
		if (client.connected())
		{
			debugPrint(F("Sending heartbeat POST request\n"));

			//send the message as JSON packet
			 client.println(F("POST /api/v1/temperature/heartbeat.json HTTP/1.1"));
			 client.print(F("Host: "));
			 String serverIPstring;
			 //stringIPandPort(serverIP, serverIPstring, port);
			 stringURLandPort(webServerURL, serverIPstring, port);
			 client.println(serverIPstring);
			 client.println(F("Connection: close"));
			 client.println(F("Content-Type: application/json"));
			 client.print(F("Content-Length: "));

			 String json_string="{\"Heartbeat\":\"OK\", \"RF_bitcount\":"+String(RF_reciever::noiseCount)+" }";
			//reset noise count
			 RF_reciever::noiseCount=0;
			 client.println(json_string.length());
			 client.print(F("\r\n"));
			 client.print(json_string); //heartbeat json body

			 //echo json string to serial (if you can't see this then the arduino is out of RAM)
			 debugPrint(F("JSON sent was\n"));
			 debugPrint(json_string);
			 debugPrint(F("\n"));

			 client.flush(); //make sure packet is fully sent and not stuck in ethernet buffer

			 //wait 1 second for the server to respond
			 delay(1000);

			 //server response
			debugPrint(F("Server response is:\n"));
			while(client.available())
				 {
				    String c = String((char)client.read());
				    debugPrint(c);
				  }
			debugPrint(F("\n"));

		}

		client.flush(); //send any data still in the send buffer (should be none)
		delay(100); //wait for data to be flushed
		client.stop(); //end connection as only allowed 4 connections at a time so if you don't end them you run out of RAM
	}


void EthernetWeather::sendMessage()
{

	connectToserver();

	if (client.connected())
	{
		debugPrint(F("Sending temperature POST request\n"));

		//send the message as JSON packet
		 client.println(F("POST /api/v1/temperature/TemperatureHumidity.json HTTP/1.1"));
		 client.print(F("Host: "));
		 String serverIPstring;
		 //stringIPandPort(serverIP, serverIPstring, port);
		 stringURLandPort(webServerURL, serverIPstring, port);
		 client.println(serverIPstring);
		 client.println(F("Connection: close"));
		 client.println(F("Content-Type: application/json"));
		 client.print(F("Content-Length: "));


		 makeJSONstring(); //create the JSON string from the data in the class RF_reciever

		// Serial1.print("\n\nAfter json setup, free ram is: ");
		 //Serial1.println(freeRam());

		 client.println(jsonData.length());
		 client.print(F("\r\n"));
		 client.print(jsonData);

		 //echo to serial
		 debugPrint(F("JSON sent was\n"));
		 debugPrint(jsonData);
		 debugPrint(F("\n"));

		 client.flush(); //flush to make sure full packet is sent
		 //wait 1 second for server response
		 delay(1000);

		 //server response
		debugPrint(F("Server response is:\n"));
		while(client.available())
			 {
			    String c = String((char)client.read());
			    debugPrint(c);
			  }
		debugPrint(F("\n"));

	}

	client.flush(); //flush any remaining data
	delay(100);
	client.stop(); //end connection as only allowed 4 connections at a time so if you don't end them you run out of RAM

}

void EthernetWeather::stringIPandPort(IPAddress serverIP, String &dest,uint16_t portNum)
{
	dest.reserve(21);
	dest=String(serverIP[0]);

	for(int i=1;i<4;i++)
	{
		dest=dest+".";
		dest=dest+String(serverIP[i]);

	}

	if(portNum<=10000)
	{
		dest=dest+":";
		dest=dest+String(portNum);
	}

}

void EthernetWeather::stringURLandPort(const char * url, String &dest,uint16_t portNum)
{
	dest.reserve(38);
	dest=(String)url;

	if(portNum<=10000)
	{
		dest=dest+":";
		dest=dest+String(portNum);
	}

}

void EthernetWeather::makeJSONstring()
{
	jsonData="";
	jsonData="{ \"Preamble\": "+String(RF_reciever::preamble);
	jsonData=jsonData+",\"ID\": " +String(RF_reciever::id);
	jsonData=jsonData+",\"Channel\": " +String(RF_reciever::channel);
	jsonData=jsonData+",\"Battery\": " +String(RF_reciever::batteryOK);
	jsonData=jsonData+",\"TimeMicros\": " +String(RF_reciever::timeMicros);
	jsonData=jsonData+",\"Temperature10\": ";
	if(RF_reciever::negativeTemperature)
	{
		jsonData=jsonData+"-";
	}
	jsonData=jsonData+String(RF_reciever::temperature10);
	jsonData=jsonData+",\"RelativeHumidity\": "+String(RF_reciever::relativeHumidity);
	jsonData=jsonData+" }";


}

void EthernetWeather::debugPrint(String s)
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

void EthernetWeather::debugPrint(const __FlashStringHelper * s)
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


