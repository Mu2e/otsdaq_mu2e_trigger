#include "otsdaq-trigger/DataDecoders/DataTypes.h"

#include <iostream>

using namespace ots;

//========================================================================================================================
DataTypes::DataTypes(void) {}

//========================================================================================================================
DataTypes::~DataTypes(void) {}

//========================================================================================================================
bool DataTypes::isBCOHigh(uint32_t data)
{
	int type     = data & 0x0f;
	int dataType = (data >> 4) & 0x0f;
	if(type == 9 && dataType == 2)
		return true;
	return false;
}

//========================================================================================================================
bool DataTypes::isBCOLow(uint32_t data)
{
	int type     = data & 0x0f;
	int dataType = (data >> 4) & 0x0f;
	if(type == 9 && dataType == 1)
		return true;
	return false;
}

//========================================================================================================================
bool DataTypes::isTrigger(uint32_t data) { return false; }

//========================================================================================================================
bool DataTypes::isFSSRData(uint32_t data)
{
	int type = data & 0x0f;
	if(type == 1)
		return true;
	return false;
}

//========================================================================================================================
bool DataTypes::isPSI46Data(uint32_t data) { return false; }
