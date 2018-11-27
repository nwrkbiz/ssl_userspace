#include "InfluxDB.h"

using namespace std;

// CTOR
InfluxDB::InfluxDB(std::string const &Measurement, TAG_MAP const &tag_set, FIELD_MAP const &field_set, int64_t timestamp) :
	mMeasurement {Measurement}, mTag_set {tag_set}, mField_set {field_set}, mTimestamp {timestamp}
{}

// METHODs
int64_t const InfluxDB::Get_Timestamp() const
{
	return mTimestamp;
}

std::string const & InfluxDB::Get_Measurement()	const
{
	return mMeasurement;
}

InfluxDB::TAG_MAP const & InfluxDB::Get_TagSet() const
{
	return mTag_set;
}

InfluxDB::FIELD_MAP	const & InfluxDB::Get_FieldSet() const
{
	return mField_set;	
}
