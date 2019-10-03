#ifndef _ots_RootDQMHistosConsumer_h_
#define _ots_RootDQMHistosConsumer_h_

#include "otsdaq/Configurable/Configurable.h"
#include "otsdaq/DataManager/DQMHistosConsumerBase.h"

#include <TServerSocket.h>
#include <string>

namespace ots
{
class ConfigurationManager;

class RootDQMHistosConsumer : public DQMHistosConsumerBase, public Configurable
{
  public:
	RootDQMHistosConsumer(std::string              supervisorApplicationUID,
	                      std::string              bufferUID,
	                      std::string              processorUID,
	                      const ConfigurationTree& theXDAQContextConfigTree,
	                      const std::string&       configurationPath);
	virtual ~RootDQMHistosConsumer(void);

	void startProcessingData(std::string runNumber) override;
	void stopProcessingData(void) override;

  private:
	bool workLoopThread(toolbox::task::WorkLoop* workLoop);
	void fastRead(void);
	void slowRead(void);
	void socketRead(void);

	bool                                  saveDQMFile_;  // yes or no
	std::string                           DQMFilePath_;
	std::string                           DQMFilePrefix_;
	std::vector<std::unique_ptr<TSocket>> sockets_;
	std::unique_ptr<TServerSocket>        listenSocket_;
};
}

#endif
