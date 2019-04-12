#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
//#define _LIBCPP_ENABLE_CXX17_REMOVED_FEATURES 1
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/girerr.hpp>
//#include <xmlrpc-c/registry.hpp>
//#include <xmlrpc-c/server_abyss.hpp>
#include <xmlrpc-c/client_simple.hpp>
//#undef _LIBCPP_ENABLE_CXX17_REMOVED_FEATURES
#pragma GCC diagnostic pop
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "canvas/Utilities/InputTag.h"

#include "artdaq-core/Data/ContainerFragment.hh"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/DAQdata/Globals.hh"

#include "otsdaq-utilities/ECLWriter/ECLConnection.h"
#include "otsdaq-utilities/ECLWriter/ECLSupervisor.h"

#include "otsdaq-core/NetworkUtilities/UDPDataStreamerBase.h"
#include "otsdaq-trigger/Overlays/FragmentType.hh"
#include "otsdaq-trigger/Overlays/WireChamberFragment.hh"

#include "cetlib_except/exception.h"

#include <TAxis.h>
#include <TBufferFile.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TGraph.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TF1.h>
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
#define __MF_SUBJECT__ "WireChamberDQM"

namespace ots
{
class WireChamberDQM : public art::EDAnalyzer
{
  public:
		explicit WireChamberDQM(fhicl::ParameterSet const & p);
		virtual ~WireChamberDQM() = default;

		void analyze(art::Event const & e) override;
		void beginRun(art::Run const &) override;

	private:
		struct DQMHistos {
			TH2F* h2_profile_[MAX_CHAMBERS];
			TH1F* h_time_[MAX_CHAMBERS];
			TH1F* h_timediff_[MAX_CHAMBERS];
			TH1F* h_tdc_[MAX_CHAMBERS][MAX_MODULES];
			TH1I* om_hits_;
			TH1F* om_tdcs_;
			TH1F* om_times_;
			TH1F* h_xslope_;
			TH1F* h_xintercept_;
			TH1F* h_yslope_;
			TH1F* h_yintercept_;
			TCanvas* profiles_canvas_;
			TCanvas* times_canvas_;
			TCanvas* timediffs_canvas_;
			TCanvas* tdcs_canvas_;
			TCanvas* om_canvas_;
			TCanvas* beampos_canvas_;
		};
		DQMHistos aggregate_histos_;
		DQMHistos spill_histos_;
		DQMHistos beamspot_histos_;
		bool initialized_;
		bool reset_beam_ = false;
		TFile* fFile_;

		art::RunNumber_t current_run_;

		std::string outputFileName_;
		bool writeOutput_;
		bool overwrite_mode_;

		bool writeToNetwork_;
		bool directoryForEachSpill_;
		bool aggregateHistosEnabled_;
		std::unique_ptr<ots::UDPDataStreamerBase> networkStreamer_;
                int  beamPosAggregate_;
		bool useThreePointTracks_;
		bool postBeamPosToECL_;
		bool logBeamPosToACNET_;
                std::string eclUserName_;
		std::string eclURL_;
		std::string eclPassword_;
                //Parameters for fitting tracks in the wire chambers to determine beam position
		// z coordinate of wire chamgers in cm.  0 is at the upstream wall of section 6.2a
		float zCoordWC1_;
		float zCoordWC2_;
		float zCoordWC3_;
		float zCoordWC4_;


		//position of wire 0 in X and Y for each chamber in cm.  Chambers read top to bottom (Y) and beam left to right (X)
		float xCoordWC1_;
		float xCoordWC2_;
		float xCoordWC3_;
		float xCoordWC4_;
		float yCoordWC1_;
		float yCoordWC2_;
		float yCoordWC3_;
		float yCoordWC4_;

		//maximum position deviation for a hit between first and last chamber
                float posDev_;

                //center of TDC peaks for each of 16 TDCs.  Since fits acan be fiddly these parameters represent current known values. If cables or triggering change significantly these will need to be re-derived.
		int meanTDC1_;
		int meanTDC2_;
	        int meanTDC3_;
		int meanTDC4_;
		int meanTDC5_;
		int meanTDC6_;
		int meanTDC7_;
		int meanTDC8_;
		int meanTDC9_;
		int meanTDC10_;
		int meanTDC11_;
		int meanTDC12_;
		int meanTDC13_;
		int meanTDC14_;
		int meanTDC15_;
		int meanTDC16_;

		//half width of allowed TDC window
		int tdcHalfWin_;

                float xcoords[4];
                float ycoords[4];
                float zcoords[4];
                float meantdcs[16];

		void book(std::string name);
		void bookCanvas(std::string name, 
		                DQMHistos& container, 
				std::string keySuffix = "", 
				std::string titleSuffix = "");
		void bookHistos(std::string name, 
		                DQMHistos& container, 
				std::string keySuffix = "", 
				std::string titleSuffix = "");
		void calcXY(size_t t, size_t& x, size_t& y);
		void fill(std::unique_ptr<WireChamberFragment>& frag, int event);
		void write(int event, int run, DQMHistos& container, bool isSpill = true, bool beamspot = false);
                int trackFit( int npts, float& a, float& b, float& chs, float x[], float z[] );
		void fourPlaneTrack( float www, int ixy, int nhit[4], float xx[10][4], int& ntk, float xs[5], float xi[5], float chsq[5], int knd[5], float tkhit[5][4], int itmtk[5][4]);
                void makeTracks(int nxyhit[2][4], float posxy[2][4][10]);
		void writeACNET(std::string name, float val);
};
}

