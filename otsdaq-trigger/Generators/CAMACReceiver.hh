#ifndef artdaq_ots_Generators_CAMACReceiver_hh
#define artdaq_ots_Generators_CAMACReceiver_hh

// Some C++ conventions used:

// -Append a "_" to every private member function and variable

#include "artdaq/Application/CommandableFragmentGenerator.hh"

#include <chrono>

#include "fhiclcpp/fwd.h"
#include "otsdaq-trigger/Overlays/CAMACFragment.hh"
//#include "artdaq-ots/Generators/UDPReceiver.hh"
#include "otsdaq-core/DataManager/DataConsumer.h"
#include "otsdaq-core/WorkLoopManager/WorkLoop.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <list>
#include <queue>

namespace ots
{
class CAMACReceiver : public ots::DataConsumer,
                      public artdaq::CommandableFragmentGenerator
{
  public:
	explicit CAMACReceiver(fhicl::ParameterSet const& ps);

	bool getNext_(artdaq::FragmentPtrs& output) override;

	void startProcessingData(std::string runNumber) { ; }
	void stopProcessingData(void) { ; }
	void resumeProcessingData(void) { ; }
	void pauseProcessingData(void) { ; }

	void         start() override;
	virtual void stop() override;
	virtual void stopNoMutex() override;

  private:
	bool workLoopThread(toolbox::task::WorkLoop* workLoop) { return false; }
	void ProcessData_(artdaq::FragmentPtrs& frags);
	void ProcessInput_(std::string input);

	bool        initialized_;
	std::string initializeCAMAC_(std::string initString);

	// FHiCL-configurable variables. Note that the C++ variable names
	// are the FHiCL variable names with a "_" appended
	bool                          rawOutput_;
	std::string                   rawPath_;
	artdaq::Fragment::timestamp_t ts_;
	CAMACFragment::CAMACMetadata  metadata_;

	std::list<std::vector<uint16_t>> lines_;
	std::vector<uint16_t>            nullEvent_;
};
}

#endif /* artdaq_demo_Generators_ToySimulator_hh */
