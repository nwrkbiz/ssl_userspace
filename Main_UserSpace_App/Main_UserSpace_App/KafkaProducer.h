#pragma once

#include <string.h>
#include <stdint.h>
#include <librdkafka/rdkafka.h>
#include "InfluxDB.h"

class KafkaProducer
{
public:
	// TYPEDEFs
	typedef rd_kafka_t			Producer;
	typedef rd_kafka_topic_t	Topic;
	
	// CTOR
	KafkaProducer(std::string const &server, std::string const & topic = "", uint16_t const port = 9092);
	
	// DTOR
	~KafkaProducer();
	
	// Method to Send RAW MESSAGE via Producer
	bool Send_Raw(std::string const &message, std::string const &topic = "");
	
	// Method to Send Inlfux-Protocol MESSAGE via Producer
	bool Send_InfluxDB(InfluxDB const * const influx_params, std::string const &topic = "");
private:
	// MEMBERs
	bool		CreateProducer(std::string const &broker);
	bool		CreateTopic(std::string const &topic);
	Producer *	mProducer;
	Topic	 *	mTopic;
};
