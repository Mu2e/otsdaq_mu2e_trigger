#ifndef _ots_VirtualEventBuilder_h_
#define _ots_VirtualEventBuilder_h_

#include <queue>
#include <string>
#include "otsdaq/DataManager/DataConsumer.h"
#include "otsdaq/DataManager/DataProducer.h"

namespace ots
{
class Event;

class VirtualEventBuilder : public DataProducer, public DataConsumer
{
  public:
	VirtualEventBuilder(std::string supervisorApplicationUID,
	                    std::string bufferUID,
	                    std::string processorUID);
	virtual ~VirtualEventBuilder(void);

	virtual void                build(const std::string& buffer) = 0;
	virtual std::queue<Event*>& getCompleteEvents(void)          = 0;
	virtual std::queue<Event*>& getAllEvents(void)               = 0;
	void                        flush(void);
	// Getters
	std::string getName(void);

  protected:
	bool               workLoopThread(toolbox::task::WorkLoop* workLoop);
	std::queue<Event*> completeEvents_;  // The VirtualEventBuilder is the owner of these
	                                     // events and it will delete them after the use!
};
}

#endif