ots::WireChamberDQM::WireChamberDQM(fhicl::ParameterSet const & ps)
	: art::EDAnalyzer(ps)
	, initialized_(false)
	, current_run_(0)
	, outputFileName_(ps.get<std::string>("fileName", "otsdaqdemo_onmon.root"))
	, writeOutput_(ps.get<bool>("write_to_file", false))
	, overwrite_mode_(ps.get<bool>("overwrite_output_file", true))
	, writeToNetwork_(ps.get<bool>("write_to_network", false))
	, directoryForEachSpill_(ps.get<bool>("spill_directories", true))
	, aggregateHistosEnabled_(ps.get<bool>("create_aggregate_histograms", true))
	, networkStreamer_(nullptr)
        , beamPosAggregate_(ps.get<int>("beamPosAggregate", 5))
        , useThreePointTracks_(ps.get<bool>("useThreePointTracks", true))
        , postBeamPosToECL_(ps.get<bool>("postBeamPosToECL",false))
        , logBeamPosToACNET_(ps.get<bool>("logBeamPosToACNET",true))
	, eclUserName_(ps.get<std::string>("eclUserName","otsdaq"))
	, eclURL_(ps.get<std::string>("eclURL","https://dbweb6.fnal.gov:8443/ECL/test_beam"))
	, eclPassword_(ps.get<std::string>("eclPassword","mtest2018"))
        , zCoordWC1_(ps.get<float>("zCoordWC1", 198.8))
	, zCoordWC2_(ps.get<float>("zCoordWC2", 474.1))
	, zCoordWC3_(ps.get<float>("zCoordWC3", 758.7))
	, zCoordWC4_(ps.get<float>("zCoordWC4", 1530.5))
	, xCoordWC1_(ps.get<float>("xCoordWC1", 6.362))
	, xCoordWC2_(ps.get<float>("xCoordWC2", 6.398))
	, xCoordWC3_(ps.get<float>("xCoordWC3", 6.407))
	, xCoordWC4_(ps.get<float>("xCoordWC4", 6.481))
	, yCoordWC1_(ps.get<float>("yCoordWC1", 6.441))
	, yCoordWC2_(ps.get<float>("yCoordWC2", 6.493))
	, yCoordWC3_(ps.get<float>("yCoordWC3", 6.414))
	, yCoordWC4_(ps.get<float>("yCoordWC4", 6.420))
	, posDev_(ps.get<float>("posDev", 0.5))
	, meanTDC1_(ps.get<float>("meanTDC1", 60))
	, meanTDC2_(ps.get<float>("meanTDC2", 58))
	, meanTDC3_(ps.get<float>("meanTDC3", 58))
	, meanTDC4_(ps.get<float>("meanTDC4", 59))
	, meanTDC5_(ps.get<float>("meanTDC5", 53))
	, meanTDC6_(ps.get<float>("meanTDC6", 53))
	, meanTDC7_(ps.get<float>("meanTDC7", 50))
	, meanTDC8_(ps.get<float>("meanTDC8", 51))
	, meanTDC9_(ps.get<float>("meanTDC9", 52))
	, meanTDC10_(ps.get<float>("meanTDC10", 51))
	, meanTDC11_(ps.get<float>("meanTDC11", 56))
	, meanTDC12_(ps.get<float>("meanTDC12", 57))
	, meanTDC13_(ps.get<float>("meanTDC13", 51))
	, meanTDC14_(ps.get<float>("meanTDC14", 51))
	, meanTDC15_(ps.get<float>("meanTDC15", 50))
	, meanTDC16_(ps.get<float>("meanTDC16", 51))
	, tdcHalfWin_(ps.get<float>("tdcHalfWin", 15))
{
	TLOG_INFO("WireChamberDQM") << "WireChamberDQM CONSTRUCTOR BEGIN!!!!" << TLOG_ENDL;

        zcoords[0] = zCoordWC1_;
        zcoords[1] = zCoordWC2_;
        zcoords[2] = zCoordWC3_;
        zcoords[3] = zCoordWC4_;
        xcoords[0] = xCoordWC1_;
        xcoords[1] = xCoordWC2_;
        xcoords[2] = xCoordWC3_;
        xcoords[3] = xCoordWC4_;
        ycoords[0] = yCoordWC1_;
        ycoords[1] = yCoordWC2_;
        ycoords[2] = yCoordWC3_;
        ycoords[3] = yCoordWC4_;
        meantdcs[0] = meanTDC1_;
        meantdcs[1] = meanTDC2_;
        meantdcs[2] = meanTDC3_;
        meantdcs[3] = meanTDC4_;
        meantdcs[4] = meanTDC5_;
        meantdcs[5] = meanTDC6_;
        meantdcs[6] = meanTDC7_;
        meantdcs[7] = meanTDC8_;
        meantdcs[8] = meanTDC9_;
        meantdcs[9] = meanTDC10_;
        meantdcs[10] = meanTDC11_;
        meantdcs[11] = meanTDC12_;
        meantdcs[12] = meanTDC13_;
        meantdcs[13] = meanTDC14_;
        meantdcs[14] = meanTDC15_;
        meantdcs[15] = meanTDC16_;

	gStyle->SetOptStat("irm");
	gStyle->SetMarkerStyle(22);
	gStyle->SetMarkerColor(4);

	if(writeToNetwork_)
	{
		networkStreamer_.reset(
		    new ots::UDPDataStreamerBase(ps.get<std::string>("hostname"),
		                                 ps.get<unsigned>("port"),
		                                 ps.get<std::string>("stream_to_hostname"),
		                                 ps.get<unsigned>("stream_to_port")));
	}

	TLOG_DEBUG("WireChamberDQM") << "WireChamberDQM CONSTRUCTOR END" << TLOG_ENDL;
}

