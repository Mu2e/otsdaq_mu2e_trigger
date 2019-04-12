#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "canvas/Utilities/InputTag.h"

#include "artdaq-core/Data/ContainerFragment.hh"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/DAQdata/Globals.hh"

#include "otsdaq-core/NetworkUtilities/UDPDataStreamerBase.h"
#include "otsdaq-trigger/Overlays/FragmentType.hh"
#include "otsdaq-trigger/Overlays/WireChamberFragment.hh"

#include "cetlib_except/exception.h"

#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
#include <unordered_map>
#include <vector>

#define MAX_CHAMBERS 4
#define MAX_MODULES 4
#define MAX_WIRES 128
#define MAX_HITS 1280

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "WireChamberPythonOutput"

namespace ots
{
class WireChamberPythonOutput : public art::EDAnalyzer
{
  public:
	explicit WireChamberPythonOutput(fhicl::ParameterSet const& p);
	virtual ~WireChamberPythonOutput() = default;

	void analyze(art::Event const& e) override;
	void beginRun(art::Run const&) override;
	void endRun(art::Run const&) override;

  private:
	art::RunNumber_t current_run_;
	std::string      outputFilePath_;
	std::string      timeString_;
	std::unordered_map<artdaq::Fragment::fragment_id_t, std::unique_ptr<std::ofstream>>
	                                                            outputFiles_;
	std::unordered_map<artdaq::Fragment::fragment_id_t, size_t> eventCounters_;
};
}

ots::WireChamberPythonOutput::WireChamberPythonOutput(fhicl::ParameterSet const& ps)
    : art::EDAnalyzer(ps)
    , current_run_(0)
    , outputFilePath_(ps.get<std::string>("filePath", ""))
    , timeString_("00000000000000")
    , outputFiles_()
    , eventCounters_()
{
	TLOG_INFO("WireChamberPythonOutput")
	    << "WireChamberPythonOutput CONSTRUCTOR" << TLOG_ENDL;
}

