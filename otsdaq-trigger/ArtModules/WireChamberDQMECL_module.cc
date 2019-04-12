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

#include "otsdaq-utilities/ECLWriter/ECLConnection.h"
#include "otsdaq-utilities/ECLWriter/ECLSupervisor.h"

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
#include <string>
#include <unordered_map>
#include <vector>

#define MAX_CHAMBERS 4
#define MAX_MODULES 4
#define MAX_WIRES 128
#define MAX_HITS 1280

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "WireChamberDQMECL"

namespace ots
{
class WireChamberDQMECL : public art::EDAnalyzer
{
  public:
	explicit WireChamberDQMECL(fhicl::ParameterSet const& p);
	virtual ~WireChamberDQMECL() = default;

	void analyze(art::Event const& e) override;
	void beginRun(art::Run const&) override;

  private:
	struct DQMHistos
	{
		TH2F*    h2_profile_[MAX_CHAMBERS];
		TH1F*    h_time_[MAX_CHAMBERS];
		TH1F*    h_timediff_[MAX_CHAMBERS];
		TH1F*    h_tdc_[MAX_CHAMBERS][MAX_MODULES];
		TH1I*    om_hits_;
		TH1F*    om_tdcs_;
		TH1F*    om_times_;
		TCanvas* profiles_canvas_;
		TCanvas* times_canvas_;
		TCanvas* timediffs_canvas_;
		TCanvas* tdcs_canvas_;
		TCanvas* om_canvas_;
	};
	DQMHistos aggregate_histos_;
	bool      initialized_;
	bool      reset_;

	int accumulateEvents_;  // events to accumulate before writing to ECL
	int eventCount;

	std::string eclUserName_;
	std::string eclURL_;
	std::string eclPassword_;

	art::RunNumber_t current_run_;

	std::string outputFileName_;
	bool        writeOutput_;

	void book(std::string name);
	void bookCanvas(std::string name,
	                DQMHistos&  container,
	                std::string keySuffix   = "",
	                std::string titleSuffix = "");
	void bookHistos(std::string name,
	                DQMHistos&  container,
	                std::string keySuffix   = "",
	                std::string titleSuffix = "");
	void calcXY(size_t t, size_t& x, size_t& y);
	void fill(std::unique_ptr<WireChamberFragment>& frag, int event);
	void write(int event, int run, DQMHistos& container, bool isSpill = true);
};
}

ots::WireChamberDQMECL::WireChamberDQMECL(fhicl::ParameterSet const& ps)
    : art::EDAnalyzer(ps)
    , initialized_(false)
    , reset_(false)
    , accumulateEvents_(ps.get<int>("accumlateEvents", 10))
    , eclUserName_(ps.get<std::string>("eclUserName", "otsdaq"))
    , eclURL_(ps.get<std::string>("eclURL", "https://dbweb6.fnal.gov:8443/ECL/test_beam"))
    , eclPassword_(ps.get<std::string>("eclPassword", "mtest2018"))
    , current_run_(0)
    , writeOutput_(ps.get<bool>("write_to_ecl", true))
{
	TLOG_INFO("WireChamberDQMECL")
	    << "WireChamberDQMECL CONSTRUCTOR BEGIN!!!!" << TLOG_ENDL;

	gStyle->SetOptStat("irm");
	gStyle->SetMarkerStyle(22);
	gStyle->SetMarkerColor(4);

	eventCount = 0;

	TLOG_DEBUG("WireChamberDQMECL") << "WireChamberDQMECL CONSTRUCTOR END" << TLOG_ENDL;
}

