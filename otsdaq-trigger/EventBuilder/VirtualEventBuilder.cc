#include "otsdaq-trigger/EventBuilder/VirtualEventBuilder.h"
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-trigger/EventBuilder/Event.h"

#include <TBufferFile.h>
#include <cassert>
#include <iostream>

using namespace ots;

//========================================================================================================================
VirtualEventBuilder::VirtualEventBuilder(std::string supervisorApplicationUID,
                                         std::string bufferUID,
                                         std::string processorUID)
    : WorkLoop(processorUID)
    , DataProducer(supervisorApplicationUID, bufferUID, processorUID)
    , DataConsumer(
          supervisorApplicationUID, bufferUID, processorUID, HighConsumerPriority)

{
}

//========================================================================================================================
VirtualEventBuilder::~VirtualEventBuilder(void) {}

//========================================================================================================================
void VirtualEventBuilder::flush(void)
{
	// FIXME must be implemented!
}
//========================================================================================================================
bool VirtualEventBuilder::workLoopThread(toolbox::task::WorkLoop* workLoop)
{
	// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ << " running!"
	// << std::endl;
	std::string buffer;
	// unsigned long block;
	if(DataConsumer::read<std::string, std::map<std::string, std::string>>(buffer) < 0)
		usleep(100000);
	else
	{
		// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ <<
		// "*************************************" << std::endl;  std::cout <<
		// __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ << " Buffer: " <<
		// buffer
		// << std::endl;  std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ <<
		// "*************************************" << std::endl;
		build(buffer);
		getCompleteEvents();
		// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ << "
		// Number of completed events: " << completeEvents_.size() << std::endl;
		while(!completeEvents_.empty())
		{
			// TBufferFile eventBuffer(TBuffer::kWrite);
			// completeEvents_.front()->Streamer(eventBuffer);
			// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "Writing buffer: "
			// << eventBuffer.Buffer() << std::endl;  int i=0;  write(i);  std::cout <<
			// __COUT_HDR_FL__ << __PRETTY_FUNCTION__ <<
			// "*************************************" << std::endl;
			// FIXME THIS MUST BE UNCOMMENTED OUT TO WORK BUT WE DON'T CARE ABOUT THIS GUY
			// DataProducer::write(*(completeEvents_.front()));  std::cout <<
			// __COUT_HDR_FL__ << __PRETTY_FUNCTION__ <<
			// "*************************************" << std::endl;
			delete completeEvents_.front();
			completeEvents_.pop();
		}
	}
	return true;
}