void ots::WireChamberDQM::analyze(art::Event const& e)
{
	TLOG_INFO("WireChamberDQM")
	    << "WireChamberDQM Analyzing event " << e.event() << TLOG_ENDL;

	// John F., 1/22/14 -- there's probably a more elegant way of
	// collecting fragments of various types using ART interface code;
	// will investigate. Right now, we're actually re-creating the
	// fragments locally

	artdaq::Fragments        fragments;
	artdaq::FragmentPtrs     containerFragments;
	std::vector<std::string> fragment_type_labels{"MWPC", "ContainerMWPC"};

        if (e.event() % beamPosAggregate_ == 0 ){
	   reset_beam_ = true;

	}

	TLOG_INFO("WireChamberDQM") << "WireChamberDQM Extracting Fragments" << TLOG_ENDL;
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
	TLOG_INFO("WireChamberDQM") << "WireChamberDQM: This event has " << fragments.size()
	                            << " MWPC Fragments" << TLOG_ENDL;

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
			TLOG_WARNING("WireChamberDQM")
			    << "Warning in WireChamberDQM: expected fragment with sequence ID "
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
			    "Error in WireChamberDQM: unknown fragment type supplied: " +
			    fragmentTypeToString(fragtype));
		}

		artdaq::Fragment::fragment_id_t fragment_id = frag.fragmentID();

		TLOG_INFO("WireChamberDQM") << "WireChamberDQM: Booking histograms" << TLOG_ENDL;
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
				TLOG_INFO("WireChamberDQM")
				    << "WireChamberDQM: Filling histograms" << TLOG_ENDL;
				fill(wcPtr, e.event());
			}
			break;

		default:
			throw cet::exception(
			    "Error in WireChamberDQM: unknown fragment type supplied: " +
			    fragmentTypeToString(fragtype));
		}

		// Draw the histogram
		TLOG_DEBUG("WireChamberDQM") << "WireChamberDQM: Writing data" << TLOG_ENDL;
		write(e.event(), e.run(), spill_histos_);
		if (aggregateHistosEnabled_) { write(e.event(), e.run(), aggregate_histos_, false); }


                if (reset_beam_){

		   write(e.event(), e.run(), beamspot_histos_, false, true);
		   reset_beam_ = false;
		   bookHistos(name, beamspot_histos_,"-BeamPosition"," - Beam Position");


		}

	}
	TLOG_DEBUG("WireChamberDQM") << "DONE ANALYZING" << TLOG_ENDL;
}

void ots::WireChamberDQM::calcXY(size_t t, size_t& x, size_t& y)
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

//======================  Track Finding  =====================

int ots::WireChamberDQM::trackFit(
    int npts, float& a, float& b, float& chs, float x[], float z[])
{
  float  t1, t2, t3, t4, denom, xn;
  float  sig, xt;
 //                   COMPUTE STRAIGHT LINE   X = A + B * Z
 //                   SIMPLE FIT, NO ERRORS INCLUDED OR GENERATED
 //==================================================================
       sig = 0.1/3.46;
       xn = npts;
       a = 0.0;  b = 0.0;
       t1 = 0.0; t2 = 0.0; t3 = 0.0; t4 = 0.0;
 
       for( int i = 0; i < npts; i++ )
        { t1 = t1 + z[i];
          t2 = t2 + x[i]*z[i];
          t3 = t3 + z[i]*z[i];
          t4 = t4 + x[i]; }
 
       denom = t1*t1 - xn * t3;
       if( abs(denom) < 0.0000001 ) return 1;
       a =((t1 * t2) - (t3 * t4) ) / denom;
       b = ( (t1 * t4) - (xn * t2) ) / denom;
 
 //                                         calculate chisq for the fit
      chs = 0.0;
      for( int i = 0; i < npts; i++ )
       { xt = (a + b*z[i] - x[i]);
        chs = chs + (x[i]-xt)*(x[i]-xt);
       }
       if( chs > 19.0 ) chs=19.0;
       return 0;
    }


void ots::WireChamberDQM::bookCanvas(std::string name,
                                         DQMHistos&  container,
                                         std::string keySuffix,
                                         std::string titleSuffix)
{
	// Setup Profile Canvas
	container.profiles_canvas_ = new TCanvas((name + "-Profiles" + keySuffix).c_str());
	size_t x, y;
	calcXY(MAX_CHAMBERS, x, y);
	TLOG_DEBUG("WireChamberDQM") << "Dividing Profile canvas into " << x << " x by " << y
	                             << " y plots." << TLOG_ENDL;
	container.profiles_canvas_->SetFixedAspectRatio();
	container.profiles_canvas_->Divide(x, y);
	container.profiles_canvas_->Update();
	//((TRootCanvas*)container.profiles_canvas_->GetCanvasImp())->DontCallClose();
	container.profiles_canvas_->SetTitle(
	    ("Wire Chamber Hit Profiles" + titleSuffix).c_str());

	// Setup TDC Canvas
	container.tdcs_canvas_ = new TCanvas((name + "-TDCs" + keySuffix).c_str());

	TLOG_DEBUG("WireChamberDQM") << "Dividing TDC canvas into " << MAX_MODULES << " x by "
	                             << MAX_CHAMBERS << " y plots." << TLOG_ENDL;
	container.tdcs_canvas_->Divide(MAX_MODULES, MAX_CHAMBERS);
	container.tdcs_canvas_->Update();
	//((TRootCanvas*)container.tdcs_canvas_->GetCanvasImp())->DontCallClose();
	container.tdcs_canvas_->SetTitle(("Wire Chamber Hits" + titleSuffix).c_str());

	// Setup Time Canvas
	container.times_canvas_ = new TCanvas((name + "-EventTiming" + keySuffix).c_str());
	calcXY(MAX_CHAMBERS, x, y);
	TLOG_DEBUG("WireChamberDQM")
	    << "Dividing Time canvas into " << x << " x by " << y << " y plots." << TLOG_ENDL;
	container.times_canvas_->Divide(x, y);
	container.times_canvas_->Update();
	//((TRootCanvas*)container.times_canvas_->GetCanvasImp())->DontCallClose();
	container.times_canvas_->SetTitle(
	    ("Wire Chamber Event Timing" + titleSuffix).c_str());

	// Setup Timediff Canvas
	container.timediffs_canvas_ =
	    new TCanvas((name + "-EventTimeDiff" + keySuffix).c_str());
	calcXY(MAX_CHAMBERS, x, y);
	TLOG_DEBUG("WireChamberDQM") << "Dividing Timediff canvas into " << x << " x by " << y
	                             << " y plots." << TLOG_ENDL;
	container.timediffs_canvas_->Divide(x, y);
	container.timediffs_canvas_->Update();
	//((TRootCanvas*)container.timediffs_canvas_->GetCanvasImp())->DontCallClose();
	container.timediffs_canvas_->SetTitle(
	    ("Wire Chamber Event Time Differences" + titleSuffix).c_str());

	// Setup Online Monitoring Canvas
	container.om_canvas_ = new TCanvas((name + "-OnlineMonitor" + keySuffix).c_str());
	TLOG_DEBUG("WireChamberDQM")
	    << "Dividing Online Monitoring canvas into 2 y plots." << TLOG_ENDL;
	container.om_canvas_->Divide(1, 2);
	auto lowerPad = container.om_canvas_->cd(2);
	TLOG_DEBUG("WireChamberDQM")
	    << "Dividing lower Online Monitoring canvas into 2 x plots" << TLOG_ENDL;
	lowerPad->Divide(2, 1);
	lowerPad->Update();
	container.om_canvas_->Update();
	//((TRootCanvas*)container.om_canvas_->GetCanvasImp())->DontCallClose();
	container.om_canvas_->SetTitle(("Wire Chamber Online Monitor" + titleSuffix).c_str());

	// Setup beam position canvas
	container.beampos_canvas_ = new TCanvas((name + "-BeamPosition" + keySuffix).c_str());
	container.beampos_canvas_->Divide(2, 2);
	container.beampos_canvas_->Update();
	container.beampos_canvas_->SetTitle(
	    ("Beam position and angle" + titleSuffix).c_str());
}