void ots::WireChamberDQMECL::analyze(art::Event const& e)
{
	TLOG_INFO("WireChamberDQMECL")
	    << "WireChamberDQMECL Analyzing event " << e.event() << TLOG_ENDL;

	// John F., 1/22/14 -- there's probably a more elegant way of
	// collecting fragments of various types using ART interface code;
	// will investigate. Right now, we're actually re-creating the
	// fragments locally

	artdaq::Fragments        fragments;
	artdaq::FragmentPtrs     containerFragments;
	std::vector<std::string> fragment_type_labels{"MWPC", "ContainerMWPC"};

	TLOG_INFO("WireChamberDQMECL")
	    << "WireChamberDQMECL Extracting Fragments" << TLOG_ENDL;
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

		if(label == "ContainerMWPC")
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
	TLOG_INFO("WireChamberDQMECL") << "WireChamberDQMECL: This event has "
	                               << fragments.size() << " MWPC Fragments" << TLOG_ENDL;

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
		// Pointers to the types of fragment overlays WireChamberDQM can handle;
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
			TLOG_WARNING("WireChamberDQMECL")
			    << "Warning in WireChamberDQMECL: expected fragment with sequence ID "
			    << expected_sequence_id << ", received one with sequence ID "
			    << frag.sequenceID() << TLOG_ENDL;
		}

		FragmentType fragtype = static_cast<FragmentType>(frag.type());
		// TLOG_DEBUG("WireChamberDQM") << "WireChamberDQM: Fragment type is " << fragtype
		// << " (DataGen=" << FragmentType::DataGen << ")" << TLOG_ENDL;
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
			    "Error in WireChamberDQMECL: unknown fragment type supplied: " +
			    fragmentTypeToString(fragtype));
		}

		artdaq::Fragment::fragment_id_t fragment_id = frag.fragmentID();

		TLOG_INFO("WireChamberDQMECL")
		    << "WireChamberDQMECL: Booking histograms" << TLOG_ENDL;
		std::string name = "MWPCFragment-";
		book(name + frag.metadata<WireChamberFragment::WireChamberMetadata>()
		                ->WireChamberHostname);

		// For every event, fill the histogram (prescale is ignored here)

		// Is there some way to templatize an ART module? If not, we're
		// stuck with this switch code...

		switch(fragtype)
		{
		case FragmentType::MWPC:
			// for (auto val = drPtr->dataBegin(); val <= drPtr->dataEnd(); ++val )
			{
				TLOG_INFO("WireChamberDQMECL")
				    << "WireChamberDQMECL: Filling histograms" << TLOG_ENDL;
				fill(wcPtr, e.event());
				eventCount++;
			}
			break;

		default:
			throw cet::exception(
			    "Error in WireChamberDQMECL: unknown fragment type supplied: " +
			    fragmentTypeToString(fragtype));
		}

		// Draw the histogram
		// write(e.event(), e.run(), spill_histos_);
		if(eventCount == accumulateEvents_)
		{
			TLOG_INFO("WireChamberDQMECL")
			    << "WireChamberDQMECL: Writing data from last " << accumulateEvents_
			    << " wire chamber events" << TLOG_ENDL;

			write(e.event(), e.run(), aggregate_histos_, false);
			eventCount = 0;
			reset_     = true;
		}
	}
	TLOG_INFO("WireChamberDQMECL") << "DONE ANALYZING" << TLOG_ENDL;
}

void ots::WireChamberDQMECL::calcXY(size_t t, size_t& x, size_t& y)
{
	switch(t)
	{
	case 1:
		x = y = 1;
		break;
	case 2:
		x = 2;
		y = 1;
		break;
	case 3:
	case 4:
		x = 2;
		y = 2;
		break;
	case 5:
	case 6:
		x = 3;
		y = 2;
		break;
	case 7:
	case 8:
		x = 4;
		y = 2;
		break;
	default:
		x = y = static_cast<std::size_t>(ceil(sqrt(t)));
	}
}

