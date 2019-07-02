#include "otsdaq-trigger/DataDecoders/TriggerData.h"

#include <iostream>

using namespace ots;

//========================================================================================================================
TriggerData::TriggerData(void) {}

//========================================================================================================================
TriggerData::~TriggerData(void) {}

//========================================================================================================================
bool TriggerData::isTrigger(uint32_t data)
{
	// int type     = data & 0xf;
	// int dataType = (data>>4) & 0xf;
	// if(type==8 && dataType==0xb) return true;
	if((data & 0xf) == 8 && (data & 0xf0) == 0xf0)
		return true;
	else
		return false;
}

//========================================================================================================================
uint32_t TriggerData::decodeTrigger(uint32_t data) { return (data >> 8) & 0xffffff; }
