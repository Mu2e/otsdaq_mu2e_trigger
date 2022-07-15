#ifndef _ots_WireChamberDQMHistosConsumer_h_
#define _ots_WireChamberDQMHistosConsumer_h_

#include <string>
#include "otsdaq/Configurable/Configurable.h"
#include "otsdaq/DataManager/DQMHistosConsumerBase.h"
#include "otsdaq-trigger/DQMHistos/WireChamberDQMHistos.h"
class TFile;
class TDirectory;

namespace ots
{
class WireChamberDQMHistosConsumer : public DQMHistosConsumerBase,
                                     public WireChamberDQMHistos,
                                     public Configurable
{
  public:
	WireChamberDQMHistosConsumer(std::string              supervisorApplicationUID,
	                             std::string              bufferUID,
	                             std::string              processorUID,
	                             const ConfigurationTree& theXDAQContextConfigTree,
	                             const std::string&       configurationPath);
	virtual ~WireChamberDQMHistosConsumer(void);

	void startProcessingData(std::string runNumber) override;
	void stopProcessingData(void) override;
	void load(std::string fileName) { ; }

  private:
	bool workLoopThread(toolbox::task::WorkLoop* workLoop);
	void fastRead(void);
	void slowRead(void);

	// For fast read
	std::string*                        dataP_;
	std::map<std::string, std::string>* headerP_;

	std::string filePath_;
	std::string fileRadix_;
	bool        saveFile_;  // yes or no
};
}

#endif