void ots::WireChamberDQMECL::bookCanvas(std::string name,
                                            DQMHistos&  container,
                                            std::string keySuffix,
                                            std::string titleSuffix)
{
	// Setup Profile Canvas
	container.profiles_canvas_ =
	    new TCanvas((name + "-Profiles-ECL" + keySuffix).c_str());
	size_t x, y;
	calcXY(MAX_CHAMBERS, x, y);
	TLOG_DEBUG("WireChamberDQMECL") << "Dividing Profile canvas into " << x << " x by "
	                                << y << " y plots." << TLOG_ENDL;
	container.profiles_canvas_->SetFixedAspectRatio();
	container.profiles_canvas_->Divide(x, y);
	container.profiles_canvas_->Update();
	//((TRootCanvas*)container.profiles_canvas_->GetCanvasImp())->DontCallClose();
	container.profiles_canvas_->SetTitle(
	    ("Wire Chamber Hit Profiles" + titleSuffix).c_str());

	// Setup TDC Canvas
	container.tdcs_canvas_ = new TCanvas((name + "-TDCs-ECL" + keySuffix).c_str());

	TLOG_DEBUG("WireChamberDQMECL")
	    << "Dividing TDC canvas into " << MAX_MODULES << " x by " << MAX_CHAMBERS
	    << " y plots." << TLOG_ENDL;
	container.tdcs_canvas_->Divide(MAX_MODULES, MAX_CHAMBERS);
	container.tdcs_canvas_->Update();
	//((TRootCanvas*)container.tdcs_canvas_->GetCanvasImp())->DontCallClose();
	container.tdcs_canvas_->SetTitle(("Wire Chamber Hits" + titleSuffix).c_str());

	// Setup Time Canvas
	container.times_canvas_ =
	    new TCanvas((name + "-EventTiming-ECL" + keySuffix).c_str());
	calcXY(MAX_CHAMBERS, x, y);
	TLOG_DEBUG("WireChamberDQMECL")
	    << "Dividing Time canvas into " << x << " x by " << y << " y plots." << TLOG_ENDL;
	container.times_canvas_->Divide(x, y);
	container.times_canvas_->Update();
	//((TRootCanvas*)container.times_canvas_->GetCanvasImp())->DontCallClose();
	container.times_canvas_->SetTitle(
	    ("Wire Chamber Event Timing" + titleSuffix).c_str());

	// Setup Timediff Canvas
	container.timediffs_canvas_ =
	    new TCanvas((name + "-EventTimeDiff-ECL" + keySuffix).c_str());
	calcXY(MAX_CHAMBERS, x, y);
	TLOG_DEBUG("WireChamberDQMECL") << "Dividing Timediff canvas into " << x << " x by "
	                                << y << " y plots." << TLOG_ENDL;
	container.timediffs_canvas_->Divide(x, y);
	container.timediffs_canvas_->Update();
	//((TRootCanvas*)container.timediffs_canvas_->GetCanvasImp())->DontCallClose();
	container.timediffs_canvas_->SetTitle(
	    ("Wire Chamber Event Time Differences" + titleSuffix).c_str());

	// Setup Online Monitoring Canvas
	container.om_canvas_ = new TCanvas((name + "-OnlineMonitor-ECL" + keySuffix).c_str());
	TLOG_DEBUG("WireChamberDQMECL")
	    << "Dividing Online Monitoring canvas into 2 y plots." << TLOG_ENDL;
	container.om_canvas_->Divide(1, 2);
	auto lowerPad = container.om_canvas_->cd(2);
	TLOG_DEBUG("WireChamberDQMECL")
	    << "Dividing lower Online Monitoring canvas into 2 x plots" << TLOG_ENDL;
	lowerPad->Divide(2, 1);
	lowerPad->Update();
	container.om_canvas_->Update();
	//((TRootCanvas*)container.om_canvas_->GetCanvasImp())->DontCallClose();
	container.om_canvas_->SetTitle(("Wire Chamber Online Monitor" + titleSuffix).c_str());
}

