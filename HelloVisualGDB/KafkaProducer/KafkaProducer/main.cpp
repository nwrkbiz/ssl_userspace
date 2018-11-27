#include <iostream>
#include <string>
#include <stdint.h>
#include "KafkaProducer.h"

using namespace std;

////////////////////////////////////////////////
// CONFIGURATION for ESD/SSL
////////////////////////////////////////////////
string		const HOSTNAME	= "193.170.192.210";
uint16_t	const PORT		= 9092;
string		const TOPIC		= "telegraf";

int main()
{
	try
	{
		// (1) CREATE Kafka Producer
		KafkaProducer producer(HOSTNAME, TOPIC, PORT);
		
		// (2) CREATE InfluxDB compliant CLASS (for InfluxDB Line Protocol)
		InfluxDB::TAG_MAP	tags	= { make_pair("sensor"	, "mycyclone5") };
		InfluxDB::FIELD_MAP fields	= { make_pair("wert"	, 23.6)			};
		InfluxDB inflx("temperatur", tags, fields, -1);
		
		// (3) Send via CLASS METHOD
		if(!producer.Send_InfluxDB(inflx))
		{
			cout << "ERROR while sending message!" << endl;
		}
	}
	catch (string const & exept)
	{
		cout << "EXCEPTION: " << exept << endl;
	}	
	
	return 0;
}