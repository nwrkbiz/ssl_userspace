#include "InfluxDB.h"

using namespace std;

// CTOR
InfluxDB::InfluxDB(TAG_MAP const &tag_set, FIELD_MAP const &field_set, int32_t timestamp, std::string const &Measurement) :
	mMeasurement {Measurement}, mTag_set {tag_set}, mField_set {field_set}, mTimestamp {timestamp}
{}

// METHODs
string const InfluxDB::Get_RawString() const
{
	string message = mMeasurement + ",";
	
	// Put all TAGs into message
	for(auto const & x : mTag_set)
	{
		message += x.first + "=" + x.second + ",";
	}
	message.pop_back(); 	// remove last ","
	message += " ";
	
	// Put all FIELDs into message
	for(auto const & x : mField_set)
	{
		message += x.first + "=" + to_string(x.second) + ",";
	}
	message.pop_back();  	// remove last ","
	message += " ";
	
	// Put TIMESTAMP into message if THERE is one
	if(mTimestamp != -1)
	{
		message += to_string(mTimestamp*1000000);	// to get ns out of ms
	} else
	{
		message.pop_back();   	// remove last space
	}
	
	return message;
}


void InfluxDB::Set_Measurement(std::string const &Measurement)
{
	mMeasurement = Measurement;
}


void InfluxDB::Set_Timestamp(int64_t Timestamp)
{
	mTimestamp = Timestamp;
}
