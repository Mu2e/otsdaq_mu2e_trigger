#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "canvas/Utilities/InputTag.h"

#include "artdaq-core/Data/ContainerFragment.hh"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/DAQdata/Globals.hh"

#include "otsdaq-trigger/Overlays/FragmentType.hh"
#include "otsdaq-trigger/Overlays/STIBFragment.hh"
#include "otsdaq-trigger/Overlays/WireChamberEventFragment.hh"

#include "cetlib_except/exception.h"

#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <unordered_map>
#include <vector>

#define MAX_CHAMBERS 4
#define MAX_MODULES 4
#define MAX_WIRES 128
#define MAX_HITS 1280

namespace ots
{
class TriggerCounter : public art::EDAnalyzer
{
  public:
	explicit TriggerCounter(fhicl::ParameterSet const& p);
	virtual ~TriggerCounter() = default;

	void analyze(art::Event const& e) override;
	void beginRun(art::Run const&) override;

	typedef std::pair<FragmentType, artdaq::Fragment::fragment_id_t> fragment_typeid;

  private:
	art::RunNumber_t current_run_;

	std::unordered_map<uint64_t, std::set<fragment_typeid>> triggers_;
	std::set<fragment_typeid>                               fragmentIDs_;
	std::map<fragment_typeid, uint64_t>                     triggerOffsets_;
	size_t                                                  expectedFragmentCount_;

	void AddTrigger(uint64_t trigNum, fragment_typeid fragId);
};
}

ots::TriggerCounter::TriggerCounter(fhicl::ParameterSet const& ps)
    : art::EDAnalyzer(ps)
{
	TLOG_INFO("TriggerCounter") << "TriggerCounter CONSTRUCTOR BEGIN!!!!" << TLOG_ENDL;
	TLOG_DEBUG("TriggerCounter") << "TriggerCounter CONSTRUCTOR END" << TLOG_ENDL;
}

void ots::TriggerCounter::AddTrigger(uint64_t trigNum, fragment_typeid fragId)
{
	if(!fragmentIDs_.count(fragId))
	{
		TLOG_INFO("TriggerCounter")
		    << "Fragment " << fragmentTypeToString(fragId.first) << " "
		    << std::to_string(fragId.second)
		    << ": Trigger Offset: " << std::to_string(trigNum) << TLOG_ENDL;
		triggerOffsets_[fragId] = trigNum;
		fragmentIDs_.insert(fragId);
	}
	auto offsetTrigNum = trigNum - triggerOffsets_[fragId];
	triggers_[offsetTrigNum].insert(fragId);
	if(triggers_[offsetTrigNum].size() > expectedFragmentCount_)
	{
		expectedFragmentCount_ = triggers_[offsetTrigNum].size();
	}
}