void ots::WireChamberPythonOutput::analyze(art::Event const& e)
{
	TLOG_INFO("WireChamberPythonOutput")
	    << "WireChamberPythonOutput Analyzing event " << e.event() << TLOG_ENDL;

	// John F., 1/22/14 -- there's probably a more elegant way of
	// collecting fragments of various types using ART interface code;
	// will investigate. Right now, we're actually re-creating the
	// fragments locally

	artdaq::Fragments        fragments;
	artdaq::FragmentPtrs     containerFragments;
	std::vector<std::string> fragment_type_labels{"MWPC", "Container", "ContainerMWPC"};

	TLOG_DEBUG("WireChamberPythonOutput")
	    << "WireChamberPythonOutput Extracting Fragments" << TLOG_ENDL;
	for(auto label : fragment_type_labels)
	{
		art::Handle<artdaq::Fragments> fragments_with_label;
		e.getByLabel("daq", label, fragments_with_label);

		if(!fragments_with_label.isValid())
			continue;
		//    for (int i_l = 0; i_l < static_cast<int>(fragments_with_label->size());
		//    ++i_l) {
		//      fragments.emplace_back( (*fragments_with_label)[i_l] );
		//    }

		if(label == "Container" || label == "ContainerMWPC")
		{
			for(auto cont : *fragments_with_label)
			{
				artdaq::ContainerFragment contf(cont);
				for(size_t ii = 0; ii < contf.block_count(); ++ii)
				{
					containerFragments.push_back(contf[ii]);
					fragments.push_back(*containerFragments.back());
				}
			}
		}
		else
		{
			for(auto frag : *fragments_with_label)
			{
				fragments.emplace_back(frag);
			}
		}
	}
	TLOG_DEBUG("WireChamberPythonOutput")
	    << "WireChamberPythonOutput: This event has " << fragments.size()
	    << " MWPC Event Fragments" << TLOG_ENDL;

	// John F., 1/5/14

	// Here, we loop over the fragments passed to the analyze
	// function. A warning is flashed if either (A) the fragments aren't
	// all from the same event, or (B) there's an unexpected number of
	// fragments given the number of boardreaders and the number of
	// fragments per board

	// For every Nth event, where N is the "prescale" setting, plot the
	// distribution of ADC counts from each board_id / fragment_id
	// combo. Also, if "digital_sum_only" is set to false in the FHiCL
	// string, then plot, for the Nth event, a graph of the ADC values
	// across all channels in each board_id / fragment_id combo

	artdaq::Fragment::sequence_id_t expected_sequence_id =
	    std::numeric_limits<artdaq::Fragment::sequence_id_t>::max();

	//  for (std::size_t i = 0; i < fragments.size(); ++i) {
	for(const auto& frag : fragments)
	{
		// Pointers to the types of fragment overlays WireChamberPythonOutput can handle;
		// only one will be used per fragment, of course

		std::unique_ptr<WireChamberFragment> wcPtr;

		//  const auto& frag( fragments[i] );  // Basically a shorthand

		//    if (i == 0)
		if(expected_sequence_id ==
		   std::numeric_limits<artdaq::Fragment::sequence_id_t>::max())
		{
			expected_sequence_id = frag.sequenceID();
		}

		if(expected_sequence_id != frag.sequenceID())
		{
			TLOG_WARNING("WireChamberPythonOutput")
			    << "Warning in WireChamberPythonOutput: expected fragment with sequence "
			       "ID "
			    << expected_sequence_id << ", received one with sequence ID "
			    << frag.sequenceID() << TLOG_ENDL;
		}

		FragmentType fragtype = static_cast<FragmentType>(frag.type());
		// TLOG_DEBUG("WireChamberPythonOutput") << "WireChamberPythonOutput: Fragment
		// type is " << fragtype << " (DataGen=" << FragmentType::DataGen << ")" <<
		// TLOG_ENDL;
		// John F., 1/22/14 -- this should definitely be improved; I'm
		// just using the max # of bits per ADC value for a given fragment
		// type as is currently defined for the V172x fragments (as
		// opposed to the Toy fragment, which have this value in their
		// metadata). Since it's not using external variables for this
		// quantity, this would need to be edited should these values
		// change.

		switch(fragtype)
		{
		case FragmentType::MWPC:
			wcPtr.reset(new WireChamberFragment(frag));
			break;
		default:
			throw cet::exception(
			    "Error in WireChamberPythonOutput: unknown fragment type supplied: " +
			    fragmentTypeToString(fragtype));
		}

		TLOG_TRACE("WireChamberPythonOutput")
		    << "Writing MWPC Spill Data to Python-format output file" << TLOG_ENDL;

		artdaq::Fragment::fragment_id_t fragment_id = frag.fragmentID();
		if(!outputFiles_.count(fragment_id))
		{
			TLOG_INFO("WireChamberPythonOutput")
			    << "Opening file for fragment_id " << std::to_string(fragment_id)
			    << TLOG_ENDL;
			std::stringstream fileName;
			fileName << outputFilePath_ << "/exp_" << timeString_ << "_r"
			         << std::setfill('0') << std::setw(6) << current_run_ << std::setw(0)
			         << std::setfill(' ') << "_c" << fragment_id << ".dat";
			outputFiles_[fragment_id].reset(new std::ofstream(fileName.str()));
			*outputFiles_[fragment_id] << "test 120 GeV OTSDAQ" << std::endl;
			eventCounters_[fragment_id] = 0;
			TLOG_DEBUG("WireChamberPythonOutput")
			    << "Done opening file " << fileName.str() << TLOG_ENDL;
		}

		TLOG_TRACE("WireChamberPythonOutput") << "Writing Spill Header" << TLOG_ENDL;
		*outputFiles_[fragment_id] << "SPILL    " << std::setw(8)
		                           << (int)wcPtr->GetControllerHeader()->SpillCounter
		                           << std::endl;
		auto year =
		    WireChamberFragment::ConvertBCD(wcPtr->GetControllerHeader()->YearMonth >> 8);
		auto month = WireChamberFragment::ConvertBCD(
		    wcPtr->GetControllerHeader()->YearMonth & 0xFF);
		auto day =
		    WireChamberFragment::ConvertBCD(wcPtr->GetControllerHeader()->DaysHours >> 8);
		auto hour = WireChamberFragment::ConvertBCD(
		    wcPtr->GetControllerHeader()->DaysHours & 0xFF);
		auto minute = WireChamberFragment::ConvertBCD(
		    wcPtr->GetControllerHeader()->MinutesSeconds >> 8);
		auto seconds = WireChamberFragment::ConvertBCD(
		    wcPtr->GetControllerHeader()->MinutesSeconds & 0xFF);
		*outputFiles_[fragment_id] << std::setfill('0');
		*outputFiles_[fragment_id] << "SDATE    " << std::setw(2) << day << "      "
		                           << std::setw(2) << month << "      " << std::setw(2)
		                           << year << std::endl;
		*outputFiles_[fragment_id] << "STIME    " << std::setw(2) << hour << "      "
		                           << std::setw(2) << minute << "      " << std::setw(2)
		                           << seconds << std::endl;
		*outputFiles_[fragment_id]
		    << "TTIME    " << std::setw(2) << day << "/" << std::setw(2) << month << "/"
		    << std::setw(2) << year << " " << std::setw(2) << hour << ":" << std::setw(2)
		    << minute << ":" << std::setw(2) << seconds << std::endl;
		*outputFiles_[fragment_id] << std::setfill(' ');
		TLOG_TRACE("WireChamberPythonOutput") << "Done writing Spill Header" << TLOG_ENDL;

		auto evtPtr = wcPtr->dataBegin();
		if(evtPtr != nullptr)
		{
			auto eventNumber =
			    wcPtr->TDCEventTriggerCounter(evtPtr) +
			    1;  // Offset by one so that if statement below is triggered
			auto tdc = evtPtr->TDCNumber +
			           1;  // Offset by one so that if statement below is triggered

			while(evtPtr != nullptr)
			{
				// 1st consistency check
				auto chamberModuleNumber =
				    (evtPtr->TDCNumber - 1) % 4;  // module number within chamber, 0-3
				auto chamber = (evtPtr->TDCNumber - 1) / 4;

				if(chamberModuleNumber >= MAX_MODULES || chamber >= MAX_CHAMBERS)
				{
					TLOG_TRACE("WireChamberPythonOutput")
					    << "Bad data detected, moving on to next event" << TLOG_ENDL;
					evtPtr = wcPtr->nextEvent(evtPtr);
					continue;
				}

				TLOG_TRACE("WireChamberPythonOutput")
				    << "Writing Event Header" << TLOG_ENDL;
				// Print Event Header
				if(eventNumber != wcPtr->TDCEventTriggerCounter(evtPtr))
				{
					eventCounters_[fragment_id] += 1;
					eventNumber = wcPtr->TDCEventTriggerCounter(evtPtr);
					*outputFiles_[fragment_id]
					    << "EVENT    " << std::setw(8) << (int)eventCounters_[fragment_id]
					    << "    " << std::setw(8) << (int)eventNumber << std::endl;
					*outputFiles_[fragment_id]
					    << "ETIME    " << std::setw(8)
					    << (int)evtPtr->ControllerEventTimeStamp << "    " << std::setw(8)
					    << wcPtr->TDCEventTimestamp(evtPtr) << std::endl;
				}

				// Print Module header
				if(evtPtr->TDCNumber != tdc)
				{
					*outputFiles_[fragment_id] << "Module   " << std::setw(8)
					                           << (int)evtPtr->TDCNumber << std::endl;
					tdc = evtPtr->TDCNumber;
				}
				size_t dataSz;
				auto   eventData = wcPtr->eventData(evtPtr, dataSz);

				// Check against unreasonable sizes
				if(dataSz > wcPtr->sizeBytes())
				{
					evtPtr = nullptr;
					break;
				}

				TLOG_TRACE("WireChamberPythonOutput")
				    << "Writing Event Data" << TLOG_ENDL;
				// TLOG_DEBUG("WireChamberPythonOutput") << "Processing " <<
				// std::to_string(dataSz) << " hits in chamber " <<
				// std::to_string(chamber)
				// << " module " << std::to_string(chamberModuleNumber) << " trigger
				// number " << std::to_string(frag->TDCEventTriggerCounter(evtPtr)) <<
				// TLOG_ENDL;
				unsigned lastChannel = 0;

				for(size_t ii = 0; ii < dataSz; ++ii)
				{
					auto         data = eventData[ii];
					unsigned int chan = data.ChannelNumber;

					// TLOG_DEBUG("WireChamberPythonOutput") << "Channel number " <<
					// std::to_string(chan) << " (last " << std::to_string(lastChannel) <<
					// ")" << TLOG_ENDL;
					if(chan < lastChannel)
					{
						TLOG_TRACE("WireChamberPythonOutput")
						    << "Bad data detected, moving on to next event" << TLOG_ENDL;
						evtPtr =
						    reinterpret_cast<WireChamberFragment::TDCEvent const*>(&data);
						break;
					}
					lastChannel = chan;

					unsigned int datatdc = data.TimeBits;

					*outputFiles_[fragment_id] << "Channel  " << std::setw(8) << (int)chan
					                           << "    " << std::setw(8) << (int)datatdc
					                           << std::endl;
				}

				// TLOG_DEBUG("WireChamberPythonOutput") << "Getting next event" <<
				// TLOG_ENDL;
				evtPtr = wcPtr->nextEvent(evtPtr);
			}
		}
	}
	TLOG_TRACE("WireChamberPythonOutput") << "DONE ANALYZING" << TLOG_ENDL;
}

void ots::WireChamberPythonOutput::beginRun(art::Run const& e)
{
	if(e.run() == current_run_)
		return;
	current_run_ = e.run();

	std::time_t t = std::time(nullptr);
	char        mbstr[15];
	std::strftime(mbstr, sizeof(mbstr), "%Y%m%d%H%M%S", std::localtime(&t));
	timeString_ = std::string(mbstr);
	eventCounters_.clear();
	outputFiles_.clear();
}

void ots::WireChamberPythonOutput::endRun(art::Run const& e)
{
	for(auto& file : outputFiles_)
	{
		file.second->close();
	}
	outputFiles_.clear();
}

DEFINE_ART_MODULE(ots::WireChamberPythonOutput)