void ots::WireChamberDQM::bookHistos(std::string name,
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

	container.beampos_canvas_->cd(1);
	container.h_xintercept_ = new TH1F("XIntercept"," X Track Intercept;intecept (cm);events",101,-5.0,5.0);
	container.beampos_canvas_->cd(2);
	container.h_yintercept_ = new TH1F("YIntercept"," Y Track Intercept;intecept (cm);events",101,-4.0,4.0);
	container.beampos_canvas_->cd(3);
	container.h_xslope_ = new TH1F("XSlope"," X Track Slope;slope (rad);events",101,-0.005,0.005);
	container.beampos_canvas_->cd(4);
	container.h_yslope_ = new TH1F("YSlope"," Y Track Slope;slope (rad);events",101,-0.003,0.003);

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
	container.beampos_canvas_->Modified();
	container.beampos_canvas_->Update();
	if(writeOutput_)
	{
		container.profiles_canvas_->Write(0, TObject::kOverwrite);
		container.tdcs_canvas_->Write(0, TObject::kOverwrite);
		container.times_canvas_->Write(0, TObject::kOverwrite);
		container.timediffs_canvas_->Write(0, TObject::kOverwrite);
		container.om_canvas_->Write(0, TObject::kOverwrite);
                container.beampos_canvas_->Write(0, TObject::kOverwrite);
	}
}

void ots::WireChamberDQM::book(std::string name)
{
	if(!initialized_)
	{
		bookCanvas(name, spill_histos_);


		bookCanvas(name, beamspot_histos_,"-BeamPosition"," - Beam Position");
		bookHistos(name, beamspot_histos_,"-BeamPosition"," - Beam Position");
		if (aggregateHistosEnabled_)
		{
			bookCanvas(name, aggregate_histos_, "-AllSpills", " - All Spills");
			bookHistos(name, aggregate_histos_, "-AllSpills", " - All Spills");
		}

		initialized_ = true;
	}

	bookHistos(name, spill_histos_);
}

