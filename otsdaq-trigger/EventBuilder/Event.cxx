#include "otsdaq-trigger/EventBuilder/Event.h"
#include "otsdaq/Macros/CoutMacros.h"
#include "otsdaq/MessageFacility/MessageFacility.h"

#include <iostream>

using namespace ots;

ClassImp(Event)

    //========================================================================================================================
    Event::Event()
    : triggerNumber_(-1), bcoNumber_(-1)
{
}

//========================================================================================================================
Event::Event(int64_t bcoNumber) : triggerNumber_(-1), bcoNumber_(bcoNumber) {}

//========================================================================================================================
Event::~Event(void) {}

//========================================================================================================================
int32_t Event::getTriggerNumber(void) { return triggerNumber_; }

//========================================================================================================================
int64_t Event::getBCONumber(void) { return bcoNumber_; }

//========================================================================================================================
uint32_t Event::getRawHit(unsigned int position)
{
	if(position < rawHits_.size())
		return rawHits_[position];
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__
	          << "Asking for raw hit in position " << position << " but there are only "
	          << rawHits_.size() << " hits!" << std::endl;
	return 0;
}

//========================================================================================================================
unsigned int Event::getNumberOfRawHits(void) { return rawHits_.size(); }

//========================================================================================================================
const std::vector<uint32_t>& Event::getRawHits(void) { return rawHits_; }

//========================================================================================================================
void Event::setTriggerNumber(int32_t triggerNumber) { triggerNumber_ = triggerNumber; }

//========================================================================================================================
void Event::setBCONumber(int64_t bcoNumber) { bcoNumber_ = bcoNumber; }

//========================================================================================================================
void Event::setRawHit(uint32_t rawHit) { rawHits_.push_back(rawHit); }

//========================================================================================================================
void Event::setRawHits(const std::vector<uint32_t>& rawHits) { rawHits_ = rawHits; }
