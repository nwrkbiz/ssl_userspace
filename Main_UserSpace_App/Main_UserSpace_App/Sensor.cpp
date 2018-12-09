// for READ from chardevice
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <cstring>
#include <stdio.h>

#include "Sensor.h"

using namespace std;

Sensor::Sensor(string const & SensorName, string const & CharDevice_Path, size_t Buffer_Length, KafkaProducer & Producer)
	: 
	mSensorName(SensorName), mCharDevice_Path(CharDevice_Path), mBuffer_Length(Buffer_Length), mProducer(Producer)
{
	// (1) Allocate Buffer for reading CHAR DEVICE
	mRead_Buffer = new uint8_t[Buffer_Length];
	
	// (2) Check if SUCCESS
	if(mRead_Buffer == nullptr)
		throw("Error: allocating memory for Buffer!");
	
	// (3) Set BUFFER to 0
	memset(mRead_Buffer, 0, Buffer_Length);
	
	// (4) Reset TIMESTAMP
	mTimestamp = 0;
	
	// (5) Reset Tags and Fields
	mInflux_Fields.clear();
	mInflux_Tags.clear();
	mInflux_Tags.insert(make_pair("Sensor", SensorName));
	
	// (6) Allocate InfluxDB class
	mInflux = new InfluxDB(mInflux_Tags, mInflux_Fields);
	if (mInflux == nullptr)
		throw ("ERROR: allocating memory for Influx Class!");
}

Sensor::~Sensor()
{
	if (mRead_Buffer != nullptr)
		delete[] mRead_Buffer;
	
	mRead_Buffer = nullptr;
	
	if (mInflux != nullptr)
	{
		delete mInflux; mInflux = nullptr;
	}
}

bool Sensor::Read_CharDevice()
{
	int input_fd = open(mCharDevice_Path.c_str(), O_RDONLY);
	if (input_fd == -1)
	{
		cerr << "Error: opending Char Device!" << endl;
		return false;
	}
		
	ssize_t ret = read(input_fd, mRead_Buffer, mBuffer_Length);
		
	if ((ret == -1) || (ret == 0))
	{
		cerr << "Error: reading Char Device!" << endl;
		return false;
	}
	close(input_fd);
	
	return true;
}

uint32_t Sensor::Get_Timestamp() const
{
	return mTimestamp;
}

std::string const & Sensor::Get_SensorName() const
{
	return mSensorName;
}

uint8_t * Sensor::Get_Buffer() const
{
	return mRead_Buffer;
}

InfluxDB * Sensor::Get_Influx()
{
	return mInflux;
}

InfluxDB::TAG_MAP &	Sensor::Get_InfluxTags()
{
	return mInflux_Tags;
}

InfluxDB::FIELD_MAP & Sensor::Get_Influx_Fields()
{
	return mInflux_Fields;
}

void Sensor::Set_Timestamp(TS_TYPE Timestamp)
{
	mTimestamp = Timestamp;
}

KafkaProducer & Sensor::Get_Producer() const
{
	return mProducer;
}