void ots::WireChamberDQMECL::bookHistos(std::string name,
                                            DQMHistos&  container,
                                            std::string keySuffix,
                                            std::string titleSuffix)
{
	std::stringstream ss;
	for(unsigned int ichamber = 0; ichamber < MAX_CHAMBERS; ichamber++)
	{
		ss.str("");
		ss << "MWPC" << ichamber + 1;

		auto profiles_pad = container.profiles_canvas_->cd(ichamber + 1);
		profiles_pad->SetFixedAspectRatio();
		container.h2_profile_[ichamber] = new TH2F(
		    ss.str().c_str(), ss.str().c_str(), 128, -0.5, 127.5, 128, -127.5, 0.5);
		container.h2_profile_[ichamber]->SetXTitle("x (mm)");
		container.h2_profile_[ichamber]->SetYTitle("y (mm)");
		container.h2_profile_[ichamber]->Draw("colz");
		container.h2_profile_[ichamber] =
		    (TH2F*)container.profiles_canvas_->GetPad(ichamber + 1)
		        ->GetPrimitive(ss.str().c_str());

		container.times_canvas_->cd(ichamber + 1);
		container.h_time_[ichamber] =
		    new TH1F(ss.str().c_str(), ss.str().c_str(), 600, 0.0, 6.0);
		container.h_time_[ichamber]->SetXTitle("Time (s)");
		container.h_time_[ichamber]->Draw();
		container.h_time_[ichamber] = (TH1F*)container.times_canvas_->GetPad(ichamber + 1)
		                                  ->GetPrimitive(ss.str().c_str());

		auto pad = container.timediffs_canvas_->cd(ichamber + 1);
		pad->SetLogx();
		pad->SetLogy();
		container.h_timediff_[ichamber] =
		    new TH1F(ss.str().c_str(), ss.str().c_str(), 250, 0.0, 0.025);
		container.h_timediff_[ichamber]->SetXTitle("Time (s)");
		container.h_timediff_[ichamber]->Draw();
		container.h_timediff_[ichamber] =
		    (TH1F*)container.timediffs_canvas_->GetPad(ichamber + 1)
		        ->GetPrimitive(ss.str().c_str());
	}

	for(unsigned int ichamber = 0; ichamber < MAX_CHAMBERS; ichamber++)
	{
		for(unsigned int imod = 0; imod < MAX_MODULES; imod++)
		{
			container.tdcs_canvas_->cd(ichamber * MAX_MODULES + imod + 1);
			ss.str("");
			ss << "TDC" << ichamber * 4 + imod + 1;
			container.h_tdc_[ichamber][imod] =
			    new TH1F(ss.str().c_str(), ss.str().c_str(), 500, -0.5, 499.5);
			container.h_tdc_[ichamber][imod]->SetXTitle("TDC Counts (9.4 ns)");
			container.h_tdc_[ichamber][imod]->Draw();
			container.h_tdc_[ichamber][imod] =
			    (TH1F*)container.tdcs_canvas_->GetPad(ichamber * MAX_MODULES + imod + 1)
			        ->GetPrimitive(ss.str().c_str());
		}
	}

	container.om_canvas_->cd(1);
	container.om_hits_ = new TH1I("HitProfiles", "Hit Profiles", 16 * 64, 0, 16 * 64 + 1);
	auto lowerPad      = container.om_canvas_->cd(2);
	lowerPad->cd(1);
	container.om_tdcs_ = new TH1F("TDCs", "TDCs", 500, -0.5, 499.5);
	lowerPad->cd(2);
	container.om_times_ = new TH1F("HitTimes", "Hit Times", 600, 0.0, 6.0);
	lowerPad->Modified();
	lowerPad->Update();

	container.profiles_canvas_->Modified();
	container.profiles_canvas_->Update();
	container.tdcs_canvas_->Modified();
	container.tdcs_canvas_->Update();
	container.times_canvas_->Modified();
	container.times_canvas_->Update();
	container.timediffs_canvas_->Modified();
	container.timediffs_canvas_->Update();
	container.om_canvas_->Modified();
	container.om_canvas_->Update();
}

void ots::WireChamberDQMECL::book(std::string name)
{
	if(!initialized_)
	{
		// book ansemble
		bookCanvas(name, aggregate_histos_, "-AllSpills", " - All Spills");
		bookHistos(name, aggregate_histos_, "-AllSpills", " - All Spills");

		initialized_ = true;
	}
	if(reset_)
	{
		bookHistos(name, aggregate_histos_, "-AllSpills", " - All Spills");
		reset_ = false;
	}
}

