#include <stdio.h>
#include <string>
#include <cstring>
#include <iostream>
#include <math.h>
#include "APDS9301.h"

using namespace std;

APDS9301::APDS9301(std::string const SensorName, std::string const CharDevice_Path, size_t const Buffer_Length, KafkaProducer & Producer)
	: Sensor(SensorName, CharDevice_Path, Buffer_Length, Producer)
{
	mBrightness	= 0;
	
	// Prepare InfluxDB fields
	Get_Influx_Fields().insert(make_pair("Wert", 0.0f));
}

bool APDS9301::Measure()
{
	// Buffer which contains values from sensor
	uint8_t * buf = Get_Buffer();
	
	// (1) Read from CHAR DEVICE --> return NULLPTR if ERROR
	if(!Read_CharDevice())
		return false;
	
	// Calculate Values out of BUFFER
	uint16_t temp_CH0 = ((((uint16_t)buf[0]) << 8) | (uint16_t)buf[1]);	
	uint16_t temp_CH1 = ((((uint16_t)buf[2]) << 8) | (uint16_t)buf[3]);
	
	float temp = 0.0f;
	float temp_Division = ((float)temp_CH1) / ((float)temp_CH0);
	
	if ((temp_Division > 0) && (temp_Division <= 0.5))
		temp = ((0.0304 * (float)temp_CH0) - (0.062 * (float)temp_CH0 * pow(temp_Division, 1.4)));
	else if ((temp_Division > 0.5) && (temp_Division <= 0.61))
		temp = ((0.0224 * (float)temp_CH0) - (0.031 * (float)temp_CH1));
	else if ((temp_Division > 0.61) && (temp_Division <= 0.8))
		temp = ((0.0128 * (float)temp_CH0) - (0.0153 * (float)temp_CH1));
	else if ((temp_Division > 0.8) && (temp_Division <= 1.3))
		temp = ((0.00146  * (float)temp_CH0) - (0.00112 * (float)temp_CH1));
	else
		temp = 0;
	
	mBrightness = temp;
		
	uint8_t timestamp_buf[] = { buf[4], buf[5], buf[6], buf[7] };
	TS_TYPE ts = 0;
	Set_Timestamp(0);
	memcpy(&ts, timestamp_buf, 4);
	Set_Timestamp(ts);
	
	return true;
}

uint32_t APDS9301::Get_Brightness() const
{
	return mBrightness;
}

bool APDS9301::SendValues(int64_t Timestamp)
{
	// Send BRIGTHNESS value
	Get_Influx()->Set_Measurement("Helligkeit");
	Get_Influx_Fields()["Wert"] = mBrightness;
	Get_Influx()->Set_Timestamp(Timestamp);
				
	if (!Get_Producer().Send_InfluxDB(Get_Influx()))
			return false;
	
	return true;
}