void ots::TriggerCounter::analyze(art::Event const& e)
{
	TLOG_INFO("TriggerCounter")
	    << "TriggerCounter Analyzing event " << e.event() << TLOG_ENDL;

	// John F., 1/22/14 -- there's probably a more elegant way of
	// collecting fragments of various types using ART interface code;
	// will investigate. Right now, we're actually re-creating the
	// fragments locally

	artdaq::Fragments        fragments;
	artdaq::FragmentPtrs     containerFragments;
	std::vector<std::string> fragment_type_labels{"MWPC",
	                                              "STIB",
	                                              "MWPCEVT",
	                                              "Container",
	                                              "ContainerMWPC",
	                                              "ContainerSTIB",
	                                              "ContainerMWPCEVT"};

	TLOG_DEBUG("TriggerCounter") << "TriggerCounter Extracting Fragments" << TLOG_ENDL;
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

		if(label == "Container" || label == "ContainerMWPCEVT" ||
		   label == "ContainerMWPC" || label == "ContainerSTIB")
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
	TLOG_DEBUG("TriggerCounter") << "TriggerCounter: This event has " << fragments.size()
	                             << " Fragments" << TLOG_ENDL;

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
	triggers_.clear();
	triggerOffsets_.clear();
	fragmentIDs_.clear();
	expectedFragmentCount_ = 0;

	//  for (std::size_t i = 0; i < fragments.size(); ++i) {
	for(const auto& frag : fragments)
	{
		// Pointers to the types of fragment overlays TriggerCounter can handle;
		// only one will be used per fragment, of course

		std::unique_ptr<STIBFragment>             sPtr;
		std::unique_ptr<WireChamberFragment>      wcPtr;
		std::unique_ptr<WireChamberEventFragment> wcePtr;

		//  const auto& frag( fragments[i] );  // Basically a shorthand

		//    if (i == 0)
		if(expected_sequence_id ==
		   std::numeric_limits<artdaq::Fragment::sequence_id_t>::max())
		{
			expected_sequence_id = frag.sequenceID();
		}

		if(expected_sequence_id != frag.sequenceID())
		{
			TLOG_WARNING("TriggerCounter")
			    << "Warning in TriggerCounter: expected fragment with sequence ID "
			    << expected_sequence_id << ", received one with sequence ID "
			    << frag.sequenceID() << TLOG_ENDL;
		}

		FragmentType                    fragtype = static_cast<FragmentType>(frag.type());
		artdaq::Fragment::fragment_id_t fragment_id = frag.fragmentID();
		auto                            fragId = std::make_pair(fragtype, fragment_id);

		switch(fragtype)
		{
		case FragmentType::MWPC:
		{
			wcPtr.reset(new WireChamberFragment(frag));

			unsigned int chamber;
			unsigned int chamberModuleNumber = 0;  // module number within chamber, 0-3

			auto evtPtr = wcPtr->dataBegin();
			while(evtPtr != nullptr)
			{
				chamberModuleNumber =
				    (evtPtr->TDCNumber - 1) % 4;  // module number within chamber, 0-3
				chamber = (evtPtr->TDCNumber - 1) / 4;

				if(chamberModuleNumber >= MAX_MODULES || chamber >= MAX_CHAMBERS)
				{
					TLOG_DEBUG("WireChamberDQM")
					    << "Bad data detected, moving on to next event" << TLOG_ENDL;
					evtPtr = wcPtr->nextEvent(evtPtr);
					continue;
				}

				auto trigNum = wcPtr->TDCEventTriggerCounter(evtPtr);
				AddTrigger(trigNum, fragId);

				size_t dataSz;
				auto   eventData = wcPtr->eventData(evtPtr, dataSz);

				// Check against unreasonable sizes
				if(dataSz > wcPtr->sizeBytes())
				{
					evtPtr = nullptr;
					break;
				}

				// TLOG_DEBUG("WireChamberDQM") << "Processing " << std::to_string(dataSz)
				// << " hits in chamber " << std::to_string(chamber) << " module " <<
				// std::to_string(chamberModuleNumber) << " trigger number " <<
				// std::to_string(frag->TDCEventTriggerCounter(evtPtr)) << TLOG_ENDL;
				unsigned lastChannel = 0;

				for(size_t ii = 0; ii < dataSz; ++ii)
				{
					auto         data = eventData[ii];
					unsigned int chan = data.ChannelNumber;

					// TLOG_DEBUG("WireChamberDQM") << "Channel number " <<
					// std::to_string(chan) << " (last " << std::to_string(lastChannel) <<
					// ")" << TLOG_ENDL;
					if(chan < lastChannel)
					{
						TLOG_DEBUG("WireChamberDQM")
						    << "Bad data detected, moving on to next event" << TLOG_ENDL;
						evtPtr =
						    reinterpret_cast<WireChamberFragment::TDCEvent const*>(&data);
						break;
					}
					lastChannel = chan;
				}

				// TLOG_DEBUG("WireChamberDQM") << "Getting next event" << TLOG_ENDL;
				evtPtr = wcPtr->nextEvent(evtPtr);
			}
		}
		break;
		case FragmentType::MWPCEVT:
		{
			wcePtr.reset(new WireChamberEventFragment(frag));
			auto trigNum = wcePtr->trigger_number();
			AddTrigger(trigNum, fragId);
		}
		break;
		case FragmentType::STIB:
		{
			sPtr.reset(new STIBFragment(frag));
			auto it = sPtr->dataBegin();
			while(it != nullptr && it != sPtr->dataEnd())
			{
				auto trigNum = it->trigger_counter;
				AddTrigger(trigNum, fragId);
				it = sPtr->nextEvent(it);
			}
		}
		break;
		default:
			throw cet::exception(
			    "Error in TriggerCounter: unknown fragment type supplied: " +
			    fragmentTypeToString(fragtype));
		}
	}

	TLOG_INFO("TriggerCounter") << "Event " << e.event() << " contains "
	                            << triggers_.size() << " triggers." << TLOG_ENDL;

	std::map<fragment_typeid, uint64_t> triggerCounts;

	std::ostringstream hdr;
	hdr << "TRIG #\t\t";
	for(auto& fid : fragmentIDs_)
	{
		hdr << fragmentTypeToString(fid.first) << " " << std::to_string(fid.second)
		    << "\t\t";
	}
	TLOG_INFO("TriggerCounter") << hdr.str() << TLOG_ENDL;

	for(auto& trig : triggers_)
	{
		if(trig.second.size() != expectedFragmentCount_)
		{
			std::ostringstream t;
			t << std::to_string(trig.first) << "\t\t";
			for(auto& fid : fragmentIDs_)
			{
				if(trig.second.count(fid))
				{
					triggerCounts[fid]++;
					t << "XXXX";
				}
				t << "\t\t";
			}
			TLOG_INFO("TriggerCounter") << t.str() << TLOG_ENDL;
		}
	}

	for(auto& fid : triggerCounts)
	{
		TLOG_INFO("TriggerCounter")
		    << "Fragment " << fragmentTypeToString(fid.first.first) << " "
		    << std::to_string(fid.first.second) << " had " << fid.second
		    << " triggers in this event" << TLOG_ENDL;
	}

	TLOG_DEBUG("TriggerCounter") << "DONE ANALYZING" << TLOG_ENDL;
}

void ots::TriggerCounter::beginRun(art::Run const& e)
{
	if(e.run() == current_run_)
		return;
	current_run_ = e.run();
}

DEFINE_ART_MODULE(ots::TriggerCounter)