void ots::WireChamberDQMECL::fill(std::unique_ptr<WireChamberFragment>& frag,
                                      int                                   event)
{
	unsigned int nhit_x = 0;
	unsigned int wire_x[MAX_HITS];
	unsigned int time_x[MAX_HITS];

	unsigned int nhit_y = 0;
	unsigned int wire_y[MAX_HITS];
	unsigned int time_y[MAX_HITS];

	unsigned int chamber;
	unsigned int lastChamber         = 0;
	unsigned int chamberModuleNumber = 0;  // module number within chamber, 0-3
	bool         xory;                     // 0 = x-wire, 1 = y-wire
	bool         first_or_second;
	unsigned int eventTime = 0;
	unsigned int lastEventTime[MAX_CHAMBERS];
	bool         firstEvent[MAX_CHAMBERS];

	for(int ch = 0; ch < MAX_CHAMBERS; ++ch)
	{
		lastEventTime[ch] = 0;
		firstEvent[ch]    = true;
	}

	TLOG_DEBUG("WireChamberDQMECL") << "Starting event processing loop" << TLOG_ENDL;
	auto evtPtr = frag->dataBegin();
	while(evtPtr != nullptr)
	{
		chamberModuleNumber =
		    (evtPtr->TDCNumber - 1) % 4;  // module number within chamber, 0-3
		chamber = (evtPtr->TDCNumber - 1) / 4;

		if(chamberModuleNumber >= MAX_MODULES || chamber >= MAX_CHAMBERS)
		{
			TLOG_DEBUG("WireChamberDQMECL")
			    << "Bad data detected, moving on to next event" << TLOG_ENDL;
			evtPtr = frag->nextEvent(evtPtr);
			continue;
		}

		xory            = (chamberModuleNumber) / 2;  // 0 = x-wire, 1 = y-wire
		first_or_second = (chamberModuleNumber) % 2;

		if(chamber != lastChamber)
		{
			// TLOG_DEBUG("WireChamberDQM") << "NumHitsX: " << nhit_x << " NumHitsY: " <<
			// nhit_y << TLOG_ENDL;

			int onlygoodhits = 0;
			for(unsigned int ix = 0; ix < nhit_x; ix++)
			{
				float xpos = wire_x[ix];
				int   xtdc = time_x[ix];
				int   xmod = xpos / 64;

				// Use only hits within first peak
				if(onlygoodhits)  // && xtdc > tdcmean[xmod]+2.0*tdcsigma[xmod] )
				{
					continue;
				}

				for(unsigned int iy = 0; iy < nhit_y; iy++)
				{
					int ywire = wire_y[iy];
					// invert ywire to geometric position
					float ypos = -ywire;
					int   ytdc = time_y[iy];
					int   ymod = 2 + ywire / 64;

					// Use only hits within first peak
					if(onlygoodhits)  //&& ytdc > tdcmean[ymod]+2.0*tdcsigma[ymod] )
					{
						continue;
					}
					//__MOUT__ << "MWPC \t xpos " << xpos << ", ypos " << ypos <<
					// TLOG_ENDL;
					aggregate_histos_.h2_profile_[lastChamber]->Fill(xpos, ypos);
				}
			}
			nhit_x = 0;
			nhit_y = 0;
		}
		lastChamber = chamber;

		size_t dataSz;
		auto   eventData = frag->eventData(evtPtr, dataSz);

		// Check against unreasonable sizes
		if(dataSz > frag->sizeBytes())
		{
			evtPtr = nullptr;
			break;
		}
		eventTime = frag->TDCEventTimestamp(evtPtr) & 0xFFFFFC00;

		if(firstEvent[chamber])
		{
			firstEvent[chamber]    = false;
			lastEventTime[chamber] = eventTime;
		}
		else
		{
			auto evtTimeDiff       = (eventTime - lastEventTime[chamber]) * 9.14E-9;
			lastEventTime[chamber] = eventTime;
			TLOG_TRACE("WireChamberDQMECL")
			    << "Not first event, filling event time diff: " << evtTimeDiff
			    << TLOG_ENDL;
			aggregate_histos_.h_timediff_[chamber]->Fill(evtTimeDiff);
		}

		// TLOG_DEBUG("WireChamberDQM") << "Processing " << std::to_string(dataSz) << "
		// hits in chamber " << std::to_string(chamber) << " module " <<
		// std::to_string(chamberModuleNumber) << " trigger number " <<
		// std::to_string(frag->TDCEventTriggerCounter(evtPtr)) << TLOG_ENDL;
		unsigned lastChannel = 0;

		for(size_t ii = 0; ii < dataSz; ++ii)
		{
			auto         data = eventData[ii];
			unsigned int chan = data.ChannelNumber;

			// TLOG_DEBUG("WireChamberDQM") << "Channel number " << std::to_string(chan)
			// << " (last " << std::to_string(lastChannel) << ")" << TLOG_ENDL;
			if(chan < lastChannel)
			{
				TLOG_DEBUG("WireChamberDQMECL")
				    << "Bad data detected, moving on to next event" << TLOG_ENDL;
				evtPtr = reinterpret_cast<WireChamberFragment::TDCEvent const*>(&data);
				break;
			}
			lastChannel = chan;

			unsigned int tdc = data.TimeBits;

			auto tdcEventTime = (eventTime + tdc) * 9.14E-9;
			aggregate_histos_.h_time_[chamber]->Fill(tdcEventTime);
			aggregate_histos_.om_times_->Fill(tdcEventTime);

			// Fill tdc hit distribution
			aggregate_histos_.h_tdc_[chamber][chamberModuleNumber]->Fill(tdc);
			aggregate_histos_.om_tdcs_->Fill(tdc);
			aggregate_histos_.om_hits_->Fill(chan + 64 * (evtPtr->TDCNumber - 1));

			if(xory == 0)  // x-wire
			{
				if(nhit_x < MAX_HITS - 1)
				{
					wire_x[nhit_x] = chan + 64 * first_or_second;
					time_x[nhit_x] = tdc;
					nhit_x++;
				}
				else
				{
					TLOG_DEBUG("WireChamberDQMECL")
					    << "Too many x-hits, mwpc " << evtPtr->TDCNumber << ", " << nhit_x
					    << " hits!" << TLOG_ENDL;
				}
			}
			else if(xory == 1)
			{
				if(nhit_y < MAX_HITS - 1)
				{
					wire_y[nhit_y] = chan + 64 * first_or_second;
					time_y[nhit_y] = tdc;
					nhit_y++;
				}
				else
				{
					TLOG_DEBUG("WireChamberDQMECL")
					    << "Too many y-hits, mwpc " << evtPtr->TDCNumber << ", " << nhit_y
					    << " hits!" << TLOG_ENDL;
				}
			}
		}

		// TLOG_DEBUG("WireChamberDQM") << "Getting next event" << TLOG_ENDL;
		evtPtr = frag->nextEvent(evtPtr);
	}
	TLOG_DEBUG("WireChamberDQMECL") << "Done processing data" << TLOG_ENDL;
}

