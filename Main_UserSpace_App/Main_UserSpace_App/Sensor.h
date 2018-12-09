#pragma once

#include <string>
#include <stdint.h>
#include "KafkaProducer.h"
#include "InfluxDB.h"

class Sensor
{
public:	
	//TYPEDEFs
	typedef uint32_t TS_TYPE;
	
	// CTOR and DTRO
	Sensor(std::string const & SensorName, std::string const & CharDevice_Path, size_t Buffer_Length, KafkaProducer & Producer);
	virtual ~Sensor();
	
	// METHODs
	// GETTER methods
	TS_TYPE				Get_Timestamp() const;
	std::string const & Get_SensorName() const;
	// Every Sensor (specific sensor class) MUST implement
	// all of these methods!
	virtual bool		Measure()						= 0;
	virtual bool		SendValues(TS_TYPE Timestamp)	= 0;
	
private:
	
	// Internal Members ONLY used inside the class
	std::string const    mCharDevice_Path;
	     size_t const    mBuffer_Length;
		uint8_t		  *  mRead_Buffer;
	    TS_TYPE			 mTimestamp;
	std::string const    mSensorName;
	// Tagset and Fieldset for Influx Class
	InfluxDB::TAG_MAP	 mInflux_Tags;
	InfluxDB::FIELD_MAP  mInflux_Fields;
	// Influx Class for sending data via Line Protocol
	InfluxDB		  *  mInflux;
	// Kafka Producer for sending via Network
	KafkaProducer	   & mProducer;
	
protected:
	// These methods MUST be 'protected' to be callable
	// from the specific (derived) sensor class,
	// and therefore and NOT callable from outside!
	bool					Read_CharDevice();
	uint8_t	*				Get_Buffer() const;
	InfluxDB::TAG_MAP &		Get_InfluxTags();
	InfluxDB::FIELD_MAP &	Get_Influx_Fields();
	InfluxDB *				Get_Influx();
	KafkaProducer		&	Get_Producer() const;
	void					Set_Timestamp(TS_TYPE Timestamp);
};

