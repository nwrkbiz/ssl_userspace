#pragma once
#include "Sensor.h"
#include <stdint.h>

class HDC1000 : public Sensor
{
public:
	// CTOR
	HDC1000(std::string const SensorName, std::string const CharDevice_Path, size_t const Buffer_Length, KafkaProducer & Producer);
	
	// METHODs
	virtual bool Measure();
	virtual bool SendValues(int64_t Timestamp);
	
	// Sensor specific GETTER methods
	double  Get_Temperature() const;
	uint8_t Get_Humidity() const;
private:
	double   mTemperature;  // [°C]
	uint8_t  mHumidity;  	// [%]
};

