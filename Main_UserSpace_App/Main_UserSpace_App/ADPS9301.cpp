#include <stdio.h>
#include <string>
#include <cstring>
#include <iostream>
#include <math.h>
#include "ADPS9301.h"

using namespace std;

ADPS9301::ADPS9301(std::string const SensorName, std::string const CharDevice_Path, size_t const Buffer_Length, KafkaProducer & Producer)
	: Sensor(SensorName, CharDevice_Path, Buffer_Length, Producer)
{
	mBrightness	= 0;
	
	// Prepare InfluxDB fields
	Get_Influx_Fields().insert(make_pair("Wert", 0.0f));
}

bool ADPS9301::Measure()
{
	// Buffer which contains values from sensor
	uint8_t * buf = Get_Buffer();
	
	// (1) Read from CHAR DEVICE --> return NULLPTR if ERROR
	if(!Read_CharDevice())
		return false;
	
	// Calculate Values out of BUFFER
	uint16_t temp_CH0 = ((((uint16_t)buf[1]) << 8) | (uint16_t)buf[0]);	
	uint16_t temp_CH1 = ((((uint16_t)buf[3]) << 8) | (uint16_t)buf[2]);
	
	double temp_Division = ((double)temp_CH1) / ((double)temp_CH0);
	
	if ((temp_Division > 0) && (temp_Division <= 0.5))
	{
		mBrightness = ((0.0304 * temp_CH0) - (0.062 * temp_CH0 * pow(temp_Division, 1.4)));
	}
	else if ((temp_Division > 0.5) && (temp_Division <= 0.61))
	{
		mBrightness = ((0.0224 * temp_CH0) - (0.031 * temp_CH1));
	}
	else if ((temp_Division > 0.61) && (temp_Division <= 0.8))
	{
		mBrightness = ((0.0128 * temp_CH0) - (0.0153 * temp_CH1));
	}
	else if ((temp_Division > 0.8) && (temp_Division <= 1.3))
	{
		mBrightness = ((0.00146  * temp_CH0) - (0.00112 * temp_CH1));
	}
	else
		mBrightness = 0;
		
	uint8_t timestamp_buf[] = { buf[7], buf[6], buf[5], buf[4] };
	TS_TYPE ts = 0;
	Set_Timestamp(0);
	memcpy(&ts, timestamp_buf, 4);
	Set_Timestamp(ts);
	
	return true;
}

uint16_t ADPS9301::Get_Brightness() const
{
	return mBrightness;
}

bool ADPS9301::SendValues(TS_TYPE Timestamp)
{
	// Send BRIGTHNESS value
	Get_Influx()->Set_Measurement("Helligkeit");
	Get_InfluxTags()["Wert"] = mBrightness;
	Get_Influx()->Set_Timestamp(Timestamp);
				
	if (!Get_Producer().Send_InfluxDB(Get_Influx()))
		return false;
}
