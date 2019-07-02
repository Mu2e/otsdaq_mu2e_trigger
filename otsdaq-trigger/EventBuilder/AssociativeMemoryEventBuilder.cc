#include "otsdaq-trigger/EventBuilder/AssociativeMemoryEventBuilder.h"
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-trigger/EventBuilder/Event.h"

#include <iostream>

using namespace ots;

//========================================================================================================================
AssociativeMemoryEventBuilder::AssociativeMemoryEventBuilder(
    std::string      supervisorApplicationUID,
    std::string      bufferUID,
    std::string      processorUID,
    ConsumerPriority priority)
    : WorkLoop(processorUID)
    , VirtualEventBuilder(supervisorApplicationUID, bufferUID, processorUID)
    , bcoIsComplete_(true)
    , currentBCO_(0)
{
}

//========================================================================================================================
AssociativeMemoryEventBuilder::~AssociativeMemoryEventBuilder(void) {}

//========================================================================================================================
void AssociativeMemoryEventBuilder::build(const std::string& buffer)
{
	// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ <<
	// "********************************Begin buffer: " << buffer << std::endl;
	theDataDecoder_.convertBuffer(buffer, convertedBuffer_, true);
	// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ << " Buffer
	// size: " << convertedBuffer_.size() << std::endl;
	while(!convertedBuffer_.empty())
	{
		// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ << " Data:
		// " << hex << convertedBuffer_.front() << dec  << " size: " <<
		// convertedBuffer_.size() << std::endl;
		if(theDataDecoder_.isBCOHigh(convertedBuffer_.front()) ||
		   theDataDecoder_.isBCOLow(convertedBuffer_.front()))
		{
			if(bcoIsComplete_)
			{
				// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ <<
				// " High: " << hex << convertedBuffer_.front() << dec << std::endl;
				currentBCO_ = 0;
				theDataDecoder_.insertBCOHigh(currentBCO_, convertedBuffer_.front());
				bcoIsComplete_ = false;
				convertedBuffer_.pop();
				if(convertedBuffer_.empty())
					return;
			}
			// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ << "
			// Low: " << hex << convertedBuffer_.front() << dec << std::endl;
			theDataDecoder_.insertBCOLow(
			    currentBCO_,
			    convertedBuffer_.front());  // After a BCO High there must be a Low
			if(memory_.find(currentBCO_) == memory_.end())
				memory_[currentBCO_] = new Event(currentBCO_);
			bcoIsComplete_ = true;
			convertedBuffer_.pop();
			continue;
		}
		if(bcoIsComplete_)
		{
			if(memory_.find(currentBCO_) != memory_.end())
			{
				// if(theDataDecoder_.isTriggerHigh(convertedBuffer_.front()))
				//    memory_[currentBCO_]->setTriggerNumber(theDataDecoder_.decodeTriggerHigh(convertedBuffer_.front()));
				// else
				//    memory_[currentBCO_]->setRawHit(convertedBuffer_.front());
			}
			else
			{
				// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ <<
				// "****************************************************************" <<
				// std::endl;  std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ <<
				// processorUID_ << " Data is coming before any high BCO you MUST fix the
				// start procedure!" << std::endl;  std::cout << __COUT_HDR_FL__ <<
				// __PRETTY_FUNCTION__ << processorUID_ <<
				// "****************************************************************" <<
				// std::endl;
			}
		}
		else
		{
			// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ <<
			// "****************************************************************" <<
			// std::endl;  std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ <<
			// processorUID_ << " Data is missing the low bco!" << std::endl;  std::cout
			// <<
			// __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ <<
			// "****************************************************************" <<
			// std::endl;
		}
		convertedBuffer_.pop();
	}
	// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ <<
	// "********************************End buffer: " << buffer << std::endl;
}

//========================================================================================================================
std::queue<Event*>& AssociativeMemoryEventBuilder::getCompleteEvents(void)
{
	return getCompleteEvents(512);
}

//========================================================================================================================
std::queue<Event*>& AssociativeMemoryEventBuilder::getAllEvents(void)
{
	return getCompleteEvents(0);
}

//========================================================================================================================
std::queue<Event*>& AssociativeMemoryEventBuilder::getCompleteEvents(
    unsigned int bcoDifference)
{
	uint64_t currentBCO = memory_.rbegin()->second->getBCONumber();

	for(std::map<uint64_t, Event*>::iterator it = memory_.begin(); it != memory_.end();
	    it++)
		if(currentBCO - it->second->getBCONumber() >= bcoDifference)
		{
			completeEvents_.push(it->second);  // The pointer is not erased because now it
			                                   // is owned by the completeEvents_ queue
			memory_.erase(it--);
		}
	return VirtualEventBuilder::completeEvents_;
}
