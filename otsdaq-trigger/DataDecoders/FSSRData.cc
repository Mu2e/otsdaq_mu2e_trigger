#include "otsdaq-trigger/DataDecoders/FSSRData.h"

#include <iostream>

using namespace ots;

//========================================================================================================================
FSSRData::FSSRData(void) {}

//========================================================================================================================
FSSRData::~FSSRData(void) {}

//========================================================================================================================
bool FSSRData::isFSSR(uint32_t data)
{
	return data & 0x1;  // 1 true 0 false
}

//========================================================================================================================
FSSRData& FSSRData::decode(uint32_t data)
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
unsigned int FSSRData::getStibId(void) { return (unsigned int)stibId_; }

//========================================================================================================================
unsigned int FSSRData::getChannelNumber(void) { return (unsigned int)channelNumber_; }

//========================================================================================================================
unsigned int FSSRData::getChipId(void) { return (unsigned int)chipId_; }

//========================================================================================================================
unsigned int FSSRData::getStripNumber(void) { return (unsigned int)stripNumber_; }

//========================================================================================================================
unsigned int FSSRData::getBco(void) { return (unsigned int)bco_; }

//========================================================================================================================
unsigned int FSSRData::getAdc(void) { return (unsigned int)adc_; }

//========================================================================================================================
unsigned int FSSRData::getSensorStrip()
{
	static unsigned char set_number[] = {
	    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0,  1,  4, 5, 3,   2,
	    255, 255, 12,  13,  8,   9,   11,  10,  255, 255, 15, 14, 7, 6, 255, 255};
	static unsigned char strip_number[] = {
	    255, 255, 255, 255, 255, 0, 2, 1, 255, 6, 4, 5, 255, 7, 3, 255};

	return 128 * ((int)chipId_ - 1) + set_number[(int)set_] * 8 +
	       strip_number[(int)stripNumber_];
}
