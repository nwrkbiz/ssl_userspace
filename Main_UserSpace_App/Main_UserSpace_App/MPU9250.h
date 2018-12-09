#pragma once
#include <stdint.h>
#include <array>
#include <string>
#include "Sensor.h"

class MPU9250 : public Sensor
{
public:	
	// To define array that holds X, Y and Z coordinates of values
	typedef std::array<int16_t, 3> ThreeAxis;
	
	// CTOR 
	MPU9250(std::string const SensorName, std::string const CharDevice_Path, size_t const Buffer_Length, KafkaProducer & Producer);
	
	// METHODs
	virtual bool Measure();
	virtual bool SendValues(TS_TYPE Timestamp);
	
	// Sensor specific GETTER methods
	typedef enum { GYRO, ACC, MAGN } eVALUE_TYPE;
	ThreeAxis const * const Get_Values(eVALUE_TYPE Which_Values) const;
private:
	
	// Array that holds X, Y and Z values
	ThreeAxis mGyro;	// [°/s]
	ThreeAxis mAcc;  	// [g]
	ThreeAxis mMagn; 	// [uT]
};



