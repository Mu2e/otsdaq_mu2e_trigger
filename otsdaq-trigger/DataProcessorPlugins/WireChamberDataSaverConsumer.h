#ifndef _ots_WireChamberDataSaverConsumer_h_
#define _ots_WireChamberDataSaverConsumer_h_

#include <vector>
#include "otsdaq/DataManager/RawDataSaverConsumerBase.h"

namespace ots
{
struct TDCHeaderStruct
{
	unsigned int tdcSpillWordCount;
	unsigned int tdcNumber;
	unsigned int tdcSpillTriggerCount;
	unsigned int tdcSpillStatus;
	unsigned int tdcWords;
	unsigned int tdcHeaderIndex;
};

struct TDCEvent
{
	unsigned int              wordCount;
	unsigned int              tdcNumber;
	unsigned int              eventStatus;
	unsigned int              triggerNumber;  // 32 bit number
	unsigned int              triggerType;
	unsigned int              controllerEventTimeStamp;
	unsigned int              tdcEventTimeStamp;  // 32 bit number
	unsigned int              dataWords;
	unsigned int              words;
	std::vector<unsigned int> tdcData;
};

class WireChamberDataSaverConsumer : public RawDataSaverConsumerBase
{
  public:
	WireChamberDataSaverConsumer(std::string              supervisorApplicationUID,
	                             std::string              bufferUID,
	                             std::string              processorUID,
	                             const ConfigurationTree& theXDAQContextConfigTree,
	                             const std::string&       configurationPath);
	virtual ~WireChamberDataSaverConsumer(void);

  protected:
	// void writeHeader   (void) override;
	void convertSpillData(const std::string& spillData);
	// void writeDataPaw	  (std::string label, int firstValue, int secondValue, int
	// thirdValue, std::string txt);
	void openFile(std::string runNumber) override;
	void save(const std::string& data) override;

  private:
	std::vector<TDCHeaderStruct> vectorOfTDCHeaders_;
	std::vector<TDCEvent>        vectorOfTDCEvents_;
	// Constants
	unsigned int NUMBER_OF_TDCs;
};
}

#endif
