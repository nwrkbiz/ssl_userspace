/*
 * This Class is using the Kafka driver from librdkafka
 * (https://github.com/edenhill/librdkafka)
 */

#include <stdio.h>
#include <signal.h>
#include <sstream>
#include "KafkaProducer.h"

using namespace std;

// PROTOTYPES for KAFKA Functions
static void stop(int sig);
static void dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage, void *opaque);

////////////////////////////////////////////////
// CTORs
////////////////////////////////////////////////
KafkaProducer::KafkaProducer(std::string const &server, uint16_t const port /* = 9092 */) : mProducer{nullptr}, mTopic{nullptr}
{
	ostringstream strs;

	// Create new KAFKA Procuder (host:port)
	strs.clear();
	strs << server << ":" << port;
	if (!CreateProducer(strs.str()))
	{
		throw ("ERROR: creating Kafka Producer!");
	}
	
	/* Signal handler for clean shutdown */
	signal(SIGINT, stop);
}

KafkaProducer::KafkaProducer(std::string const &server, std::string const &topic, uint16_t const port /* = 9092 */) : mProducer{nullptr}, mTopic{nullptr}
{
	char errstr[512];		/* librdkafka API error reporting buffer */
	ostringstream strs;

	// Create new KAFKA Procuder (host:port)
	strs.clear();
	strs << server << ":" << port;
	if (!CreateProducer(strs.str()))
	{
		throw ("ERROR: creating Kafka Producer!");
	}

	// Create topic object
	if(!CreateTopic(topic))
	{
		throw ("ERROR: creating Kafka Topic: " + topic + "!");
	}

	/* Signal handler for clean shutdown */
	signal(SIGINT, stop);
}

////////////////////////////////////////////////
// DTOR
////////////////////////////////////////////////
KafkaProducer::~KafkaProducer()
{
	// if TOPIC was created --> DESTROY
	if(mTopic != nullptr)
	{
		rd_kafka_topic_destroy(mTopic);
		mTopic = nullptr;
	}
	
	// if PRODUCER was created --> DESTROY
	if(mProducer != nullptr)
	{
		rd_kafka_destroy(mProducer);
		mProducer = nullptr;
	}
}

////////////////////////////////////////////////
// METHODs
////////////////////////////////////////////////
bool KafkaProducer::CreateProducer(string const &broker)
{
	rd_kafka_conf_t *conf;	// Temporary configuration object
	char errstr[512];		// librdkafka API error reporting buffer

	// Create Kafka client configuration place-holder
	conf = rd_kafka_conf_new();

	// Set the CONFIG for the Producer
	if (rd_kafka_conf_set(conf, "bootstrap.servers", broker.c_str(), errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		fprintf(stderr, "%s\n", errstr);
		return false;
	}

	// Set the delivery report callback.
	rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);

	// Create producer instance.
	mProducer = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
	if (mProducer == nullptr) {
		fprintf(stderr, "%% Failed to create new producer: %s\n", errstr);
		return false;
	}
	
	// IF SUCCESS --> return true
	return true;
}

bool KafkaProducer::CreateTopic(std::string const &topic)
{
	// Create topic object
	mTopic = rd_kafka_topic_new(mProducer, topic.c_str(), NULL);
	if (mTopic == nullptr) {
		fprintf(stderr, "%% Failed to create topic object: %s\n", rd_kafka_err2str(rd_kafka_last_error()));
		return false;
	}
	
	return true;
}

bool KafkaProducer::Send_Raw(std::string const &message, std::string const &topic)
{
	// Check if TOPIC was already created!
	if(mTopic == nullptr)
	{
		if (!CreateTopic(topic))
			return false;	// return ERROR
	}
	
	// Send/Produce message.
retry:
	if (rd_kafka_produce(
			/* Topic object */
			mTopic,
			/* Use builtin partitioner to select partition*/
			RD_KAFKA_PARTITION_UA,
			/* Make a copy of the payload. */
			RD_KAFKA_MSG_F_COPY,
			/* Message payload (value) and length */
			(void*)message.c_str(),
			message.length(),
			/* Optional key and its length */
			NULL,
			0,
			/* Message opaque, provided in
			* delivery report callback as
			* msg_opaque. */
			NULL) == -1)
		{
		// Failed to *enqueue* message for producing.
		fprintf(stderr, "%% Failed to produce to topic %s: %s\n", rd_kafka_topic_name(mTopic), rd_kafka_err2str(rd_kafka_last_error()));

		// if QUEUE is FULL --> try again
		if (rd_kafka_last_error() == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
			rd_kafka_poll(mProducer, 1000/*block for max 1000ms*/);
			goto retry;
		}
		
		// OTHER ERROR
		return false;
	}

	// Wait for messages to be sent
	rd_kafka_poll(mProducer, 0/*non-blocking*/);
	rd_kafka_flush(mProducer, 10 * 1000 /* wait for max 10 seconds */);

	return true;	// return SUCCESS	
}

bool KafkaProducer::Send_InfluxDB(InfluxDB const &influx_params, std::string const &topic)
{	
	string message = influx_params.Get_Measurement() + ",";
	
	// Put all TAGs into message
	for(auto const & x : influx_params.Get_TagSet())
	{
		message += x.first + "=" + x.second + ",";
	}
	message.pop_back();	// remove last ","
	message += " ";
	
	// Put all FIELDs into message
	for(auto const & x : influx_params.Get_FieldSet())
	{
		message += x.first + "=" + to_string(x.second) + ",";
	}
	message.pop_back(); 	// remove last ","
	message += " ";
	
	// Put TIMESTAMP into message if THERE is one
	if(influx_params.Get_Timestamp() != -1)
	{
		message += to_string(influx_params.Get_Timestamp());
	} else
	{
		message.pop_back();  	// remove last space
	}
	
	// SEND MESSAGE
	return Send_Raw(message, topic);
}


////////////////////////////////////////////////
// KAFKA STUFF
////////////////////////////////////////////////

 // Signal termination of program
static void stop(int sig) {
	fclose(stdin); /* abort fgets() */
}

// Message delivery report callback.
static void dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage, void *opaque) {
	if (rkmessage->err)
	{
		fprintf(stderr, "%% Message delivery failed: %s\n", rd_kafka_err2str(rkmessage->err));
	}
	else
	{
		fprintf(stderr, "%% Message delivered (%zd bytes, partition %"PRId32")\n", rkmessage->len, rkmessage->partition);
	}
}