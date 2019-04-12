#ifndef artdaq_ots_Generators_WireChamberEventReceiver_hh
#define artdaq_ots_Generators_WireChamberEventReceiver_hh

// Some C++ conventions used:

// -Append a "_" to every private member function and variable

#include "artdaq/Application/CommandableFragmentGenerator.hh"

#include <chrono>

#include "artdaq-core/Data/Fragment.hh"
#include "fhiclcpp/fwd.h"
//#include "artdaq-ots/Generators/UDPReceiver.hh"
#include "otsdaq-core/DataManager/DataConsumer.h"
#include "otsdaq-trigger/Overlays/WireChamberFragment.hh"

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
class WireChamberEventReceiver : public ots::DataConsumer,
                                 public artdaq::CommandableFragmentGenerator
{
  public:
	explicit WireChamberEventReceiver(fhicl::ParameterSet const& ps);

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
	void ProcessSpillData_(artdaq::FragmentPtrs& frags);
	void ProcessTriggerData_(artdaq::FragmentPtrs& frags, WireChamberFragment& spillData);
	std::string buffer_;

	// FHiCL-configurable variables. Note that the C++ variable names
	// are the FHiCL variable names with a "_" appended
	bool                                           rawOutput_;
	std::string                                    rawPath_;
	int                                            dataport_;
	std::string                                    ip_;
	std::string                                    mwpc_hostname_;
	uint8_t                                        expectedPacketNumber_;
	size_t                                         expectedDataSize_;
	bool                                           sendCommands_;
	bool                                           sendEndOfSubrunFrag_;
	double                                         fragmentWindow_;
	std::chrono::high_resolution_clock::time_point lastFrag_;
};
}

#endif /* artdaq_demo_Generators_ToySimulator_hh */