void ots::WireChamberDQM::makeTracks(int nxyhit[2][4], float posxy[2][4][10]){
                  float zrat2 = (zcoords[1]-zcoords[0])/(zcoords[3]-zcoords[0]);
                  float zrat3 = (zcoords[2]-zcoords[0])/(zcoords[3]-zcoords[0]);

		  int usehit[10][4], i1temp, i2temp, i3temp, i4temp;
		  float window, dev2s, dev3s, devs;
		  int maxtrack=5;
		  int ntkx=0, ntky=0;
		  float xi[5], chisqx[5], chisqy[5], xs[5], xtrack[4], xtry, yi[5], ys[5];
                  int missingplane[3][4] = { {0,0,0,1},{1,1,2,2},{2,3,3,3}};
		  for(int it=0;it<maxtrack;++it){
		     xi[it]=0.0;
		     xs[it]=0.0;
		     yi[it]=0.0;
		     ys[it]=0.0;
		     chisqx[it]=0.0;
		     chisqy[it]=0.0;
		  }


		  window=posDev_;
		for(int v=0; v<2; ++v){
		  for (int it=0; it<4;++it){
		     for (int jt=0;jt<10;++jt){
			usehit[jt][it]=0;
		     }
		  }
                  for(int i1=0; i1<nxyhit[v][0];++i1){
		     if (usehit[i1][0] > 0) continue;
		     xtrack[0] = posxy[v][0][i1];
		     i1temp = i1;

		     for(int i4=0;i4<nxyhit[v][3];++i4){
			if (usehit[i4][3] > 0) continue;
			i4temp = i4;
			xtrack[3] = posxy[v][3][i4];
			xtry = xtrack[0]+(xtrack[3]-xtrack[0])*zrat2;
			window=posDev_;
			i2temp = -5;
			dev2s = -5.0;

			for(int i2=0;i2<nxyhit[v][1];++i2){
			   if (usehit[i2][1] > 0) continue;
			   xtrack[1] = posxy[v][1][i2];
			   float dev2 = xtrack[1] - xtry;
			   if (fabs(dev2) <= window){
			      i2temp = i2;
			      window = fabs(dev2);
			      dev2s = dev2;
			   }
			}
			if (i2temp < 0) continue;
			xtrack[1] = posxy[v][1][i2temp]; 
			xtry = xtrack[0] + (xtrack[3]-xtrack[0])*zrat3;
			window=posDev_;
			i3temp=-5;
			dev3s = -5.0;
			for(int i3=0;i3<nxyhit[v][2];++i3){
			   if (usehit[i3][2] > 0)continue;
			   xtrack[2] = posxy[v][2][i3];
			   float dev3 = xtrack[2] -xtry;
			   if (fabs(dev3) < window){
			      window = fabs(dev3);
			      i3temp = i3; 
			      dev3s = dev3;
			   }
			}
			if (i3temp < 0) continue;
			xtrack[2] = posxy[v][2][i3temp];
                        if (v==0){
			   trackFit(4,xi[ntkx],xs[ntkx],chisqx[ntkx],xtrack,zcoords);
			   ntkx++;
			   if (ntkx>4)ntkx=4;
			usehit[i1temp][0] = ntkx+1;
			usehit[i2temp][1] = ntkx+1;
			usehit[i3temp][2] = ntkx+1;
			usehit[i4temp][3] = ntkx+1;
			}
			else if (v==1){
			   trackFit(4,yi[ntky],ys[ntky],chisqy[ntky],xtrack,zcoords);
			   ntky++;
			   if (ntky>4)ntky=4;
			usehit[i1temp][0] = ntky+1;
			usehit[i2temp][1] = ntky+1;
			usehit[i3temp][2] = ntky+1;
			usehit[i4temp][3] = ntky+1;
			}
		     }
		
		  }
		  float ztrack[3];
		  if (useThreePointTracks_){
		    for (int m=0;m<4;++m){
		       int m1 = missingplane[0][m];
		       for(int i1=0;i1<nxyhit[v][m1];++i1){
		  	  if(usehit[i1][m1]>0)continue;
			  xtrack[0] = posxy[v][m1][i1];
			  ztrack[0] = zcoords[m1];
			  i1temp = i1;
			  int m3 = missingplane[2][m];
			  for(int i3=0;i3<nxyhit[v][m3];++i3){
			     if (usehit[i3][m3]>0)continue;
			     xtrack[2]=posxy[v][m3][i3];
			     ztrack[2]=zcoords[m3];
			     i3temp = i3;
			     int m2 = missingplane[1][m];
			     float zrat = (zcoords[m2] - zcoords[m1])/(zcoords[m3]-zcoords[m1]);
			     xtry = posxy[v][m1][i1] + (posxy[v][m3][i3]-posxy[v][m1][i1])*zrat;
			     window=posDev_;
			     i2temp = -5;
			     dev3s = -5.0;
			     for(int i2=0;i2<nxyhit[v][m2];++i2){
			        if(usehit[i2][m3]>0)continue;
			        float dev3 = posxy[v][m2][i2] - xtry;
			        if (fabs(dev3)< window){
			 	   window = fabs(dev3);
                                   dev3s = dev3;
				   i2temp = i2;
			        }
			     }
			     if (i2temp < 0) continue;
			     xtrack[1]=posxy[v][m2][i2temp];
			     ztrack[1]=zcoords[m2];
                        if (v==0){
			   trackFit(3,xi[ntkx],xs[ntkx],chisqx[ntkx],xtrack,ztrack);
			   ntkx++;
			   if (ntkx>4)ntkx=4;
		             usehit[i1temp][m1] = ntkx+1;
			     usehit[i2temp][m2] = ntkx+1;
			     usehit[i3temp][m3] = ntkx+1;
			}
			else if (v==1){
			   trackFit(3,yi[ntky],ys[ntky],chisqy[ntky],xtrack,ztrack);
			   ntky++;
			   if (ntky>4)ntky=4;
		             usehit[i1temp][m1] = ntky+1;
			     usehit[i2temp][m2] = ntky+1;
			     usehit[i3temp][m3] = ntky+1;
			}
			  }
		       }
		    }
		  }
		}
		if (ntkx > 0 && ntky > 0){
			  spill_histos_.h_xintercept_->Fill(xi[0]);
			  spill_histos_.h_xslope_->Fill(xs[0]);
		          if (aggregateHistosEnabled_)
			  {
		            aggregate_histos_.h_xintercept_->Fill(xi[0]);
		            aggregate_histos_.h_xslope_->Fill(xs[0]);
			  }
			  beamspot_histos_.h_xintercept_->Fill(xi[0]);
			  beamspot_histos_.h_xslope_->Fill(xs[0]);
			  spill_histos_.h_yintercept_->Fill(yi[0]);
			  spill_histos_.h_yslope_->Fill(ys[0]);
		          if (aggregateHistosEnabled_)
			  {
		            aggregate_histos_.h_yintercept_->Fill(yi[0]);
		            aggregate_histos_.h_yslope_->Fill(ys[0]);
			  }
			  beamspot_histos_.h_yintercept_->Fill(yi[0]);
			  beamspot_histos_.h_yslope_->Fill(ys[0]);

		}
}

