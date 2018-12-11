#pragma once
#include "Sensor.h"
#include <stdint.h>

class APDS9301 : public Sensor
{
public:
	// CTOR
	APDS9301(std::string const SensorName, std::string const CharDevice_Path, size_t const Buffer_Length, KafkaProducer & Producer);
	
	// METHODs
	virtual bool Measure();
	virtual bool SendValues(int64_t Timestamp);
	
	// Sensor specific GETTER methods
	uint32_t Get_Brightness() const;
private:
	uint32_t mBrightness;   // [Lux]
};

