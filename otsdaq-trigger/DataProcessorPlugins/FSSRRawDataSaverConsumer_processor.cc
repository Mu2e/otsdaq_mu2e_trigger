#include "otsdaq/Macros/ProcessorPluginMacros.h"
#include "otsdaq-trigger/DataProcessorPlugins/FSSRRawDataSaverConsumer.h"

using namespace ots;

//========================================================================================================================
FSSRRawDataSaverConsumer::FSSRRawDataSaverConsumer(
    std::string              supervisorApplicationUID,
    std::string              bufferUID,
    std::string              processorUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       configurationPath)
    : WorkLoop(processorUID)
    , RawDataSaverConsumerBase(supervisorApplicationUID,
                               bufferUID,
                               processorUID,
                               theXDAQContextConfigTree,
                               configurationPath)
{
}

//========================================================================================================================
FSSRRawDataSaverConsumer::~FSSRRawDataSaverConsumer(void) {}

//========================================================================================================================
void FSSRRawDataSaverConsumer::writeHeader(void)
{
	unsigned int version = 0x00010001;
	outFile_.write("stib", 4);
	outFile_.write((char*)&version, 4);
}

DEFINE_OTS_PROCESSOR(FSSRRawDataSaverConsumer)
