#include "otsdaq-trigger/DataDecoders/PSI46DigData.h"

#include <iostream>

using namespace ots;

//========================================================================================================================
PSI46DigData::PSI46DigData(void) {}

//========================================================================================================================
PSI46DigData::~PSI46DigData(void) {}

//========================================================================================================================
bool PSI46DigData::isPSI46Dig(uint32_t data)
{
	int type = data & 0x0f;
	if(type == 1)
		return true;
	return false;
}

//========================================================================================================================
PSI46DigData& PSI46DigData::decode(uint32_t data)
{
	stibId_        = (data >> 30) & 0x03;
	channelNumber_ = (data >> 27) & 0x07;
	chipId_        = (data >> 24) & 0x07;
	set_           = (data >> 12) & 0x1f;
	stripNumber_   = (data >> 17) & 0x0f;
	bco_           = (data >> 4) & 0xff;
	adc_           = (data >> 1) & 0x7;
	// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << " Chan: " << chan << "
	// chipId: " << (int)chipId_
	//    << " set: " << (unsigned int)set_ << " strip: " << (int)stripNumber_ << " bco: "
	//    << (int)bco_ << std::endl;

	return *this;
}

//========================================================================================================================
unsigned int PSI46DigData::getStibId(void) { return (unsigned int)stibId_; }

//========================================================================================================================
unsigned int PSI46DigData::getChannelNumber(void) { return (unsigned int)channelNumber_; }

//========================================================================================================================
unsigned int PSI46DigData::getChipId(void) { return (unsigned int)chipId_; }

//========================================================================================================================
unsigned int PSI46DigData::getStripNumber(void) { return (unsigned int)stripNumber_; }

//========================================================================================================================
unsigned int PSI46DigData::getBco(void) { return (unsigned int)bco_; }

//========================================================================================================================
unsigned int PSI46DigData::getAdc(void) { return (unsigned int)adc_; }

//========================================================================================================================
unsigned int PSI46DigData::getSensorStrip()
{
	static unsigned char set_number[] = {
	    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0,  1,  4, 5, 3,   2,
	    255, 255, 12,  13,  8,   9,   11,  10,  255, 255, 15, 14, 7, 6, 255, 255};
	static unsigned char strip_number[] = {
	    255, 255, 255, 255, 255, 0, 2, 1, 255, 6, 4, 5, 255, 7, 3, 255};

	return 128 * ((int)chipId_ - 1) + set_number[(int)set_] * 8 +
	       strip_number[(int)stripNumber_];
}
