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
#include "otsdaq-trigger/DataDecoders/FSSRData.h"
#include "otsdaq-trigger/Overlays/FragmentType.hh"
#include "otsdaq-trigger/Overlays/STIBFragment.hh"

#include "cetlib_except/exception.h"

#include <TAxis.h>
#include <TBufferFile.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TGraph.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TRootCanvas.h>
#include <TStyle.h>

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

#include <map>
#include <queue>
#include <string>

#include <iostream>
#include <sstream>
#include <string>

#include <TCanvas.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TFrame.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH2.h>
#include <TH2F.h>
#include <TProfile.h>
#include <TROOT.h>
#include <TRandom.h>
#include <TThread.h>

#include <stdint.h>

namespace ots
{
class STIBDQM : public art::EDAnalyzer
{
  public:
	STIBDQM(fhicl::ParameterSet const& p);
	virtual ~STIBDQM() = default;

	void analyze(art::Event const& e) override;
	void beginRun(art::Run const&) override;

	void book();
	void bookPlaneHistogram(artdaq::Fragment::fragment_id_t fragId, unsigned int channel);
	void clear();
	void fill(artdaq::Fragment::fragment_id_t fragId,
	          std::unique_ptr<STIBFragment>&  frag);
	void write();

  protected:
	art::RunNumber_t current_run_;
	std::string      outputFileName_;
	TFile*           fFile_;
	bool             writeOutput_;

	TDirectory* planesDir_;

