#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/Macros/ProcessorPluginMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-trigger/DataProcessorPlugins/RootDQMHistosConsumer.h"

#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TMessage.h>

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "RootDQMHistosConsumer"

//========================================================================================================================
RootDQMHistosConsumer::RootDQMHistosConsumer(
    std::string              supervisorApplicationUID,
    std::string              bufferUID,
    std::string              processorUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       configurationPath)
    : WorkLoop(processorUID)
    , DQMHistosConsumerBase(
          supervisorApplicationUID, bufferUID, processorUID, LowConsumerPriority)
    , Configurable(theXDAQContextConfigTree, configurationPath)
    , saveDQMFile_(theXDAQContextConfigTree.getNode(configurationPath)
                       .getNode("SaveDQMFile")
                       .getValue<bool>())
    , DQMFilePath_(theXDAQContextConfigTree.getNode(configurationPath)
                       .getNode("DQMFilePath")
                       .getValue<std::string>())
    , DQMFilePrefix_(theXDAQContextConfigTree.getNode(configurationPath)
                         .getNode("DQMFileNamePrefix")
                         .getValue<std::string>())
    , sockets_()
    , listenSocket_(new TServerSocket(theXDAQContextConfigTree.getNode(configurationPath)
                                          .getNode("RootListenPort")
                                          .getValue<int>(),
                                      kTRUE))
{
	listenSocket_->SetOption(kNoBlock, 1);
}

//========================================================================================================================
RootDQMHistosConsumer::~RootDQMHistosConsumer(void)
{
	closeFile();
	for(auto& socket : sockets_)
		socket->Close();
	listenSocket_->Close();
}

//========================================================================================================================
void RootDQMHistosConsumer::startProcessingData(std::string runNumber)
{
	// IMPORTANT
	// The file must be always opened because even the LIVE DQM uses the pointer to it
	DQMHistosBase::openFile(DQMFilePath_ + "/" + DQMFilePrefix_ + "_Run" + runNumber +
	                        ".root");

	DataConsumer::startProcessingData(runNumber);
}

//========================================================================================================================
void RootDQMHistosConsumer::stopProcessingData(void)
{
	DataConsumer::stopProcessingData();
	if(saveDQMFile_)
	{
		save();
	}
	closeFile();
}

//========================================================================================================================
bool RootDQMHistosConsumer::workLoopThread(toolbox::task::WorkLoop* workLoop)
{
	//__MOUT__ << DataProcessor::processorUID_ << " running, because workloop: " <<
	//	WorkLoop::continueWorkLoop_ << std::endl;
	fastRead();
	return WorkLoop::continueWorkLoop_;
}

//========================================================================================================================
void RootDQMHistosConsumer::fastRead(void) { socketRead(); }

//========================================================================================================================
void RootDQMHistosConsumer::slowRead(void) { socketRead(); }

void RootDQMHistosConsumer::socketRead(void)
{
	// __MOUT__ << "Checking for ROOT Objects to read";
	// Check for new connections
	auto sts = listenSocket_->Accept();
	if((int64_t)sts > 0)
	{
		sockets_.emplace_back(sts);
		sts->SetOption(kNoBlock, 1);
	}

	// Read socket data
	for(auto& socket : sockets_)
	{
		TMessage* message;
		auto      sts = socket->Recv(message);
		if(sts < 0)
			continue;

		theFile_->cd();

		__MOUT__ << "Received ROOT TObject from socket!";
		TObject* h = (TObject*)message->ReadObject(message->GetClass());
		h->Write(0, TObject::kOverwrite);
		delete message;
	}
}

DEFINE_OTS_PROCESSOR(RootDQMHistosConsumer)
