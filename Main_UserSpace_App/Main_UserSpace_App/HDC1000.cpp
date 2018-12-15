#include <stdio.h>
#include <string>
#include <cstring>
#include <iostream>
#include "HDC1000.h"

using namespace std;

HDC1000::HDC1000(std::string const SensorName, std::string const CharDevice_Path, size_t const Buffer_Length, KafkaProducer & Producer)
	: Sensor(SensorName, CharDevice_Path, Buffer_Length, Producer)
{
	mHumidity		= 0;
	mTemperature	= 0.0;
	
	// Prepare InfluxDB fields
	Get_Influx_Fields().insert(make_pair("Wert", 0.0f));
}

bool HDC1000::Measure()
{
	// Buffer which contains values from sensor
	uint8_t * buf = Get_Buffer();
	
	// (1) Read from CHAR DEVICE --> return NULLPTR if ERROR
	if (!Read_CharDevice())
		return false;
	
	// (2) Calculate Values out of BUFFER
	uint16_t temp_raw = ((((uint16_t)buf[1]) << 8) | (uint16_t)buf[0]);	
	mTemperature = (((double)temp_raw / (double)(1 << 16)) * 165) - 40;
		
	uint16_t humid_raw = ((((uint16_t)buf[3]) << 8) | (uint16_t)buf[2]);
	mHumidity = (((double)temp_raw / (double)(1 << 16)) * 100);
		
	uint8_t timestamp_buf[] = { buf[4], buf[5], buf[6], buf[7] };
	TS_TYPE ts = 0;
	Set_Timestamp(0);
	memcpy(&ts, timestamp_buf, 4);
	Set_Timestamp(ts);
	
	return true;
}

double HDC1000::Get_Temperature() const
{
	return mTemperature;
}

uint8_t HDC1000::Get_Humidity() const
{
	return mHumidity;
}

bool HDC1000::SendValues(int64_t Timestamp)
{
	// (1) Send TEMPERATURE value
	Get_Influx()->Set_Measurement("Temperatur");
	Get_Influx_Fields()["Wert"] = mTemperature;
	Get_Influx()->Set_Timestamp(Timestamp);
				
	if(!Get_Producer().Send_InfluxDB(Get_Influx()))
		return false;
				
	// (2) Send HUMIDITY value
	Get_Influx()->Set_Measurement("Feuchtigkeit");
	Get_Influx_Fields()["Wert"] = mHumidity;
		
	if (!Get_Producer().Send_InfluxDB(Get_Influx()))
		return false;

	return true;
}
