#pragma once

#include <string.h>
#include <map>
#include <stdint.h>

class InfluxDB
{
public:
	//TYPEDEFs
	typedef std::map<std::string, std::string>	TAG_MAP;
	typedef std::map<std::string, float>		FIELD_MAP;
	
	// CTOR
	InfluxDB(std::string const &Measurement, TAG_MAP const &tag_set, FIELD_MAP const &field_set, int64_t timestamp);
	
	// METHODs
	std::string const & Get_Measurement()	const;
	TAG_MAP		const & Get_TagSet()		const;
	FIELD_MAP	const & Get_FieldSet()		const;
	int64_t		const	Get_Timestamp()		const;
private:	
	// MEMBER
	const std::string	mMeasurement;
	const TAG_MAP		mTag_set;
	const FIELD_MAP		mField_set;
	const int64_t		mTimestamp; 	// set to -1 if default should be used
};

