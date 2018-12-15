#include <stdio.h>
#include <string>
#include <cstring>
#include <iostream>
#include "MPU9250.h"

using namespace std;

MPU9250::MPU9250(std::string const SensorName, std::string const CharDevice_Path, size_t const Buffer_Length, KafkaProducer & Producer)
	: Sensor(SensorName, CharDevice_Path, Buffer_Length, Producer)
{
	fill(mGyro.begin(), mGyro.end(), 0);
	fill(mAcc.begin(),  mAcc.end(), 0);
	fill(mMagn.begin(), mMagn.end(), 0);
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

bool MPU9250::Measure()
{
	// Buffer which contains values from sensor
	uint8_t * buf = Get_Buffer();
	
	// (1) Read from CHAR DEVICE --> return NULLPTR if ERROR
	if(!Read_CharDevice())
		return false;
	
	// Calculate Values out of BUFFER
//	mAcc[0]  = ((((uint16_t)buf[0]) << 8)  | (uint16_t)buf[1]);
//	mAcc[1]  = ((((uint16_t)buf[2]) << 8)  | (uint16_t)buf[3]);
//	mAcc[2]  = ((((uint16_t)buf[4]) << 8)  | (uint16_t)buf[5]);
//	mGyro[0] = ((((uint16_t)buf[6]) << 8)  | (uint16_t)buf[7]);
//	mGyro[1] = ((((uint16_t)buf[8]) << 8)  | (uint16_t)buf[9]);
//	mGyro[2] = ((((uint16_t)buf[10]) << 8) | (uint16_t)buf[11]);
//	mMagn[0] = ((((uint16_t)buf[12]) << 8) | (uint16_t)buf[13]);
//	mMagn[1] = ((((uint16_t)buf[14]) << 8) | (uint16_t)buf[15]);
//	mMagn[2] = ((((uint16_t)buf[16]) << 8) | (uint16_t)buf[17]);
	
	mAcc[0]  = ((((uint16_t)buf[1]) << 8)  | (uint16_t)buf[0]);
	mAcc[1]  = ((((uint16_t)buf[3]) << 8)  | (uint16_t)buf[2]);
	mAcc[2]  = ((((uint16_t)buf[5]) << 8)  | (uint16_t)buf[4]);
	mGyro[0] = ((((uint16_t)buf[7]) << 8)  | (uint16_t)buf[6]);
	mGyro[1] = ((((uint16_t)buf[9]) << 8)  | (uint16_t)buf[8]);
	mGyro[2] = ((((uint16_t)buf[11]) << 8) | (uint16_t)buf[10]);
	mMagn[0] = ((((uint16_t)buf[13]) << 8) | (uint16_t)buf[12]);
	mMagn[1] = ((((uint16_t)buf[15]) << 8) | (uint16_t)buf[14]);
	mMagn[2] = ((((uint16_t)buf[17]) << 8) | (uint16_t)buf[16]);
		
	uint8_t timestamp_buf[] = { buf[18], buf[19], buf[20], buf[21] };
	TS_TYPE ts = 0;
	Set_Timestamp(0);
	memcpy(&ts, timestamp_buf, 4);
	Set_Timestamp(ts);
	
	return true;
}

bool MPU9250::SendValues(int64_t Timestamp)
{
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
