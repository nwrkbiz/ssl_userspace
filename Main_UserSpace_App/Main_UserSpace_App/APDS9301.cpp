#include <stdio.h>
#include <string>
#include <cstring>
#include <iostream>
#include <math.h>
#include "APDS9301.h"

// To print debug infos to file "output.csv"
#define PRINT_DEBUG

#if defined(PRINT_DEBUG)
#include <fstream>
std::ofstream file("output.csv");
#endif

using namespace std;

uint16_t t1, t2;

APDS9301::APDS9301(std::string const SensorName, std::string const CharDevice_Path, size_t const Buffer_Length, KafkaProducer & Producer)
	: Sensor(SensorName, CharDevice_Path, Buffer_Length, Producer)
{
	mBrightness	= 0;
	
	// Prepare InfluxDB fields
	Get_Influx_Fields().insert(make_pair("Wert", 0.0f));
#if defined(PRINT_DEBUG)
	file << "CH0 dec; CH0 hex;CH1 dec;CH1 hex;brightness dec;brightness hex;timestamp" << endl;
#endif
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
	uint16_t temp_CH1 = ((((uint16_t)buf[3]) << 8) | (uint16_t)buf[2]);
	
	t1 = temp_CH0;
	t2 = temp_CH1;
	
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
	
	Set_Timestamp(&buf[4]);
	
	return true;
}

uint32_t APDS9301::Get_Brightness() const
{
	return mBrightness;
}

bool APDS9301::SendValues(int64_t Timestamp)
{
//	static uint32_t i = 0;
//	
//	if (i < 10)
//	{
//		i++;
//		return true;
//	}
//	
//	i = 0;
	
	// Send BRIGTHNESS value
	Get_Influx()->Set_Measurement("Helligkeit");
	Get_Influx_Fields()["Wert"] = mBrightness;
	Get_Influx()->Set_Timestamp(Timestamp);
				
	if (!Get_Producer().Send_InfluxDB(Get_Influx()))
			return false;
	
#if defined(PRINT_DEBUG)
	file << dec << t1 << ";0x" << hex << t1 << ";"
		<< dec << t2 << ";0x" << hex << t2 << ";"
		<< dec << mBrightness << ";0x" << hex << mBrightness << dec << ";" << Get_Timestamp() << endl;
#endif
	
	return true;
}