void ots::WireChamberDQM::fill(std::unique_ptr<WireChamberFragment>& frag, int event)
{
	unsigned int nhit_x = 0;
	unsigned int wire_x[MAX_HITS];
	unsigned int time_x[MAX_HITS];

	unsigned int nhit_y = 0;
	unsigned int wire_y[MAX_HITS];
	unsigned int time_y[MAX_HITS];

	unsigned int nhit_x_g = 0;
	int wire_x_g[MAX_HITS];
        int time_x_g[MAX_HITS];

	unsigned int nhit_y_g = 0;
        int wire_y_g[MAX_HITS];
	int time_y_g[MAX_HITS];


	unsigned int nhit_x_cut = 0;
	int wire_x_cut[MAX_HITS] = {0};
	int time_x_cut[MAX_HITS] = {0};

	unsigned int nhit_y_cut = 0;
	int wire_y_cut[MAX_HITS] = {0};
	int time_y_cut[MAX_HITS] = {0};

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

	//TLOG_INFO("WireChamberDQM") << "Starting event processing loop" << TLOG_ENDL;
	auto evtPtr = frag->dataBegin();
	while(evtPtr != nullptr)
	{
		chamberModuleNumber =
		    (evtPtr->TDCNumber - 1) % 4;  // module number within chamber, 0-3
		chamber = (evtPtr->TDCNumber - 1) / 4;

		if(chamberModuleNumber >= MAX_MODULES || chamber >= MAX_CHAMBERS)
		{
			TLOG_DEBUG("WireChamberDQM")
			    << "Bad data detected, moving on to next event" << TLOG_ENDL;
			evtPtr = frag->nextEvent(evtPtr);
			continue;
		}

		//execute track finding after data from all four chambers has been collected
		if (evtPtr->TDCNumber == 1){
                  int nxyhit[2][4];
		  float posxy[2][4][10];
		  int timexy[2][4][10];
		  for( int ich=0;ich<4;++ich){
		     nxyhit[0][ich] = 0;
		     nxyhit[1][ich] = 0;
		     for(int ih=0;ih<10;++ih){
			posxy[0][ich][ih] = -77.0;
			posxy[1][ich][ih] = -77.0;
			timexy[0][ich][ih] = -70;
			timexy[1][ich][ih] = -70;
		     }
		  }

                  for (unsigned int ihit=0;ihit<nhit_x_g;++ihit){

                    int itdc = wire_x_g[ihit] / 64;
		    int tdc = time_x_g[ihit] - meantdcs[itdc];
		    if (abs(tdc) < tdcHalfWin_){
		    //TLOG_INFO("WireChamberDQM") << ihit<<" "<<nhit_x_g<<" "<< itdc<<" "<<time_x_g[ihit]<<" "<<meantdcs[itdc]<<" "<<tdc<<" "<<tdcHalfWin_<<TLOG_ENDL;

                      wire_x_cut[nhit_x_cut] = wire_x_g[ihit];
		      time_x_cut[nhit_x_cut] = tdc;
		      nhit_x_cut++;
                      int tcham = (itdc / 4 );
		      int txy = (itdc % 4) / 2;
		      if (nxyhit[txy][tcham]>9) nxyhit[txy][tcham] = 9;
		      int nn = nxyhit[txy][tcham];
		      if (txy == 0){
			 posxy[txy][tcham][nn] = xcoords[tcham] - (wire_x_g[ihit] - (((tcham*2)+txy)*128))*0.1;
			 timexy[txy][tcham][nn] = tdc;
			 nxyhit[txy][tcham]++;
		      }
		    }

		  }



                  for (unsigned int ihit=0;ihit<nhit_y_g;++ihit){

                    int itdc = wire_y_g[ihit] / 64;
		    int tdc = time_y_g[ihit] - meantdcs[itdc];
		    if (abs(tdc) < tdcHalfWin_){

                      wire_y_cut[nhit_y_cut] = wire_y_g[ihit];
		      time_y_cut[nhit_y_cut] = tdc;
		      nhit_y_cut++;
                      int tcham = (itdc / 4);
		      int txy = (itdc % 4) / 2;
		      if (nxyhit[txy][tcham]>9) nxyhit[txy][tcham] = 9;
		      int nn = nxyhit[txy][tcham];
		      if (txy == 1){
			 posxy[txy][tcham][nn] = ycoords[tcham] - (wire_y_g[ihit] - (((tcham*2)+txy)*128))*0.1;
			 timexy[txy][tcham][nn] = tdc;
			 nxyhit[txy][tcham]++;
		      }
		    }

		  }
                  makeTracks( nxyhit, posxy);

		   nhit_x_g = 0;
		   nhit_y_g = 0;
                   nhit_x_cut = 0;
		   nhit_y_cut = 0;

		}

		xory = (chamberModuleNumber) / 2;		// 0 = x-wire, 1 = y-wire
		first_or_second = (chamberModuleNumber) % 2;
 //               TLOG_INFO("WireChamberDQM") << evtPtr->TDCNumber << " " << chamberModuleNumber << " " << chamber <<" " << xory <<" "<< first_or_second << TLOG_ENDL;
		if (chamber != lastChamber)
		{

//			TLOG_INFO("WireChamberDQM") << "NumHitsX: " << nhit_x << " NumHitsY: " << nhit_y  << " Chamber " << chamber << " Last Chamber "<< lastChamber <<TLOG_ENDL;

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
					spill_histos_.h2_profile_[lastChamber]->Fill(xpos, ypos);
					if(aggregateHistosEnabled_)
					{
						aggregate_histos_.h2_profile_[lastChamber]->Fill(xpos, ypos);
					}
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
			TLOG_TRACE("WireChamberDQM")
			    << "Not first event, filling event time diff: " << evtTimeDiff
			    << TLOG_ENDL;
			spill_histos_.h_timediff_[chamber]->Fill(evtTimeDiff);
			if(aggregateHistosEnabled_)
			{
				aggregate_histos_.h_timediff_[chamber]->Fill(evtTimeDiff);
			}
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
				TLOG_DEBUG("WireChamberDQM")
				    << "Bad data detected, moving on to next event" << TLOG_ENDL;
				evtPtr = reinterpret_cast<WireChamberFragment::TDCEvent const*>(&data);
				break;
			}
			lastChannel = chan;

			unsigned int tdc = data.TimeBits;

			auto tdcEventTime = (eventTime + tdc) * 9.14E-9;
			spill_histos_.h_time_[chamber]->Fill(tdcEventTime);
			spill_histos_.om_times_->Fill(tdcEventTime);
			if(aggregateHistosEnabled_)
			{
				aggregate_histos_.h_time_[chamber]->Fill(tdcEventTime);
				aggregate_histos_.om_times_->Fill(tdcEventTime);
			}

			// Fill tdc hit distribution
			spill_histos_.h_tdc_[chamber][chamberModuleNumber]->Fill(tdc);
			spill_histos_.om_tdcs_->Fill(tdc);
			spill_histos_.om_hits_->Fill(chan + 64 * (evtPtr->TDCNumber - 1));
			if(aggregateHistosEnabled_)
			{
				aggregate_histos_.h_tdc_[chamber][chamberModuleNumber]->Fill(tdc);
				aggregate_histos_.om_tdcs_->Fill(tdc);
				aggregate_histos_.om_hits_->Fill(chan + 64 * (evtPtr->TDCNumber - 1));
			}

			if(xory == 0)  // x-wire
			{
				if(nhit_x < MAX_HITS - 1)
				{
					wire_x[nhit_x] = chan + 64 * first_or_second;
					time_x[nhit_x] = tdc;
					nhit_x++;
					wire_x_g[nhit_x_g] = chan + 64 * (evtPtr->TDCNumber - 1);
					time_x_g[nhit_x_g] = tdc;
					nhit_x_g++;
				}
				else
				{
					TLOG_DEBUG("WireChamberDQM")
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
					wire_y_g[nhit_y_g] = chan + 64 * (evtPtr->TDCNumber - 1);
					time_y_g[nhit_y_g] = tdc;
					nhit_y_g++;
				}
				else
				{
					TLOG_DEBUG("WireChamberDQM")
					    << "Too many y-hits, mwpc " << evtPtr->TDCNumber << ", " << nhit_y
					    << " hits!" << TLOG_ENDL;
				}
			}
		}

		// TLOG_DEBUG("WireChamberDQM") << "Getting next event" << TLOG_ENDL;
		evtPtr = frag->nextEvent(evtPtr);
	}
	TLOG_DEBUG("WireChamberDQM") << "Done processing data" << TLOG_ENDL;
}