	std::map<artdaq::Fragment::fragment_id_t, std::map<unsigned int, TH1*>>
	      planeOccupancies_;
	TH1I* numberOfTriggers_;
};

//========================================================================================================================
STIBDQM::STIBDQM(fhicl::ParameterSet const& ps)
    : art::EDAnalyzer(ps)
    , current_run_(0)
    , outputFileName_(ps.get<std::string>("fileName", "otsdaqdemo_onmon.root"))
    , writeOutput_(ps.get<bool>("write_to_file", false))
{
}

void STIBDQM::bookPlaneHistogram(artdaq::Fragment::fragment_id_t fragId,
                                 unsigned int                    channel)
{
	std::stringstream name;
	std::stringstream title;
	name.str("");
	title.str("");
	name << "Plane_FE" << fragId << "_Channel" << channel << "_Occupancy";
	title << "Plane FE" << fragId << " Channel" << channel << " Occupancy";
	planeOccupancies_[fragId][channel] =
	    new TH1F(name.str().c_str(), title.str().c_str(), 640, -0.5, 639.5);
}

//========================================================================================================================
void STIBDQM::book()
{
	clear();
	TLOG_DEBUG("STIBDQM") << "Booking start!" << TLOG_ENDL;

	auto min =
	    static_cast<double>(std::numeric_limits<artdaq::Fragment::fragment_id_t>::min());
	auto max =
	    static_cast<double>(std::numeric_limits<artdaq::Fragment::fragment_id_t>::max());

	numberOfTriggers_ =
	    new TH1I("NumberOfTriggers", "Number of triggers", max, min, max + 1);
	planesDir_ = fFile_->mkdir("Planes", "Planes");
	planesDir_->cd();

	TLOG_DEBUG("STIBDQM") << "Booking done!" << TLOG_ENDL;
}

//========================================================================================================================
void STIBDQM::clear()
{
	numberOfTriggers_ = 0;
	planeOccupancies_.clear();
}

//========================================================================================================================
void STIBDQM::fill(artdaq::Fragment::fragment_id_t fragId,
                   std::unique_ptr<STIBFragment>&  frag)
{
	if(!writeOutput_)
		return;
	ots::FSSRData data;
	numberOfTriggers_->Fill(fragId);
	for(auto it = frag->dataBegin(); it != nullptr && it != frag->dataEnd();
	    frag->nextEvent(it))
	{
		for(size_t dat = 0; dat < it->event_data_count; ++dat)
		{
			auto dataPt = *(frag->eventDataUint(it) + dat);
			if(data.isFSSR(dataPt))
			{
				data.decode(dataPt);

				if(!planeOccupancies_.count(fragId))
				{
					if(!planeOccupancies_[fragId].count(data.getChannelNumber()))
					{
						bookPlaneHistogram(fragId, data.getChannelNumber());
					}
				}

				planeOccupancies_[fragId][data.getChannelNumber()]->Fill(
				    data.getSensorStrip());
			}
		}
	}
}

void STIBDQM::analyze(art::Event const& e)
{
	TLOG_INFO("STIBDQM") << "STIBDQM Analyzing event " << e.event() << TLOG_ENDL;

	// John F., 1/22/14 -- there's probably a more elegant way of
	// collecting fragments of various types using ART interface code;
	// will investigate. Right now, we're actually re-creating the
	// fragments locally

	artdaq::Fragments        fragments;
	artdaq::FragmentPtrs     containerFragments;
	std::vector<std::string> fragment_type_labels{"STIB", "Container", "ContainerSTIB"};

	TLOG_INFO("STIBDQM") << "STIBDQM Extracting Fragments" << TLOG_ENDL;
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

		if(label == "Container" || label == "ContainerSTIB")
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
	TLOG_INFO("STIBDQM") << "STIBDQM: This event has " << fragments.size()
	                     << " STIB Fragments" << TLOG_ENDL;

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
		// Pointers to the types of fragment overlays STIBDQM can handle;
		// only one will be used per fragment, of course

		std::unique_ptr<STIBFragment> wcPtr;

		//  const auto& frag( fragments[i] );  // Basically a shorthand

		//    if (i == 0)
		if(expected_sequence_id ==
		   std::numeric_limits<artdaq::Fragment::sequence_id_t>::max())
		{
			expected_sequence_id = frag.sequenceID();
		}

		if(expected_sequence_id != frag.sequenceID())
		{
			TLOG_WARNING("STIBDQM")
			    << "Warning in STIBDQM: expected fragment with sequence ID "
			    << expected_sequence_id << ", received one with sequence ID "
			    << frag.sequenceID() << TLOG_ENDL;
		}

		FragmentType fragtype = static_cast<FragmentType>(frag.type());
		// std::cout << __COUT_HDR_FL__ << "STIBDQM: Fragment type is " << fragtype << "
		// (DataGen=" << FragmentType::DataGen << ")" << TLOG_ENDL;
		// John F., 1/22/14 -- this should definitely be improved; I'm
		// just using the max # of bits per ADC value for a given fragment
		// type as is currently defined for the V172x fragments (as
		// opposed to the Toy fragment, which have this value in their
		// metadata). Since it's not using external variables for this
		// quantity, this would need to be edited should these values
		// change.

		switch(fragtype)
		{
		case FragmentType::STIB:
			wcPtr.reset(new STIBFragment(frag));
			break;
		default:
			throw cet::exception("Error in STIBDQM: unknown fragment type supplied: " +
			                     fragmentTypeToString(fragtype));
		}

		artdaq::Fragment::fragment_id_t fragment_id = frag.fragmentID();

		// For every event, fill the histogram (prescale is ignored here)

		// Is there some way to templatize an ART module? If not, we're
		// stuck with this switch code...

		TLOG_INFO("STIBDQM") << "STIBDQM: Filling histograms" << TLOG_ENDL;
		fill(fragment_id, wcPtr);

		// Draw the histogram
		TLOG_INFO("STIBDQM") << "STIBDQM: Writing data" << TLOG_ENDL;
		write();
	}
	TLOG_INFO("STIBDQM") << "DONE ANALYZING" << TLOG_ENDL;
}

void STIBDQM::write()
{
	if(writeOutput_)
	{
		planesDir_->cd();
		for(auto& fragIt : planeOccupancies_)
		{
			for(auto& channelIt : fragIt.second)
			{
				channelIt.second->Write(0, TObject::kOverwrite);
			}
		}
		fFile_->cd();
		numberOfTriggers_->Write();
		fFile_->Write();
		fFile_->Flush();
	}
}

void STIBDQM::beginRun(art::Run const& e)
{
	if(e.run() == current_run_)
		return;
	current_run_ = e.run();

	if(writeOutput_)
	{
		size_t pos = std::string::npos;
		while(outputFileName_.find("%n") != std::string::npos)
		{
			pos             = outputFileName_.find("%n");
			outputFileName_ = outputFileName_.replace(pos, 2, std::to_string(my_rank));
		}
		while(outputFileName_.find("%r") != std::string::npos)
		{
			pos             = outputFileName_.find("%r");
			outputFileName_ = outputFileName_.replace(pos, 2, std::to_string(e.run()));
		}

		fFile_ = new TFile(outputFileName_.c_str(), "RECREATE");
		fFile_->cd();
		book();
	}
}
}
DEFINE_ART_MODULE(ots::STIBDQM)
