#include "otsdaq-trigger/EventBuilder/EventDataSaver.h"
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-trigger/EventBuilder/Event.h"

#include <cassert>
#include <iostream>

#include <TBufferFile.h>
#include <TFile.h>
#include <TTree.h>

using namespace ots;

//========================================================================================================================
EventDataSaver::EventDataSaver(std::string      supervisorApplicationUID,
                               std::string      bufferUID,
                               std::string      processorUID,
                               ConsumerPriority priority)
    : WorkLoop(processorUID)
    , DataConsumer(
          supervisorApplicationUID, bufferUID, processorUID, HighConsumerPriority)
    , outFile_(0)
    , anEvent_(new Event())
    , outTree_(0)
{
}

//========================================================================================================================
EventDataSaver::~EventDataSaver(void)
{
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "Destructor" << std::endl;
	closeFile();
	delete anEvent_;
}

//========================================================================================================================
void EventDataSaver::openFile(std::string fileName)
{
	// anEvent_ = new Event();//FIXME maybe, hopefully is deleted when the file is closed
	outFile_ = TFile::Open((fileName + "_Event.root").c_str(), "NEW");
	if(!outFile_->IsOpen())
	{
		std::cout << __COUT_HDR_FL__ << "Can't open file " << fileName + "_Event.root"
		          << std::endl;
		assert(0);
	}
	outFile_->cd();
	outTree_ = new TTree("EventTree", "Event Tree");  // deleted when the file closes
	outTree_->SetAutoSave(10000);                     // Saves every 1 Mbyte
	outTree_->Branch("EventBranch", anEvent_->GetName(), &anEvent_, 16000, 0);
	writeHeader();
}

//========================================================================================================================
void EventDataSaver::closeFile(void)
{
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "Saving file!" << std::endl;
	if(outFile_ != 0 && outFile_->IsOpen())
	{
		// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ <<
		// "*************************************" << std::endl;  std::cout <<
		// __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ << " Saving event" <<
		// std::endl;  std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ <<
		// "*************************************" << std::endl;
		outFile_->Write();
		outFile_->Close();
		outFile_ = 0;
		outTree_ = 0;
	}
}

//========================================================================================================================
void EventDataSaver::save(std::string& data)
{
	// TBufferFile eventBuffer(TBuffer::kRead, data.length(), &(data.at(0)));
	// anEvent_->Streamer(eventBuffer);

	// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ <<
	// "*************************************" << std::endl;  std::cout << __COUT_HDR_FL__
	// << __PRETTY_FUNCTION__ << processorUID_ << " Filling event" << std::endl; std::cout
	// << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ <<
	// "*************************************"
	// << std::endl;
	if(outTree_ != 0)
		outTree_->Fill();
}

//========================================================================================================================
void EventDataSaver::writeHeader(void)
{
	//    unsigned char fileFormatVersion = 1;
	//    unsigned char dataFormatVersion = 1;
	//    outFile_.write( (char*)&fileFormatVersion, sizeof(char));
	//    outFile_.write( (char*)&dataFormatVersion, sizeof(char));
}

//========================================================================================================================
bool EventDataSaver::workLoopThread(toolbox::task::WorkLoop* workLoop)
{
	// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ << " running!"
	// << std::endl;
	std::string buffer;
	// unsigned long block;
	if(read<ots::Event, std::map<std::string, std::string>>(*anEvent_) < 0)
		usleep(100000);
	else
	{
		// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ <<
		// "*************************************" << std::endl;  std::cout <<
		// __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ << " BCO Number: " <<
		// anEvent_->getBCONumber() << std::endl;  std::cout << __COUT_HDR_FL__ <<
		// __PRETTY_FUNCTION__ << "*************************************" << std::endl;
		save(buffer);
	}
	return true;
}
