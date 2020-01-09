// ============================================================================
//
// CaloSpy:  Monitoring of Calo channels
//
// 04-Jun-2019 S.Giovannella
// First version from CaloHitsFromFragments module (author: Tomonari Miyashita)
//
// ============================================================================

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#if ART_HEX_VERSION < 0x30204
#include "art/Framework/Services/Optional/TFileService.h"
#else
#include "art_root_io/TFileService.h" // Include moved here since art v3_02_04
#endif
#include "fhiclcpp/ParameterSet.h"

#include "art/Framework/Principal/Handle.h"
#include "mu2e-artdaq-core/Overlays/ArtFragmentReader.hh"

#include <artdaq-core/Data/Fragment.hh>
#include "/home/mu2etrig/test_stand/ots_dqmcalo/srcs/offline/DataProducts/inc/TrkTypes.hh"

// ROOT includes
#include <TH1F.h>
#include <TH1.h>
#include <TProfile.h>

#include <iostream>
#include <string>
#include <memory>

int Ntot = 0;
const int nCryDisk = 674;
const int nCryTot  = 2*nCryDisk;

struct cryStru
{
  float WFpeak0 = 0.;
  float WFpeak1 = 0.;
  float WFratio = 0.;
  float Trise0  = 0.;
  float Trise1  = 0.;
  float Tdecay0 = 0.;
  float Tdecay1 = 0.;
};

TH1F     *_hCaloOccupancy0    = nullptr;
TH1F     *_hCaloOccupancy1    = nullptr;
TProfile *_hMaxWaveForm0      = nullptr;
TProfile *_hMaxWaveForm1      = nullptr;
TProfile *_hMaxWFratio0       = nullptr;
TProfile *_hMaxWFratio1       = nullptr;
TProfile *_hTrise0            = nullptr;
TProfile *_hTrise1            = nullptr;
TProfile *_hTdecay0           = nullptr;
TProfile *_hTdecay1           = nullptr;
TProfile *_hWave[6][28][20]   = {};

// ============================================================================

namespace mu2e {

// ============================================================================

  class CaloSpy : public art::EDAnalyzer {

  public:

    using EventNumber_t = art::EventNumber_t;
    using adc_t = mu2e::ArtFragmentReader::adc_t;
  
    // --- C'tor/d'tor:
    explicit  CaloSpy(fhicl::ParameterSet const& pset);

    // --- Production:
    void beginJob();
    void analyze( const art::Event& evt);
    void endJob();

  private:
    int   diagLevel_;
    int   parseCAL_;
    art::InputTag trkFragmentsTag_;
    art::InputTag caloFragmentsTag_;

  };  // CaloSpy

// ============================================================================

  CaloSpy::CaloSpy(const fhicl::ParameterSet& pset)
    :  art::EDAnalyzer(pset)
    , diagLevel_(pset.get<int>("diagLevel",0))
    , parseCAL_(pset.get<int>("parseCAL",1))
    , trkFragmentsTag_(pset.get<art::InputTag>("trkTag","daq:trk"))
    , caloFragmentsTag_(pset.get<art::InputTag>("caloTag","daq:calo"))
  {
    //produces<EventNumber_t>(); 
    //produces<mu2e::StrawDigiCollection>();
    //produces<mu2e::CaloDigiCollection>(); 
  }

// ============================================================================
// Histogram booking
// ============================================================================

  void CaloSpy::beginJob(){

    ///art::ServiceHandle<art::TFileService> tfs;

    char *hName = new char[20];;

    /*
    _hCaloOccupancy0 = tfs->make<TH1F>("hCaloOccupancy0","Calo Occupancy - Disk 0",nCryTot,0.,nCryTot);
    _hCaloOccupancy1 = tfs->make<TH1F>("hCaloOccupancy1","Calo Occupancy - Disk 1",nCryTot,0.,nCryTot);
    _hMaxWaveForm0 = tfs->make<TProfile>("hMaxWaveForm0","Max of WaveForm - Disk 0",nCryTot,0.,nCryTot,0.,2000.);
    _hMaxWaveForm1 = tfs->make<TProfile>("hMaxWaveForm1","Max of WaveForm - Disk 1",nCryTot,0.,nCryTot,0.,2000.);
    _hMaxWFratio0  = tfs->make<TProfile>("hMaxWFratio0"," Ratio of Max WF - Disk 0",nCryDisk,0.,nCryDisk,0.,2000.);
    _hMaxWFratio1  = tfs->make<TProfile>("hMaxWFratio1"," Ratio of Max WF - Disk 1",nCryDisk,0.,nCryDisk,0.,2000.);
    _hTrise0  = tfs->make<TProfile>("hTrise0","Rise Time - Disk 0",nCryTot,0.,nCryTot,0.,2000.);
    _hTrise1  = tfs->make<TProfile>("hTrise1","Rise Time - Disk 1",nCryTot,0.,nCryTot,0.,2000.);
    _hTdecay0 = tfs->make<TProfile>("hTdecay0","Decay Time - Disk 0",nCryTot,0.,nCryTot,0.,2000.);
    _hTdecay1 = tfs->make<TProfile>("hTdecay1","Decay Time - Disk 1",nCryTot,0.,nCryTot,0.,2000.);

    for( int iRoc=0; iRoc<6; iRoc++){
      for( int iDtc=0; iDtc<28; iDtc++){ 
	for( int iCha=0; iCha<20; iCha++){
	  sprintf(hName,"Wave_%d_%d_%d",iRoc,iDtc,iCha);
	  _hWave[iRoc][iDtc][iCha] = tfs->make<TProfile>(hName,"Wave",30,0.,30,-100.,2000.);
	}}}
    */
  }

// ============================================================================