void ots::WireChamberDQM::write(int event, int run, DQMHistos& container, bool isSpill, bool beamspot)
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
      
        gStyle->SetOptFit(111);
	container.beampos_canvas_->cd(1);
	container.h_xintercept_->Draw();
	container.h_xintercept_->Fit("gaus");
        if (isSpill){
	   container.beampos_canvas_->SetTitle(("Beam position and angle - Spill "+std::to_string(event)).c_str());
	}
        if (beamspot){
	   container.beampos_canvas_->SetTitle(("Beam position and angle - Spills "+std::to_string(event-beamPosAggregate_+1)+" - "+std::to_string(event)).c_str());
	}
	container.beampos_canvas_->Modified();
	container.beampos_canvas_->Update();
	container.beampos_canvas_->cd(2);
	container.h_yintercept_->Draw();
	container.h_yintercept_->Fit("gaus");
        if (isSpill){
	   container.beampos_canvas_->SetTitle(("Beam position and angle - Spill "+std::to_string(event)).c_str());
        }
        if (beamspot){
	   container.beampos_canvas_->SetTitle(("Beam position and angle - Spills "+std::to_string(event-beamPosAggregate_+1)+" - "+std::to_string(event)).c_str());
	}
	container.beampos_canvas_->Modified();
	container.beampos_canvas_->Update();
	container.beampos_canvas_->cd(3);
	container.h_xslope_->Draw();
	container.h_xslope_->Fit("gaus");
        if (isSpill){
	   container.beampos_canvas_->SetTitle(("Beam position and angle - Spill "+std::to_string(event)).c_str());
	}
        if (beamspot){
	   container.beampos_canvas_->SetTitle(("Beam position and angle - Spills "+std::to_string(event-beamPosAggregate_+1)+" - "+std::to_string(event)).c_str());
	}
	container.beampos_canvas_->Modified();
	container.beampos_canvas_->Update();
	container.beampos_canvas_->cd(4);
	container.h_yslope_->Draw();
	container.h_yslope_->Fit("gaus");
        if (isSpill){
	   container.beampos_canvas_->SetTitle(("Beam position and angle - Spill "+std::to_string(event)).c_str());
	}
        if (beamspot){
	   container.beampos_canvas_->SetTitle(("Beam position and angle - Spills "+std::to_string(event-beamPosAggregate_+1)+" - "+std::to_string(event)).c_str());
	}
	container.beampos_canvas_->Modified();
	container.beampos_canvas_->Update();
	if (beamspot 
	    && (container.h_xintercept_->GetEntries() !=0)
	    && (container.h_yintercept_->GetEntries() !=0)
	    && (container.h_xslope_->GetEntries() !=0)
	    && (container.h_yslope_->GetEntries() !=0)
	    ){
          TF1* xintfit = container.h_xintercept_->GetFunction("gaus");
          TF1* yintfit = container.h_yintercept_->GetFunction("gaus");
          TF1* xslopefit = container.h_xslope_->GetFunction("gaus");
          TF1* yslopefit = container.h_yslope_->GetFunction("gaus");
	  container.beampos_canvas_->Print("beampos_fits.png");
	  float xi = xintfit->GetParameter(1)*10.0;
	  float xa = xslopefit->GetParameter(1)*1000.0;
	  float yi = yintfit->GetParameter(1)*10.0;
	  float ya = yslopefit->GetParameter(1)*1000.0;
	  float xie = xintfit->GetParError(1)*10.0;
	  float xae = xslopefit->GetParError(1)*1000.0;
	  float yie = yintfit->GetParError(1)*10.0;
	  float yae = yslopefit->GetParError(1)*1000.0;
	  float xis = xintfit->GetParameter(2)*10.0;
	  float xas = xslopefit->GetParameter(2)*1000.0;
	  float yis = yintfit->GetParameter(2)*10.0;
	  float yas = yslopefit->GetParameter(2)*1000.0;
	  float xise = xintfit->GetParError(2)*10.0;
	  float xase = xslopefit->GetParError(2)*1000.0;
	  float yise = yintfit->GetParError(2)*10.0;
	  float yase = yslopefit->GetParError(2)*1000.0;
	  TLOG_INFO("WireChamberDQM") << "Beam spot fit, XI: " << xi << " XS: "<<xa<<" YI: "<<yi<<" YS: "<<ya  << TLOG_ENDL;
          if (logBeamPosToACNET_){
	    writeACNET("F:MTWCHPM",xi);
            writeACNET("F:MTWCVPM",yi);
            writeACNET("F:MTWCHPMU",xie);
            writeACNET("F:MTWCVPMU",yie);
            writeACNET("F:MTWCHAM",xa);
            writeACNET("F:MTWCVAM",ya);
            writeACNET("F:MTWCHAMU",xae);
            writeACNET("F:MTWCVAMU",yae);
            writeACNET("F:MTWCHPS",xis);
            writeACNET("F:MTWCVPS",yis);
            writeACNET("F:MTWCHPSU",xise);
            writeACNET("F:MTWCVPSU",yise);
            writeACNET("F:MTWCHAS",xas);
            writeACNET("F:MTWCVAS",yas);
            writeACNET("F:MTWCHASU",xase);
            writeACNET("F:MTWCVASU",yase);
	  }
	  if (postBeamPosToECL_){
  	    ECLEntry_t eclEntry;
	    eclEntry.author(eclUserName_);
	    eclEntry.category("Facility/MWPC");
	    Form_t form;
	    Field_t field;
	    Form_t::field_sequence fields;
	    form.name("OTSDAQ System Logbook Entry");

	    field = Field_t(ECLConnection::EscapeECLString("Facility MWPC"), "Experminent");
	    fields.push_back(field);

	    field = Field_t(ECLConnection::EscapeECLString(std::to_string(run)), "RunNumber");
	    fields.push_back(field);

	    field = Field_t(ECLConnection::EscapeECLString("n/a"), "ActiveUsers");
	    fields.push_back(field);

	    std::string entry = "FTBF MWPC beam position from track fits with past " + std::to_string(beamPosAggregate_) + " beam spills. "
	       + "F:MTWCHPM = "+std::to_string(xi)
	       + "F:MTWCVPM = "+std::to_string(yi)
	       + "F:MTWCHPMU = "+std::to_string(xie)
	       + "F:MTWCVPMU = "+std::to_string(yie)
	       + "F:MTWCHAM = "+std::to_string(xa)
	       + "F:MTWCVAM = "+std::to_string(ya)
	       + "F:MTWCHAMU = "+std::to_string(xae)
	       + "F:MTWCVAMU = "+std::to_string(yae)
	       + "F:MTWCHPS = "+std::to_string(xis)
	       + "F:MTWCVPS = "+std::to_string(yis)
	       + "F:MTWCHPSU = "+std::to_string(xise)
	       + "F:MTWCVPSU = "+std::to_string(yise)
	       + "F:MTWCHAS = "+std::to_string(xas)
	       + "F:MTWCVAS = "+std::to_string(yas)
	       + "F:MTWCHASU = "+std::to_string(xase)
	       + "F:MTWCVASU = "+std::to_string(yase)
	       ;
	    field = Field_t(ECLConnection::EscapeECLString(entry), "Entry");
	    fields.push_back(field);


	    form.field(fields);
	    eclEntry.form(form);

	    ECLEntry_t::attachment_sequence attachments;

	    attachments.push_back(ECLConnection::MakeAttachmentImage("beampos_fits.png"));
            eclEntry.attachment(attachments);
	    ECLConnection eclConn(eclUserName_, eclPassword_, eclURL_);
	    if (!eclConn.Post(eclEntry)) {
	    	    TLOG_WARNING("WireChamberDQMECL") << "Warning in WireChamberDQMECL: failed to post enty to ECL"
	  		    << TLOG_ENDL;
	    }
	  }
	}
	if (writeToNetwork_ && isSpill)
	{
		TBufferFile msg(TBuffer::kWrite);

		msg.SetWriteMode();

		msg.WriteObject(container.profiles_canvas_);
		msg.WriteObject(container.tdcs_canvas_);
		msg.WriteObject(container.times_canvas_);
		msg.WriteObject(container.timediffs_canvas_);
		msg.WriteObject(container.om_canvas_);
                msg.WriteObject(container.beampos_canvas_);
		networkStreamer_->send(std::string(msg.Buffer(), msg.Length()));
	}
	if(writeOutput_)
	{
		if(isSpill && directoryForEachSpill_)
		{
			TDirectory* dir = fFile_->mkdir(
			    ("Run" + std::to_string(run) + "Spill" + std::to_string(event)).c_str(),
			    ("Run " + std::to_string(run) + ", Spill " + std::to_string(event))
			        .c_str());
			dir->cd();
		        container.profiles_canvas_->Write(0, TObject::kOverwrite);
		        container.tdcs_canvas_->Write(0, TObject::kOverwrite);
		        container.times_canvas_->Write(0, TObject::kOverwrite);
		        container.timediffs_canvas_->Write(0, TObject::kOverwrite);
		        container.om_canvas_->Write(0, TObject::kOverwrite);
			container.beampos_canvas_->Write(0,TObject::kOverwrite);
			fFile_->cd();
			dir->Write();
			delete dir;
		}
		if (beamspot){
			TDirectory* dir = fFile_->mkdir(("BeamSpot_Run_" + std::to_string(run) + "_Spill_" + std::to_string(event-beamPosAggregate_+1)+"-"+std::to_string(event)).c_str(), ("BeamSpot Run " + std::to_string(run) + ", Spill " + std::to_string(event-beamPosAggregate_+1)+" - "+std::to_string(event)).c_str());
			dir->cd();
			container.beampos_canvas_->Write(0,TObject::kOverwrite);
			fFile_->cd();
			dir->Write();
			delete dir;
			return;
		}

		container.profiles_canvas_->Write(0, TObject::kOverwrite);
		container.tdcs_canvas_->Write(0, TObject::kOverwrite);
		container.times_canvas_->Write(0, TObject::kOverwrite);
		container.timediffs_canvas_->Write(0, TObject::kOverwrite);
		container.om_canvas_->Write(0, TObject::kOverwrite);
                container.beampos_canvas_->Write(0,TObject::kOverwrite);
		fFile_->Write();
		fFile_->Flush();
	}
}

void ots::WireChamberDQM::writeACNET(std::string name, float val){

          xmlrpc_c::clientSimple myClient;
	  xmlrpc_c::value result;
          myClient.call("http://www-bd.fnal.gov/xmlrpc/Remote","Remote.setting","sd",&result,name.c_str(),val);	

}

void ots::WireChamberDQM::beginRun(art::Run const &e)
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
		if(overwrite_mode_)
			fFile_ = new TFile(outputFileName_.c_str(), "RECREATE");
		else
			fFile_ = new TFile(outputFileName_.c_str(), "UPDATE");

		fFile_->cd();
	}
}

DEFINE_ART_MODULE(ots::WireChamberDQM)
