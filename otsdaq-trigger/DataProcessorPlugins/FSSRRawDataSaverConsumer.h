#ifndef _ots_FSSRRawDataSaverConsumer_h_
#define _ots_FSSRRawDataSaverConsumer_h_

#include "otsdaq-core/DataManager/RawDataSaverConsumerBase.h"

namespace ots
{
class FSSRRawDataSaverConsumer : public RawDataSaverConsumerBase
{
  public:
	FSSRRawDataSaverConsumer(std::string              supervisorApplicationUID,
	                         std::string              bufferUID,
	                         std::string              processorUID,
	                         const ConfigurationTree& theXDAQContextConfigTree,
	                         const std::string&       configurationPath);
	virtual ~FSSRRawDataSaverConsumer(void);

	virtual void writePacketHeader(const std::string& data)
	{
		unsigned char quadWordsCount = (data.length() - 2) / 8;
		outFile_.write((char*)&quadWordsCount, 1);

		// seqId is in data[1] position

		if(quadWordsCount)
		{
			unsigned char seqId = data[1];
			if(!(lastSeqId_ + 1 == seqId || (lastSeqId_ == 255 && seqId == 0)))
			{
				__COUT__ << "?????? NOOOO Missing Packets: " << (unsigned int)lastSeqId_
				         << " v " << (unsigned int)seqId << __E__;
			}
			lastSeqId_ = seqId;
		}
	}

  protected:
	void writeHeader(void) override;

	unsigned char lastSeqId_;
};
}

#endif
