
#include "otsdaq-trigger/Overlays/FragmentType.hh"
#include "otsdaq-trigger/Overlays/STIBEventFragmentWriter.hh"

#include "artdaq-ots/Generators/UDPReceiver.hh"
#include "otsdaq-core/Macros/CoutMacros.h"

//#include "art/Utilities/Exception.h"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/Application/GeneratorMacros.hh"

#include "cetlib/exception.h"
#include "fhiclcpp/ParameterSet.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>

#define VERBOSE_DEBUGGING 0

namespace ots
{
class STIBEventReceiver : public ots::UDPReceiver
{
  public:
	explicit STIBEventReceiver(fhicl::ParameterSet const& ps);

  private:
	struct STIBTrigger
	{
		uint32_t triggerCounter;
		uint8_t  systemStatus;
		uint8_t  fastBCO;
	};

	void ProcessData_(artdaq::FragmentPtrs& frags) override;
	void CreateFragment_(artdaq::FragmentPtrs& frags, uint64_t bunchCounter);

	uint64_t                                triggerOffset_;
	uint64_t                                triggerWidth_;
	bool                                    triggerOffsetSearch_;
	std::map<uint32_t, STIBTrigger>         triggers_;
	std::map<uint32_t, std::list<uint32_t>> events_;
};

STIBEventReceiver::STIBEventReceiver(fhicl::ParameterSet const& ps)
    : UDPReceiver(ps)
    , triggerOffset_(ps.get<uint64_t>("trigger_clock_delay", 0))
    , triggerWidth_(ps.get<uint64_t>("trigger_width", 1))
    , triggerOffsetSearch_(ps.get<bool>("trigger_offset_search", false))
    , triggers_()
    , events_()
{
}

void STIBEventReceiver::CreateFragment_(artdaq::FragmentPtrs& frags,
                                        uint64_t              bunchCounter)
{
	STIBEventFragment::Metadata metadata;
	metadata.port    = dataport_;
	metadata.address = si_data_.sin_addr.s_addr;

	if(!triggers_.count(bunchCounter))
	{
		TLOG_WARNING("STIBEventReceiver")
		    << "Requested Bunch Counter not in trigger map!" << TLOG_ENDL;
	}

	auto& triggerData = triggers_[bunchCounter];

	if(triggerOffsetSearch_)
	{
		for(auto& evt : events_)
		{
			if(evt.second.size())
			{
				TLOG_DEBUG("STIBEventReceiver")
				    << "Event at BCO " << std::to_string(evt.first) << " has "
				    << std::to_string(evt.second.size()) << " hits. "
				    << "Trigger BCO " << std::to_string(bunchCounter) << ", "
				    << "Start: " << std::to_string(bunchCounter - triggerOffset_)
				    << ", End: "
				    << std::to_string(bunchCounter - triggerOffset_ + triggerWidth_ - 1)
				    << ", "
				    << "BCO Offset=" << std::to_string(bunchCounter - evt.first)
				    << TLOG_ENDL;
			}
		}
	}

	std::list<uint32_t> eventData;
	for(auto it = bunchCounter - triggerOffset_;
	    it < bunchCounter - triggerOffset_ + triggerWidth_;
	    ++it)
	{
		if(events_.count(it))
		{
			eventData.splice(eventData.end(), events_[it]);
		}
	}

	// TLOG_DEBUG("STIBEventReceiver") << "End of data marker reached!" << TLOG_ENDL;
	std::size_t initial_payload_size =
	    eventData.size() * sizeof(uint32_t) + sizeof(STIBEventFragment::Header);
	frags.emplace_back(artdaq::Fragment::FragmentBytes(initial_payload_size,
	                                                   ev_counter(),
	                                                   fragment_id(),
	                                                   detail::FragmentType::STIBEVT,
	                                                   metadata,
	                                                   triggerData.triggerCounter));
	TLOG_DEBUG("STIBEventReceiver")
	    << "STIB Fragment: "
	    << "Bunch Counter: " << std::to_string(bunchCounter)
	    << ", Trigger Counter: " << std::to_string(triggerData.triggerCounter)
	    << ", hits: " << std::to_string(eventData.size()) << TLOG_ENDL;
	// We now have a fragment to contain this event:
	ev_counter_inc();
	STIBEventFragmentWriter thisFrag(*frags.back());
	thisFrag.set_hdr_bunch_counter(bunchCounter);
	thisFrag.set_hdr_trigger_counter(triggerData.triggerCounter);
	thisFrag.set_hdr_system_status(triggerData.systemStatus);

	size_t ii = 0;
	for(auto& d : eventData)
	{
		*(thisFrag.dataBegin() + ii) = d;
		++ii;
	}
	events_.erase(events_.begin(),
	              events_.find(bunchCounter - triggerOffset_ + triggerWidth_));
}

void STIBEventReceiver::ProcessData_(artdaq::FragmentPtrs& frags)
{
	std::ofstream output;
	if(rawOutput_)
	{
		std::string outputPath = rawPath_ + "/STIBEventReceiver-" + ip_ + ":" +
		                         std::to_string(dataport_) + ".bin";
		output.open(outputPath, std::ios::out | std::ios::app | std::ios::binary);
	}

	TLOG_TRACE("STIBEventReceiver")
	    << "Starting STIBEventReceiver Packet Processing Loop" << TLOG_ENDL;
	size_t counter = 1;
	for(auto packet = packetBuffers_.begin(); packet != packetBuffers_.end(); ++packet)
	{
		TLOG_TRACE("STIBEventReceiver")
		    << "Packet " << std::to_string(counter) << " / "
		    << std::to_string(packetBuffers_.size()) << TLOG_ENDL;
		counter++;
		for(size_t word = 2; word < packet->size() - 8; word += 8)
		{
			auto data = *reinterpret_cast<uint32_t*>(&(packet->at(word + 0)));
			auto bco  = *reinterpret_cast<uint32_t*>(&(packet->at(word + 4)));

			if(*reinterpret_cast<uint32_t*>(&(*packet)[word]) == 0)
				continue;

#if 0
				TLOG_TRACE("STIBEventReceiver") << "Bytes are: 0: 0x" << std::hex << static_cast<int>(byte0) << ", 1: 0x" << static_cast<int>(byte1) << ", 2: 0x" << static_cast<int>(byte2) << ", 3: 0x" << static_cast<int>(byte3) << "." << TLOG_ENDL;
#endif

			if(rawOutput_)
			{
				output.write((char*)&(packet->at(word)), sizeof(uint64_t));
			}
			if((data & 0x1) == 0x1)
			{
#if VERBOSE_DEBUGGING
				TLOG_TRACE("STIBEventReceiver") << "Data marker!" << TLOG_ENDL;
#endif
				events_[bco].push_back(data);
			}
			else
			{  // Trigger Counter Low
#if VERBOSE_DEBUGGING
				TLOG_TRACE("STIBEventReceiver") << "Trigger marker!" << TLOG_ENDL;
#endif
				auto triggerBunchCounter = bco;
				triggerBunchCounter -= triggerOffset_;
				triggers_[triggerBunchCounter].triggerCounter =
				    ((data & 0x01FFFFFE) >> 1);
				triggers_[triggerBunchCounter].fastBCO      = ((data & 0x0E000000) >> 25);
				triggers_[triggerBunchCounter].systemStatus = ((data & 0xF0000000) >> 28);
				CreateFragment_(frags, triggerBunchCounter);
			}
		}
	}
	return;
}
}
// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(ots::STIBEventReceiver)
