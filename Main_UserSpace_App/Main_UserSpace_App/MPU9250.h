#pragma once
#include <stdint.h>
#include <array>
#include <string>
#include <signal.h>
#include "Sensor.h"

class MPU9250 : public Sensor
{
public:	
	// To define array that holds X, Y and Z coordinates of values
	typedef std::array<float, 3> ThreeAxis;
	
	// CTOR 
	MPU9250(std::string const SensorName, std::string const CharDevice_Path, size_t const Buffer_Length, KafkaProducer & Producer);
	
	// METHODs
	virtual bool Measure();
	virtual bool SendValues(int64_t Timestamp);
	static void MeasureIRQ(int n, siginfo_t *info, void *unused);
	// Configure Tolerance Register for EVENT MODE
	// PARAM: Tolerances[0]: tolerance value for X-AXIS
	// PARAM: Tolerances[1]: tolerance value for Y-AXIS
	// PARAM: Tolerances[2]: tolerance value for Z-AXIS
	bool ConfigureTolerance(ThreeAxis const & Tolerances);
	
	// for FPGA HW --> to avoid invalid access
	void SetFPGAReady();
	void SetFPGANOTReady();
	
	// Sensor specific GETTER methods
	typedef enum { GYRO, ACC, MAGN } eVALUE_TYPE;
	ThreeAxis const * const Get_Values(eVALUE_TYPE Which_Values) const;
private:
	
	// Array that holds X, Y and Z values
	ThreeAxis mGyro;	// [°/s]
	ThreeAxis mAcc;  	// [g]
	ThreeAxis mMagn; 	// [uT]
	
	// To be able to check if FPGA is ready
	bool FPGA_Ready;
};



