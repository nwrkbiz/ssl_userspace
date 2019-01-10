#include <stdio.h>
#include <string>
#include <cstring>
#include <iostream>
#include "MPU9250.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define SIG_TEST 44

using namespace std;

MPU9250::MPU9250(std::string const SensorName, std::string const CharDevice_Path, size_t const Buffer_Length, KafkaProducer & Producer)
	: Sensor(SensorName, CharDevice_Path, Buffer_Length, Producer)
{
	fill(mGyro.begin(), mGyro.end(), 0);
	fill(mAcc.begin(),  mAcc.end(), 0);
	fill(mMagn.begin(), mMagn.end(), 0);
	
	FPGA_Ready = false;
	
	//kernel needs to know our pid to be able to send us a signal
	char buf[mBuffer_Length];
	int configfd = open(mCharDevice_Path.c_str(), O_WRONLY);
	sprintf(buf, "%i", getpid());
	write(configfd, buf, strlen(buf) + 1) ;
	close(configfd);

	 //setup the signal handler for SIG_TEST 
 	 //SA_SIGINFO -> we want the signal handler function with 3 arguments
	struct sigaction sig;
	sig.sa_sigaction = MeasureIRQ;
	sig.sa_flags = SA_SIGINFO;
	sigaction(SIG_TEST, &sig, NULL);
}

MPU9250::ThreeAxis const * const MPU9250::Get_Values(eVALUE_TYPE Which_Values) const
{
	switch (Which_Values)
	{
	case MPU9250::ACC:
		return &mAcc;
	case MPU9250::GYRO:
		return &mGyro;
	case MPU9250::MAGN:
		return &mMagn;
	}
}


// Configure Tolerance Register for EVENT MODE
// PARAM: Tolerances[0]: tolerance value for X-AXIS
// PARAM: Tolerances[1]: tolerance value for Y-AXIS
// PARAM: Tolerances[2]: tolerance value for Z-AXIS
bool MPU9250::ConfigureTolerance(ThreeAxis const & Tolerances)
{
	if (!FPGA_Ready)
		return true;
	
	// write X, Y and Z tolerance values
	for (auto const & tol : Tolerances)
	{
		// write tol to char device
	}
}

//this function should be callback of signal
void MPU9250::MeasureIRQ(int n, siginfo_t *info, void *unused)
{
	// 1) Read out EVENT FIFO
	// 2) Send to InfluxDB
	
	std::cout << "IRQ came" << std::endl;
}

bool MPU9250::Measure()
{
	// Values needed for calculating correct Gyro/Acc/Magn Values
	static const float cCorr_Gyro	= 1000.0			/ 32768.0;
	static const float cCorr_Acc	= 8.0				/ 32768.0;
	static const float cCorr_Magn	= (10.0 * 4219.0)	/ 32760.0; 
	
	// Buffer which contains values from sensor
	uint8_t * buf = Get_Buffer();
	
	// (1) Read from CHAR DEVICE --> return NULLPTR if ERROR
	if(!Read_CharDevice())
		return false;
	
	// Calculate Values out of BUFFER
	mAcc[0]  = ((float)(((((uint16_t)buf[1]) << 8)  | (uint16_t)buf[0]) ) * cCorr_Acc);
	mAcc[1]  = ((float)(((((uint16_t)buf[3]) << 8)  | (uint16_t)buf[2]) ) * cCorr_Acc);
	mAcc[2]  = ((float)(((((uint16_t)buf[5]) << 8)  | (uint16_t)buf[4]) ) * cCorr_Acc);
	mGyro[0] = ((float)(((((uint16_t)buf[7]) << 8)  | (uint16_t)buf[6]) ) * cCorr_Gyro);
	mGyro[1] = ((float)(((((uint16_t)buf[9]) << 8)  | (uint16_t)buf[8]) ) * cCorr_Gyro);
	mGyro[2] = ((float)(((((uint16_t)buf[11]) << 8) | (uint16_t)buf[10])) * cCorr_Gyro);
	mMagn[0] = ((float)(((((uint16_t)buf[13]) << 8) | (uint16_t)buf[12])) * cCorr_Magn);
	mMagn[1] = ((float)(((((uint16_t)buf[15]) << 8) | (uint16_t)buf[14])) * cCorr_Magn);
	mMagn[2] = ((float)(((((uint16_t)buf[17]) << 8) | (uint16_t)buf[16])) * cCorr_Magn);
		
	Set_Timestamp(&buf[18]);
	
	return true;
}

bool MPU9250::SendValues(int64_t Timestamp)
{
//	static uint32_t i = 0;
//	
//	if (i < 200)
//	{
//		i++;
//		return true;
//	}
//	
//	i = 0;
	
	// BEFORE: Set Timestamp for this measurement
	Get_Influx()->Set_Timestamp(Timestamp);
	
	// (1) Send GYROSCOPE values
	Get_Influx()->Set_Measurement("Gyroscope");
	Get_Influx_Fields().clear();
	Get_Influx_Fields().insert(make_pair("GyroX", mGyro[0]));
	Get_Influx_Fields().insert(make_pair("GyroY", mGyro[1]));
	Get_Influx_Fields().insert(make_pair("GyroZ", mGyro[2]));
	
	// Send via CLASS METHOD
	if(!Get_Producer().Send_InfluxDB(Get_Influx()))
		return false;
	
	// (2) Send ACCELEROMETER values
	Get_Influx()->Set_Measurement("Accelerometer");
	Get_Influx_Fields().clear();
	Get_Influx_Fields().insert(make_pair("AccX", mAcc[0]));
	Get_Influx_Fields().insert(make_pair("AccY", mAcc[1]));
	Get_Influx_Fields().insert(make_pair("AccZ", mAcc[2]));
	
	// Send via CLASS METHOD
	if(!Get_Producer().Send_InfluxDB(Get_Influx()))
		return false;
	
	// (1) Send MAGNETOMETER values
	Get_Influx()->Set_Measurement("Magnetometer");
	Get_Influx_Fields().clear();
	Get_Influx_Fields().insert(make_pair("MagnX", mMagn[0]));
	Get_Influx_Fields().insert(make_pair("MagnY", mMagn[1]));
	Get_Influx_Fields().insert(make_pair("MagnZ", mMagn[2]));
	
	// Send via CLASS METHOD
	if(!Get_Producer().Send_InfluxDB(Get_Influx()))
		return false;
	
	return true;
}

void MPU9250::SetFPGAReady()
{
	FPGA_Ready = true;
}

void MPU9250::SetFPGANOTReady()
{
	FPGA_Ready = false;
}
