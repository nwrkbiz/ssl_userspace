#pragma once
#include "Sensor.h"
#include <stdint.h>

class ADPS9301 : public Sensor
{
public:
	// CTOR
	ADPS9301(std::string const SensorName, std::string const CharDevice_Path, size_t const Buffer_Length, KafkaProducer & Producer);
	
	// METHODs
	virtual bool Measure();
	virtual bool SendValues(TS_TYPE Timestamp);
	
	// Sensor specific GETTER methods
	uint16_t Get_Brightness() const;
private:
	uint16_t mBrightness;
};