  void CaloSpy::analyze( const art::Event& event ){

    art::EventNumber_t eventNumber = event.event();

    ++Ntot;
    struct cryStru crystal[nCryTot];

    auto trkFragments = event.getValidHandle<artdaq::Fragments>(trkFragmentsTag_);
    auto calFragments = event.getValidHandle<artdaq::Fragments>(caloFragmentsTag_);
    size_t numTrkFrags = trkFragments->size();
    size_t numCalFrags = calFragments->size();

    if( diagLevel_ > 1 ) {
      std::cout << std::dec << "CaloSpy: Run " << event.run() << ", subrun " << event.subRun()
		<< ", event " << eventNumber << " has " << std::endl;
      std::cout << trkFragments->size() << " TRK fragments, and ";
      std::cout << calFragments->size() << " CAL fragments." << std::endl;

      size_t totalSize = 0;
      for(size_t idx = 0; idx < trkFragments->size(); ++idx) {
	auto size = ((*trkFragments)[idx]).size() * sizeof(artdaq::RawDataType);
	totalSize += size;
	//      std::cout << "\tTRK Fragment " << idx << " has size " << size << std::endl;
    }
      for(size_t idx = 0; idx < calFragments->size(); ++idx) {
	auto size = ((*calFragments)[idx]).size() * sizeof(artdaq::RawDataType);
	totalSize += size;
	//      std::cout << "\tCAL Fragment " << idx << " has size " << size << std::endl;
      }
    
      std::cout << "\tTotal Size: " << (int)totalSize << " bytes." << std::endl;  
    }

    std::string curMode = "TRK";

    // Loop over the TRK and CAL fragments
    for (size_t idx = 0; idx < numTrkFrags+numCalFrags; ++idx) {

      auto curHandle = trkFragments;
      size_t curIdx = idx;
      if(idx>=numTrkFrags) {
	curIdx = idx-numTrkFrags;
	curHandle = calFragments;
      }
      const auto& fragment((*curHandle)[curIdx]);
      
      mu2e::ArtFragmentReader cc(fragment);
    
      if( diagLevel_ > 3 ) {
	std::cout << std::endl;
	std::cout << "ArtFragmentReader: ";
	std::cout << "\tBlock Count: " << std::dec << cc.block_count() << std::endl;
	std::cout << "\tByte Count: " << cc.byte_count() << std::endl;
	std::cout << std::endl;
	std::cout << "\t" << "====== Example Block Sizes ======" << std::endl;
	for(size_t i=0; i<10; i++) {
	  if(i <cc.block_count()) {
	    std::cout << "\t" << i << "\t" << cc.blockIndexBytes(i) << "\t" << cc.blockSizeBytes(i) << std::endl;
	  }
	}
	std::cout << "\t" << "=========================" << std::endl;
      }
    
      std::string mode_;

      for(size_t curBlockIdx=0; curBlockIdx<cc.block_count(); curBlockIdx++) {

	size_t blockStartBytes = cc.blockIndexBytes(curBlockIdx);
	size_t blockEndBytes = cc.blockEndBytes(curBlockIdx);

	if( diagLevel_ > 3 ) {
	  std::cout << "BLOCKSTARTEND: " << blockStartBytes << " " << blockEndBytes << " " << cc.blockSizeBytes(curBlockIdx)<< std::endl;
	  std::cout << "IndexComparison: " << cc.blockIndexBytes(0)+16*(0+3*curBlockIdx) << "\t";
	  std::cout                        << cc.blockIndexBytes(curBlockIdx)+16*(0+3*0) << std::endl;
	}

	adc_t const *pos = reinterpret_cast<adc_t const *>(cc.dataAtBytes(blockStartBytes));

	if( diagLevel_ > 3 ) {
	  // Print binary contents the first 3 packets starting at the current position
	  // In the case of the tracker simulation, this will be the whole tracker
	  // DataBlock. In the case of the calorimeter, the number of data packets
	  // following the header packet is variable.
	  cc.printPacketAtByte(cc.blockIndexBytes(0)+16*(0+3*curBlockIdx));
	  cc.printPacketAtByte(cc.blockIndexBytes(0)+16*(1+3*curBlockIdx));
	  cc.printPacketAtByte(cc.blockIndexBytes(0)+16*(2+3*curBlockIdx));
	
	  // Print out decimal values of 16 bit chunks of packet data
	  for(int i=7; i>=0; i--) {
	    std::cout << "Data bytes: ";
	    std::cout << (adc_t) *(pos+i);
	    std::cout << " ";
	  }
	  std::cout << std::endl;
	}	    

	adc_t rocID = cc.DBH_ROCID(pos);
	adc_t valid = cc.DBH_Valid(pos);
	adc_t packetCount = cc.DBH_PacketCount(pos);
	    
	uint32_t timestampLow    = cc.DBH_TimestampLow(pos);
	uint32_t timestampMedium = cc.DBH_TimestampMedium(pos);
	size_t timestamp = timestampLow | (timestampMedium<<16);
      
	adc_t EVBMode = cc.DBH_EVBMode(pos);     
	adc_t sysID = cc.DBH_SubsystemID(pos);  // Detector Cal/Trk
	adc_t dtcID = cc.DBH_DTCID(pos);        // Fiber
	// channel Id missing!!! --> Ask Tomo!!!

	int CalPoi = 0;

	eventNumber = timestamp;
      
	if(sysID==0) {
	  mode_ = "TRK";
	} else if(sysID==1) {
	  mode_ = "CAL";
	}

//	///////////////////////////////////////////////////////////////////////////
//	// NOTE: Because the tracker code in offline has not been updated to
//	// use 15 samples, it is necessary to add an extra sample in order to
//	// initialize an ADCWaveform that can be passed to the StrawDigi
//	// constructor. This means that the digis produced by CaloHitsFromFragments
//	// will differ from those processed in offline so the filter performance
//	// will be different. This is only temporary.
//	std::array<adc_t,15> const & shortWaveform = cc.DBT_Waveform(pos);
//	mu2e::TrkTypes::ADCWaveform wf;
//	for(size_t i=0; i<15; i++) {
//	  wf[i] = shortWaveform[i];
//	}
//	wf[15] = 0;
//	///////////////////////////////////////////////////////////////////////////

	// Parse phyiscs information from CAL packets
	if(mode_ == "CAL" && packetCount>0 && parseCAL_>0) {

	  std::vector<int> cwf = cc.DBC_Waveform(pos);
	
	  if( diagLevel_ > 1 ) {
	    adc_t crystalID  = cc.DBC_CrystalID(pos);
	    adc_t apdID      = cc.DBC_apdID(pos);
	    adc_t time       = cc.DBC_Time(pos);
	    adc_t numSamples = cc.DBC_NumSamples(pos);
	    adc_t peakIdx    = cc.DBC_PeakSampleIdx(pos);
	    adc_t channelID  = crystalID % 20;

	    std::cout << "Ntot: " << Ntot << std::endl;
	    std::cout << "timestamp: " << timestamp << std::endl;
	    std::cout << "sysID: " << sysID << std::endl;
	    std::cout << "dtcID: " << dtcID << std::endl;
	    std::cout << "rocID: " << rocID << std::endl;
	    std::cout << "chaID: " << channelID << std::endl;
	    std::cout << "packetCount: " << packetCount << std::endl;
	    std::cout << "valid: " << valid << std::endl;
	    std::cout << "EVB mode: " << EVBMode << std::endl;		
	  
	    for(int i=7; i>=0; i--) {
	      std::cout << (adc_t) *(pos+8+i);
	      std::cout << " ";
	    }
	    std::cout << std::endl;
	  
	    for(int i=7; i>=0; i--) {
	      std::cout << (adc_t) *(pos+8*2+i);
	      std::cout << " ";
	    }
	    std::cout << std::endl;
	  
	    std::cout << "Crystal ID: " << crystalID << std::endl;		
	    std::cout << "APD ID: " << apdID << std::endl;
	    std::cout << "Time: " << time << std::endl;
	    std::cout << "NumSamples: " << numSamples << std::endl;
	    //std::cout << "PeakIdx: " << cwf[peakIdx] << std::endl;
	    std::cout << "Waveform: {";
	    for(size_t i=0; i<cwf.size(); i++) {
	      std::cout << cwf[i];
	      if(i<cwf.size()-1) {
		std::cout << ",";
	      }
	    }
	    std::cout << "}" << std::endl;

	    //-------------------------
	    // Fill crystal structure
	    //-------------------------

	    if( apdID==0 ){
	      crystal[crystalID].WFpeak0 = cwf[peakIdx];
	      crystal[crystalID].Trise0 = 5.*(peakIdx-4);
	      crystal[crystalID].Tdecay0 = 5.*(numSamples-peakIdx);
	    } 
	    else if( apdID==1 ){
	      crystal[crystalID].WFpeak1 = cwf[peakIdx];
	      crystal[crystalID].Trise1 = 5.*(peakIdx-4);
	      crystal[crystalID].Tdecay1 = 5.*(numSamples-peakIdx);
	    } 
	    else{
	      std::cout <<  "mu2e::CaloSpy::analyze eventNumber=" << (int)(event.event()) << 
		"Unknown SiPM id" << apdID <<std::endl;
	      exit(0);
	    }

	    //-------------------------------
	    // Fill single readout histograms
	    //-------------------------------

	    // use DMAP to extract CalPoi
	    CalPoi = 2*crystalID + apdID;
	    std::cout << "CalPoi: " << CalPoi << std::endl;
	  
	    /*
	    if( CalPoi<2*nCryDisk ){                      // Two readouts per crystal
	      _hCaloOccupancy0->Fill(CalPoi);
	      _hMaxWaveForm0->Fill(CalPoi,cwf[peakIdx]);
	      _hTrise0->Fill(CalPoi,5.*(peakIdx-4));
	      _hTdecay0->Fill(CalPoi,5.*(numSamples-peakIdx));
	      //std::cout << " Filling histos for Disk 0" << std::endl;
	    }
	    else{
	      int DiskPoi = CalPoi - 2*nCryDisk;
	      _hCaloOccupancy1->Fill(DiskPoi);
	      _hMaxWaveForm1->Fill(DiskPoi,cwf[peakIdx]);
	      _hTrise1->Fill(DiskPoi,5.*(peakIdx-4));
	      _hTdecay1->Fill(DiskPoi,5.*(numSamples-peakIdx));
	      //std::cout << " Filling histos for Disk 1" << std::endl;
	    }
	    */

	    // Text format: timestamp crystalID roID time nsamples samples...
	    // Example: 1 201 402 660 18 0 0 0 0 1 17 51 81 91 83 68 60 58 52 42 33 23 16
	    std::cout << "GREPMECAL: " << timestamp << " ";
	    std::cout << crystalID << " ";
	    std::cout << apdID << " ";
	    std::cout << time << " ";
	    std::cout << cwf.size() << " ";
	    for(size_t i=0; i<cwf.size(); i++) {
	      std::cout << cwf[i];
	      if(i<cwf.size()-1) {
		std::cout << " ";
		/*
		if( apdID==0 ){
		  _hWave[rocID][dtcID][channelID]->Fill(i,cwf[i]);
		}
		else{
		  _hWave[rocID][dtcID][channelID+8]->Fill(i,cwf[i]);
		}
		*/
	      }
	    }
	    std::cout << std::endl;
	  } // End debug output
	
	} // End Cal Mode
      
      } // End loop over DataBlocks within fragment 
      
    } // Close loop over fragments

    //-------------------------
    // Fill crystal histograms
    //------------------------

    for( int iCry=0; iCry<nCryTot; iCry++){
    
      if( crystal[iCry].WFpeak1!=0. ){
	crystal[iCry].WFratio = crystal[iCry].WFpeak0/crystal[iCry].WFpeak1;
      }
      else{
	crystal[iCry].WFratio = -1.;
      }

      /*
      if( iCry<nCryDisk ){
	_hMaxWFratio0->Fill(iCry,crystal[iCry].WFratio);
      }
      else{
	int iPoi = iCry-nCryDisk;
	_hMaxWFratio1->Fill(iPoi,crystal[iPoi].WFratio);
      }
      */
    }

    if( diagLevel_ > 0 ) {
      std::cout << "mu2e::CaloSpy::analyze exiting eventNumber=" << (int)(event.event()) << " / timestamp=" << (int)eventNumber <<std::endl; 
    }

  }  // analyze()

// ============================================================================
                                         
  void CaloSpy::endJob(){
    std::cout << "CaloSpy: Normalizing histos to number of events:" << Ntot << std::endl;

    /*
    // Get Ntot from art?
    if( Ntot>0 ){
      _hCaloOccupancy0->Sumw2();
      _hCaloOccupancy1->Sumw2();
      _hCaloOccupancy0->Scale(1./Ntot);
      _hCaloOccupancy1->Scale(1./Ntot);
    }
    */
  }

// ============================================================================

} // namespace mu2e

DEFINE_ART_MODULE(mu2e::CaloSpy)
