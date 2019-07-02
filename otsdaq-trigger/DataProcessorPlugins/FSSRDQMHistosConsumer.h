#ifndef _ots_FSSRDQMHistosConsumer_h_
#define _ots_FSSRDQMHistosConsumer_h_

#include <string>
#include "otsdaq-core/Configurable/Configurable.h"
#include "otsdaq-core/DataManager/DQMHistosConsumerBase.h"
#include "otsdaq-trigger/DQMHistos/FSSRDQMHistos.h"
class TFile;
class TDirectory;

namespace ots
{
class FSSRDQMHistosConsumer : public DQMHistosConsumerBase,
                              public FSSRDQMHistos,
                              public Configurable
{
  public:
	FSSRDQMHistosConsumer(std::string              supervisorApplicationUID,
	                      std::string              bufferUID,
	                      std::string              processorUID,
	                      const ConfigurationTree& theXDAQContextConfigTree,
	                      const std::string&       configurationPath);
	virtual ~FSSRDQMHistosConsumer(void);

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
