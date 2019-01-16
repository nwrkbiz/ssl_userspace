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
	
	// To define array that holds tolerance values for EVENT MODE
	typedef std::array<uint8_t, 12> Tolerances;
	
	// Values for calculating real ACC and GYRO values out of sensor data
	static constexpr float GYRO_DIVIDER = 16.4f;
	static constexpr float ACCEL_DIVIDER = 4096.0f * 2.0f;  	// * 2 for manual value correction
	
	// CTOR 
	MPU9250(std::string const SensorName, std::string const CharDevice_Path, size_t const Buffer_Length, KafkaProducer & Producer);
	
	// METHODs
	virtual bool Measure();
	virtual bool SendValues(int64_t Timestamp);
	static void MeasureIRQ(int n, siginfo_t *info, void * me);
	// Configure Tolerance Register for EVENT MODE
	// PARAM: uint8_t array with 12 values, containing
	// tolerance for X axis (top and bottom)
	// + Y and Z --> for every value: low and high byte
	// X_TOP_L, X_TOP_H, X_BOT_L, X_BOT_H, Y and then Z
	bool ConfigureTolerance(Tolerances const & tolerances);
	
	// for FPGA HW --> to avoid invalid access
	void SetFPGAReady();
	void SetFPGANOTReady();
	
	// To Set the base and get it
	int64_t GetBase();
	void SetBase(int64_t Base);
	
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
	
	// BASE timestamp for TS calculation
	int64_t base;
	
	// Needed for static IRQ function
	static MPU9250 * ptrMe;
	
	void Handle_IRQ(bool send);
	void ReadDevice(uint8_t * buf);
};



