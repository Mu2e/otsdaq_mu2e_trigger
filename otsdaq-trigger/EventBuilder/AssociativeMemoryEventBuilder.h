#ifndef _ots_AssociativeMemoryEventBuilder_h
#define _ots_AssociativeMemoryEventBuilder_h

#include <stdint.h>
#include <queue>
#include <string>
#include "otsdaq-trigger/DataDecoders/DataDecoder.h"
#include "otsdaq-trigger/EventBuilder/VirtualEventBuilder.h"

namespace ots
{
class Event;

class AssociativeMemoryEventBuilder : public VirtualEventBuilder
{
  public:
	AssociativeMemoryEventBuilder(std::string      supervisorApplicationUID,
	                              std::string      bufferUID,
	                              std::string      processorUID,
	                              ConsumerPriority priority);
	virtual ~AssociativeMemoryEventBuilder(void);

	void                build(const std::string& buffer);
	std::queue<Event*>& getCompleteEvents(void);
	std::queue<Event*>& getAllEvents(void);

  private:
	std::queue<Event*>&        getCompleteEvents(unsigned int bcoDifference);
	std::map<uint64_t, Event*> memory_;
	bool                       bcoIsComplete_;
	uint64_t                   currentBCO_;
	std::queue<uint32_t>       convertedBuffer_;
	DataDecoder                theDataDecoder_;
};
}

#endif