void ots::WireChamberDQMECL::write(int        event,
                                       int        run,
                                       DQMHistos& container,
                                       bool       isSpill)
{
	for(unsigned int ichamber = 0; ichamber < MAX_CHAMBERS; ichamber++)
	{
		container.profiles_canvas_->cd(ichamber + 1);
		container.h2_profile_[ichamber]->Draw("colz");
		if(isSpill)
		{
			container.profiles_canvas_->SetTitle(
			    ("Wire Chamber Hit Profiles - Spill " + std::to_string(event)).c_str());
		}
		container.profiles_canvas_->Modified();
		container.profiles_canvas_->Update();

		for(unsigned int imod = 0; imod < MAX_MODULES; imod++)
		{
			container.tdcs_canvas_->cd(ichamber * MAX_MODULES + imod + 1);
			container.h_tdc_[ichamber][imod]->Draw();
			if(isSpill)
			{
				container.tdcs_canvas_->SetTitle(
				    ("Wire Chamber Hits - Spill " + std::to_string(event)).c_str());
			}
			container.tdcs_canvas_->Modified();
			container.tdcs_canvas_->Update();
		}

		container.times_canvas_->cd(ichamber + 1);
		container.h_time_[ichamber]->Draw();
		if(isSpill)
		{
			container.times_canvas_->SetTitle(
			    ("Wire Chamber Hit Timing - Spill " + std::to_string(event)).c_str());
		}
		container.times_canvas_->Modified();
		container.times_canvas_->Update();

		container.timediffs_canvas_->cd(ichamber + 1);
		container.h_timediff_[ichamber]->Draw();
		if(isSpill)
		{
			container.timediffs_canvas_->SetTitle(
			    ("Wire Chamber Event Time Difference - Spill " + std::to_string(event))
			        .c_str());
		}
		container.timediffs_canvas_->Modified();
		container.timediffs_canvas_->Update();
	}
	container.om_canvas_->cd(1);
	container.om_hits_->Draw();
	auto lowerPad = container.om_canvas_->cd(2);
	lowerPad->cd(1);
	container.om_tdcs_->Draw();
	lowerPad->cd(2);
	container.om_times_->Draw();
	if(isSpill)
	{
		container.om_hits_->SetTitle(
		    ("Wire Chamber Online Monitoring - Spill " + std::to_string(event)).c_str());
	}
	lowerPad->Modified();
	lowerPad->Update();
	container.om_canvas_->Modified();
	container.om_canvas_->Update();

	if(writeOutput_)
	{
		container.profiles_canvas_->Print("mwpc_profiles.png");
		container.tdcs_canvas_->Print("mwpc_tdcs.png");
		container.times_canvas_->Print("mwpc_times.png");
		container.timediffs_canvas_->Print("mwpc_timediffs.png");
		container.om_canvas_->Print("mwpc_om.png");

		ECLEntry_t eclEntry;
		eclEntry.author(eclUserName_);
		eclEntry.category("Facility/MWPC");
		Form_t                 form;
		Field_t                field;
		Form_t::field_sequence fields;
		form.name("OTSDAQ System Logbook Entry");

		field = Field_t(ECLConnection::EscapeECLString("Facility MWPC"), "Experminent");
		fields.push_back(field);

		field = Field_t(ECLConnection::EscapeECLString(std::to_string(run)), "RunNumber");
		fields.push_back(field);

		field = Field_t(ECLConnection::EscapeECLString("n/a"), "ActiveUsers");
		fields.push_back(field);

		std::string entry = "MWPC summary plots from past " +
		                    std::to_string(accumulateEvents_) + " beam spills";
		field = Field_t(ECLConnection::EscapeECLString(entry), "Entry");
		fields.push_back(field);

		form.field(fields);
		eclEntry.form(form);

		ECLEntry_t::attachment_sequence attachments;

		attachments.push_back(ECLConnection::MakeAttachmentImage("mwpc_profiles.png"));

		attachments.push_back(ECLConnection::MakeAttachmentImage("mwpc_tdcs.png"));

		attachments.push_back(ECLConnection::MakeAttachmentImage("mwpc_times.png"));

		attachments.push_back(ECLConnection::MakeAttachmentImage("mwpc_timediffs.png"));

		attachments.push_back(ECLConnection::MakeAttachmentImage("mwpc_om.png"));

		eclEntry.attachment(attachments);

		ECLConnection eclConn(eclUserName_, eclPassword_, eclURL_);
		if(!eclConn.Post(eclEntry))
		{
			TLOG_WARNING("WireChamberDQMECL")
			    << "Warning in WireChamberDQMECL: failed to post enty to ECL"
			    << TLOG_ENDL;
		}
	}
}

//************************************************************


void ots::WireChamberDQMECL::beginRun(art::Run const& e)
{
	if(e.run() == current_run_)
		return;
	current_run_ = e.run();
}

DEFINE_ART_MODULE(ots::WireChamberDQMECL)
