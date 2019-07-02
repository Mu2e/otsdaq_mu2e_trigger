#include "otsdaq-trigger/DataDecoders/BCOData.h"

#include <iostream>

using namespace ots;

//========================================================================================================================
BCOData::BCOData(void) {}

//========================================================================================================================
BCOData::~BCOData(void) {}

//========================================================================================================================
bool BCOData::isBCOHigh(uint32_t data)
{
	// int type     = data & 0xf;
	// int dataType = (data>>4) & 0xf;
	// if(type==8 && dataType==2) return true;
	if((data & 0xf) == 8 && (data & 0xf0) == 0x20)
		return true;
	return false;
}

//========================================================================================================================
bool BCOData::isBCOLow(uint32_t data)
{
	// int type     = data&0x0f;
	// int dataType = (data>>4)&0x0f;
	// if(type==8 && dataType==1) return true;
	if((data & 0xf) == 8 && (data & 0xf0) == 0x10)
		return true;
	return false;
}

//========================================================================================================================
uint32_t BCOData::decodeBCOHigh(uint32_t data) { return (data >> 8) & 0xffffff; }

//========================================================================================================================
uint32_t BCOData::decodeBCOLow(uint32_t data) { return (data >> 8) & 0xffffff; }

//========================================================================================================================
uint64_t BCOData::mergeBCOHighAndLow(uint32_t bcoHigh, uint32_t bcoLow)
{
	uint64_t bco = 0;
	bco |= ((uint64_t)bcoHigh) << 24;
	bco |= (uint64_t)bcoLow;
	return bco;
}
//========================================================================================================================
void BCOData::insertBCOHigh(uint64_t& bco, uint32_t dataBCOHigh)
{
	bco |= ((uint64_t)decodeBCOHigh(dataBCOHigh)) << 24;
}

//========================================================================================================================
void BCOData::insertBCOLow(uint64_t& bco, uint32_t dataBCOLow)
{
	bco |= (uint64_t)decodeBCOLow(dataBCOLow);
}
