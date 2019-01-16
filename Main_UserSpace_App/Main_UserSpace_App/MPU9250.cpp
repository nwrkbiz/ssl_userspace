#include <stdio.h>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "MPU9250.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>


/////////////////////////////
//	DEFINES
#define SIG_TEST 44

// how many bytes to read/write to char device
#define READ_SIZE			22
#define WRITE_SIZE			21
#define EVENT_OFFSET		15
#define PID_OFFSET			16
#define PID_SIZE			5
#define TIMESTAMP_OFFSET	18

#define GET_VALUE(x) ((((uint16_t)(*((x)+1))) << 8) | ((uint16_t)(*(x))))

using namespace std;

MPU9250* MPU9250::ptrMe = NULL;

MPU9250::MPU9250(std::string const SensorName, std::string const CharDevice_Path, size_t const Buffer_Length, KafkaProducer & Producer)
	: Sensor(SensorName, CharDevice_Path, Buffer_Length, Producer)
{
	fill(mGyro.begin(), mGyro.end(), 0);
	fill(mAcc.begin(),  mAcc.end(), 0);
	fill(mMagn.begin(), mMagn.end(), 0);
	
	//kernel needs to know our pid to be able to send us a signal
	ostringstream ss;
	ss << setw(5) << setfill('0') << getpid();
	int configfd = open(mCharDevice_Path.c_str(), O_WRONLY);
	string temp = ss.str();
	uint8_t t2[WRITE_SIZE] = { 0 };
	t2[0] = 0x01;
	memcpy((t2 + PID_OFFSET), temp.c_str(), 5);
	write(configfd, t2, WRITE_SIZE);
	close(configfd);
	
	// Set ourself to static member
	MPU9250::ptrMe = this;

	//setup the signal handler for SIG_TEST 
	//SA_SIGINFO -> we want the signal handler function with 3 arguments
	struct sigaction sig;
	sig.sa_sigaction = MeasureIRQ;
	sig.sa_flags = SA_SIGINFO;
	sigaction(SIG_TEST, &sig, NULL);
	
	// reset base0
	base = 0;
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
bool MPU9250::ConfigureTolerance(MPU9250::Tolerances const & tolerances)
{
	if (!FPGA_Ready)
		return true;
	
	static uint8_t to_write[WRITE_SIZE] = { 0 };
	memset(to_write, 0, WRITE_SIZE);
	int tol_fd = open(mCharDevice_Path.c_str(), O_WRONLY);
	
	// let FREQENCY remain the same
	to_write[0] = 0;
	to_write[1] = 0;
	
	// enable tolerances
	to_write[2] = 0xFF;
	
	// write X, Y and Z tolerance values
	for(size_t i = 3; i < WRITE_SIZE; i++)
	{
		to_write[i] = tolerances[i-3];
	}
	
	// DISABLE EVENT MODE
	to_write[EVENT_OFFSET] = '0';
	
	// No change of PID
	
	// Write to CHAR DEVICE
	write(tol_fd, to_write, WRITE_SIZE);
	
	// close CHAR DEVICE
	close(tol_fd);
	
	// ONE TIME EVENT FIFO FLUSH
	Handle_IRQ(false);
}

//this function should be callback of signal
void MPU9250::MeasureIRQ(int n, siginfo_t *info, void * me)
{	
	// Info that we got an IRQ
	std::cout << "IRQ came" << std::endl;
	
	MPU9250::ptrMe->Handle_IRQ(true);
}

bool MPU9250::Measure()
{	
	// Buffer which contains values from sensor
	uint8_t * buf = Get_Buffer();
	
	// (1) Read from CHAR DEVICE --> return NULLPTR if ERROR
	if(!Read_CharDevice())
		return false;
		
	// Calculate Values out of BUFFER
	mAcc[0]  = ((int16_t)GET_VALUE(&buf[0])  / ACCEL_DIVIDER);
	mAcc[1]  = ((int16_t)GET_VALUE(&buf[2])  / ACCEL_DIVIDER);
	mAcc[2]  = ((int16_t)GET_VALUE(&buf[4])  / ACCEL_DIVIDER);
	mGyro[0] = ((int16_t)GET_VALUE(&buf[6])  / GYRO_DIVIDER);
	mGyro[1] = ((int16_t)GET_VALUE(&buf[8])  / GYRO_DIVIDER);
	mGyro[2] = ((int16_t)GET_VALUE(&buf[10]) / GYRO_DIVIDER);
	mMagn[0] = ((int16_t)GET_VALUE(&buf[12]) * 15 / 100);
	mMagn[1] = ((int16_t)GET_VALUE(&buf[14]) * 15 / 100);
	mMagn[2] = ((int16_t)GET_VALUE(&buf[16]) * 15 / 100);
		
	Set_Timestamp(&buf[18]);
	
	return true;
}

bool MPU9250::SendValues(int64_t Timestamp)
{	
	static uint32_t i = 0;
	
	if (i < 150)
	{
		i++;
		return true;
	}
	
	i = 0;
	
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

int64_t MPU9250::GetBase()
{
	return base;
}

void MPU9250::SetBase(int64_t Base)
{
	base = Base;
}

void MPU9250::Handle_IRQ(bool send)
{
	static uint8_t to_write[WRITE_SIZE] = { 0 };
	static uint8_t to_read[READ_SIZE];
	
	InfluxDB::TAG_MAP const tags = { make_pair("Sensor", Get_SensorName()) };
	InfluxDB::FIELD_MAP fields;
	InfluxDB inflx(tags, fields, -1, "EVENT_MODE");
		
	if (!FPGA_Ready)
		return;
		
	// Open CHAR DEVICE
	int fd = open(mCharDevice_Path.c_str(), O_WRONLY);
	if (fd == -1)
	{
		cerr << "Error: opending Char Device!" << endl;
		return;
	}
		
	// (1) Set device driver into EVENT mode (write EVENT byte)
	memset(to_write, 0, WRITE_SIZE);
	to_write[EVENT_OFFSET] = '1';
	write(fd, to_write, WRITE_SIZE);
	close(fd);
		
	// (2) Read out EVENT FIFO and send to cloud
	ReadDevice(to_read);
	TS_TYPE timestamp, old_timestamp = 0;
	memcpy(&timestamp, &to_read[TIMESTAMP_OFFSET], 4);
	
	size_t i = 0;
		
	// As long as we read a new timestamp --> Send to cloud!
	while(timestamp != old_timestamp)
	{
		old_timestamp = timestamp;
			
		if (send)
		{
			// Send all values
			inflx.Set_Timestamp(ptrMe->GetBase() + timestamp);
			fields.clear();
			fields.insert(make_pair("AccX", ((int16_t)GET_VALUE(&to_read[0])  / ACCEL_DIVIDER)));
			fields.insert(make_pair("AccY", ((int16_t)GET_VALUE(&to_read[2])  / ACCEL_DIVIDER)));
			fields.insert(make_pair("AccZ", ((int16_t)GET_VALUE(&to_read[4])  / ACCEL_DIVIDER))); 
	
			if (!mProducer.Send_InfluxDB(&inflx))
				cout << "Error sending to cloud" << endl;
			
			i++;
		}
				
		// New MEASUREMENT
		ReadDevice(to_read);
		memcpy(&timestamp, &to_read[TIMESTAMP_OFFSET], 4);
		
		if (timestamp == old_timestamp)
			break;
	}
	
	cout << i << " values were sent!" << endl;
		
	// (3) Set device driver back into STREAMING mode
	fd = open(mCharDevice_Path.c_str(), O_WRONLY);
	if (fd == -1)
	{
		cerr << "Error: opending Char Device!" << endl;
		return;
	}
	memset(to_write, 0, WRITE_SIZE);
	to_write[EVENT_OFFSET] = '0';
	write(fd, to_write, WRITE_SIZE);
		
	// Close CHAR DEVICE
	close(fd);
}


void MPU9250::ReadDevice(uint8_t * buf)
{
	int fdtemp = open(mCharDevice_Path.c_str(), O_RDONLY);
	if (fdtemp == -1)
	{
		cerr << "Error: opending Char Device!" << endl;
		return;
	}
	ssize_t ret = read(fdtemp, buf, READ_SIZE);
			
	if (ret != READ_SIZE)
	{
		cerr << "Error: reading Char Device!" << endl;
		return;
	}
	close(fdtemp);
}
