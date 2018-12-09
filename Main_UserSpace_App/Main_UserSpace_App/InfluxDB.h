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
	InfluxDB(TAG_MAP const &tag_set, FIELD_MAP const &field_set, int32_t timestamp = -1, std::string const &Measurement = "");
	
	// METHODs
	void				Set_Timestamp(uint32_t Timestamp);
	void				Set_Measurement(std::string const &Measurement);
	std::string const   Get_RawString()		const;
private:	
	// MEMBER
	std::string		 mMeasurement;
	const TAG_MAP	&mTag_set;
	const FIELD_MAP	&mField_set;
	int32_t			 mTimestamp; 	// set to -1 if default should be used
};

