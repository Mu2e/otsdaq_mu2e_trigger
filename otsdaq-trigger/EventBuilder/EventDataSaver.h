#ifndef _ots_EventDataSaver_h
#define _ots_EventDataSaver_h

#include "otsdaq-core/DataManager/DataConsumer.h"
//#include "otsdaq-core/EventBuilder/Event.h"
#include <string>

class TFile;
class TTree;

namespace ots
{
class Event;

class EventDataSaver : public DataConsumer
{
  public:
	EventDataSaver(std::string      supervisorApplicationUID,
	               std::string      bufferUID,
	               std::string      processorUID,
	               ConsumerPriority priority);
	virtual ~EventDataSaver(void);

	void openFile(std::string fileName);
	void closeFile(void);
	void save(std::string& data);
	// Getters
	std::string getName(void);

	// Setters

  protected:
	void   writeHeader(void);
	bool   workLoopThread(toolbox::task::WorkLoop* workLoop);
	TFile* outFile_;
	Event* anEvent_;
	TTree* outTree_;
};
}

#endif
