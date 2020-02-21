// ======================================================================
//
// CaloCosmics:  Monitoring of calorimeter with cosmic rays
//
// ======================================================================

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "fhiclcpp/ParameterSet.h"

#include "art/Framework/Principal/Handle.h"
#include "mu2e-artdaq-core/Overlays/ArtFragmentReader.hh"

#include <artdaq-core/Data/Fragment.hh>

// ROOT includes
//#include "art/Framework/Services/Optional/TFileService.h"
//#include "art_root_io/TFileService.h" // Include moved here since art v3_02_04
//#include <TH1F.h>
//#include <TH1.h>
//#include <TProfile.h>

#include <iostream>

#include <string>

#include <memory>

//int Ntot = 0;
//const int nCryDisk = 674;
//const int nCryTot  = 2*nCryDisk;

// Notation: 0/1 disk number; A/B readout side

//struct cryStru
//{
//  float WFpeakA = 0.;
//  float WFpeakB = 0.;
//  float WFratio = 0.;
//  float TriseA  = 0.;
//  float TriseB  = 0.;
//  float TdecayA = 0.;
//  float TdecayB = 0.;
//};

//TH1F     *_hCaloOccupancy0    = nullptr;
//TH1F     *_hCaloOccupancy1    = nullptr;
//TProfile *_hMaxWaveForm0      = nullptr;
//TProfile *_hMaxWaveForm1      = nullptr;
//TProfile *_hMaxWFratio0       = nullptr;
//TProfile *_hMaxWFratio1       = nullptr;
//TProfile *_hTrise0            = nullptr;
//TProfile *_hTrise1            = nullptr;
//TProfile *_hTdecay0           = nullptr;
//TProfile *_hTdecay1           = nullptr;
//TProfile *_hWave[6][28][20]   = {};

namespace art {
  class CaloCosmics;
}

using art::CaloCosmics;

// ======================================================================

class art::CaloCosmics
  : public EDAnalyzer
{

public:

  /*
  using EventNumber_t = art::EventNumber_t;
  using adc_t = mu2e::ArtFragmentReader::adc_t;
  */

  // --- C'tor/d'tor:
  explicit  CaloCosmics(fhicl::ParameterSet const& pset);
  virtual  ~CaloCosmics()  { }

  // --- Production:
  virtual void beginJob();
  virtual void analyze( Event const&);
  virtual void endJob();

private:
  int   diagLevel_;

  int   parseCAL_;
  int   parseTRK_;

  /*
  art::InputTag trkFragmentsTag_;
  art::InputTag caloFragmentsTag_;
  */
};  // CaloCosmics

// ======================================================================

CaloCosmics::CaloCosmics(fhicl::ParameterSet const& pset)
  : EDAnalyzer(pset)
  , diagLevel_(pset.get<int>("diagLevel",0))
    //  , parseCAL_(pset.get<int>("parseCAL",1))
    //  , trkFragmentsTag_(pset.get<art::InputTag>("trkTag","daq:trk"))
    //  , caloFragmentsTag_(pset.get<art::InputTag>("caloTag","daq:calo"))
{
}

// ======================================================================
// Begin Job - Histogram booking
// ======================================================================

  void CaloCosmics::beginJob(){
    /*
    art::ServiceHandle<art::TFileService> tfs;

    char *hName = new char[20];;

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

// ======================================================================
// Event Analyzer
// ======================================================================

void
CaloCosmics::
analyze( Event const& event )
{
  art::EventNumber_t eventNumber = event.event();

  errdd

  ++Ntot;
  if( Ntot%1==0 && _diaglevel > 0 ) std::cout<<"[CaloCosmics] Processing event "<<Ntot<<std::endl;

  /*
  struct cryStru crystal[nCryTot];

  art::Handle<artdaq::Fragments> trkFragments, calFragments;
  size_t numTrkFrags(0), numCalFrags(0);
  if (parseTRK_){
    event.getByLabel(trkFragmentsTag_ , trkFragments);
    if (!trkFragments.isValid()){            
      return;
    }
    numTrkFrags = trkFragments->size();
  }
  if (parseCAL_){
    event.getByLabel(caloFragmentsTag_, calFragments);
    if (!calFragments.isValid()){
      return;
    }
    numCalFrags = calFragments->size();
  }
  // size_t numTrkFrags = trkFragments->size();
  // size_t numCalFrags = calFragments->size();

  if( diagLevel_ > 1 ) {
    std::cout << std::dec << "[CaloCosmics] Run " << event.run() << ", subrun " << event.subRun()
	      << ", event " << eventNumber << " has " << std::endl;
    std::cout << numTrkFrags << " TRK fragments, and ";
    std::cout << numCalFrags << " CAL fragments." << std::endl;

    size_t totalSize = 0;
    for(size_t idx = 0; idx < numTrkFrags; ++idx) {
      auto size = ((*trkFragments)[idx]).size() * sizeof(artdaq::RawDataType);
      totalSize += size;
      //      std::cout << "\tTRK Fragment " << idx << " has size " << size << std::endl;
    }
    for(size_t idx = 0; idx < numCalFrags; ++idx) {
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
    
    if( diagLevel_ > 1 ) {
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

      if( diagLevel_ > 1 ) {
	std::cout << "BLOCKSTARTEND: " << blockStartBytes << " " << blockEndBytes << " " << cc.blockSizeBytes(curBlockIdx)<< std::endl;
	std::cout << "IndexComparison: " << cc.blockIndexBytes(0)+16*(0+3*curBlockIdx) << "\t";
	std::cout                        << cc.blockIndexBytes(curBlockIdx)+16*(0+3*0) << std::endl;
      }

      adc_t const *pos = reinterpret_cast<adc_t const *>(cc.dataAtBytes(blockStartBytes));

      if( diagLevel_ > 1 ) {
	// Print binary contents the first 3 packets starting at the current position
	// In the case of the tracker simulation, this will be the whole tracker
	// DataBlock. In the case of the calorimeter, the number of data packets
	// following the header packet is variable.
	cc.printPacketAtByte(cc.blockIndexBytes(0)+16*(0+3*curBlockIdx));
	cc.printPacketAtByte(cc.blockIndexBytes(0)+16*(1+3*curBlockIdx));
	cc.printPacketAtByte(cc.blockIndexBytes(0)+16*(2+3*curBlockIdx));
	
	// Print out decimal values of 16 bit chunks of packet data
	for(int i=7; i>=0; i--) {
	  std::cout <<"0x" << std::hex << std::setw(4) << std::setfill('0')<< (adc_t) *(pos+i) << std::dec << std::setw(0);
	  std::cout << " ";
	}
	std::cout << std::endl;
      }    

      auto hdr = cc.GetHeader(curBlockIdx);
      if(hdr == nullptr) {
	mf::LogError("CaloCosmics") << "Unable to retrieve header from block " << curBlockIdx << "!" << std::endl;
	continue;
      }
      
      if(diagLevel_ > 1) {


	std::cout << "timestamp: " << static_cast<int>(hdr->GetTimestamp()) << std::endl;
	std::cout << "hdr->SubsystemID: " << static_cast<int>(hdr->SubsystemID) << std::endl;
	std::cout << "dtcID: " << static_cast<int>(hdr->DTCID) << std::endl;
	std::cout << "rocID: " << static_cast<int>(hdr->ROCID) << std::endl;
	std::cout << "packetCount: " <<static_cast<int>( hdr->PacketCount) << std::endl;
	std::cout << "valid: " << static_cast<int>(hdr->Valid) << std::endl;
	std::cout << "EVB mode: " << static_cast<int>(hdr->EVBMode) << std::endl;

	for(int i=7; i>=0; i--) {
	  std::cout << (adc_t) *(pos+8+i);
	  std::cout << " ";
	}
	std::cout << std::endl;
      }

      eventNumber = hdr->GetTimestamp();
      
      if(idx < numTrkFrags){
	mode_ = "TRK";
      }else {
	mode_ = "CAL";
      }

      int CalPoi = 0;

      if(mode_ == "CAL" && hdr->PacketCount>0 && parseCAL_>0) {// Parse phyiscs information from CAL packets
	
	auto calData = cc.GetCalorimeterData(curBlockIdx);
	if(calData == nullptr) {
	  mf::LogError("CaloCosmics") << "Error retrieving Calorimeter data from block " << curBlockIdx << "! Aborting processing of this block!";
	  continue;
	}

	if( diagLevel_ > 0 ) {
	  std::cout <<"[CaloCosmics] NEW CALDATA: NumberOfHits "<< calData->NumberOfHits << std::endl;
	}

	bool err = false;
	for(size_t hitIdx = 0; hitIdx<calData->NumberOfHits; hitIdx++) {

	  // Fill the CaloDigiCollection
	  const mu2e::ArtFragmentReader::CalorimeterHitReadoutPacket* hitPkt(0);
	  hitPkt = cc.GetCalorimeterReadoutPacket(curBlockIdx, hitIdx);
	  if(hitPkt == nullptr) {
	    mf::LogError("CaloCosmics") << "Error retrieving Calorimeter data from block " << curBlockIdx << " for hit " << hitIdx << "! Aborting processing of this block!";
	    err = true;
	    break;
	  }
	    
	  if( diagLevel_ > 0 ) {
	    std::cout <<"[CaloCosmics] calo hit "<< hitIdx <<std::endl;
	    std::cout <<"[CaloCosmics] \thitPkt " << hitPkt << std::endl;
	    std::cout <<"[CaloCosmics] \tChNumber   " << (int)hitPkt->ChannelNumber  << std::endl;
	    std::cout <<"[CaloCosmics] \tDIRACA     " << (int)hitPkt->DIRACA   << std::endl;
	    std::cout <<"[CaloCosmics] \tDIRACB     " << (int)hitPkt->DIRACB  << std::endl;
	    std::cout <<"[CaloCosmics] \tErrorFlags " << (int)hitPkt->ErrorFlags  << std::endl;
	    std::cout <<"[CaloCosmics] \tTime       " << (int)hitPkt->Time  << std::endl;
	    std::cout <<"[CaloCosmics] \tNSamples   " << (int)hitPkt->NumberOfSamples << std::endl;
	    std::cout <<"[CaloCosmics] \tIndexMax   " << (int)hitPkt->IndexOfMaxDigitizerSample << std::endl;
	  }
	    
	  auto first = cc.GetCalorimeterReadoutSample(curBlockIdx,hitIdx,0);
	  auto last  = cc.GetCalorimeterReadoutSample(curBlockIdx, hitIdx, hitPkt->NumberOfSamples - 1);
	  if(first == nullptr || last == nullptr) {
	    mf::LogError("CaloCosmics") << "Error retrieving Calorimeter samples from block " << curBlockIdx << " for hit " << hitIdx << "! Aborting processing of this block!";
	    err = true;
	    break;
	  }

	  //the second argument is not included in the vector, so we need to add "+1"
	  // because we want the "last" item included
	  std::vector<int> cwf(first,last+1);

	  // IMPORTANT NOTE: we don't have a final
	  // mapping yet so for the moment, the BoardID field (described in docdb 4914) is just a
	  // placeholder. Because we still need to know which crystal a hit belongs to, we are
	  // temporarily storing the 4-bit apdID and 12-bit crystalID in the Reserved DIRAC A slot.
	  // Also, note that until we have an actual map, channel index does not actually correspond
	  // to the physical readout channel on a ROC.
	  adc_t crystalID  = hitPkt->DIRACB & 0x0FFF;
	  adc_t apdID      = hitPkt->DIRACB >> 12;
	  adc_t numSamples = hitPkt->NumberOfSamples;
	  adc_t peakIdx    = hitPkt->IndexOfMaxDigitizerSample;
	  adc_t channelID  = crystalID % 20;  // *** TEMPORARY until final mapping ***
	  if( diagLevel_ > 1 ) {
	    // Until we have the final mapping, the BoardID is just a placeholder
	    // adc_t BoardId    = cc.DBC_BoardID(pos,channelIdx);
	          
	    std::cout << "Crystal ID: " << (int)crystalID << std::endl;
	    std::cout << "APD ID: " << (int)apdID << std::endl;
	    std::cout << "Time: " << (int)hitPkt->Time << std::endl;
	    std::cout << "NumSamples: " << (int)hitPkt->NumberOfSamples << std::endl;
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
	      crystal[crystalID].WFpeakA = cwf[peakIdx];
	      crystal[crystalID].TriseA = 5.*(peakIdx-4);
	      crystal[crystalID].TdecayA = 5.*(numSamples-peakIdx);
	    } 
	    else if( apdID==1 ){
	      crystal[crystalID].WFpeakB = cwf[peakIdx];
	      crystal[crystalID].TriseB = 5.*(peakIdx-4);
	      crystal[crystalID].TdecayB = 5.*(numSamples-peakIdx);
	    } 
	    else{
	      std::cout <<  "mu2e::CaloCosmics::analyze eventNumber=" << (int)(event.event()) << 
		"Unknown SiPM id" << apdID <<std::endl;
	      exit(0);
	    }
	    
	    //-------------------------------
	    // Fill single readout histograms
	    //-------------------------------
      
	    // Text format: timestamp crystalID roID time nsamples samples...
	    // Example: 1 201 402 660 18 0 0 0 0 1 17 51 81 91 83 68 60 58 52 42 33 23 16
	    std::cout << "GREPMECAL: " << hdr->GetTimestamp() << " ";
	    std::cout << crystalID << " ";
	    std::cout << apdID << " ";
	    std::cout << hitPkt->Time << " ";
	    std::cout << cwf.size() << " ";
	    for(size_t i=0; i<cwf.size(); i++) {
	      std::cout << cwf[i];
	      if(i<cwf.size()-1) {
		std::cout << " ";
	      }
	    }
	    std::cout << std::endl;
	  } // End debug output
	      
	} // End loop over readout channels in DataBlock
	if(err) continue;
	
      } // End Cal Mode
       
    } // End loop over DataBlocks within fragment 
  
  } // Close loop over fragments

  //  }  // Close loop over the TRK and CAL collections
  */
  if( diagLevel_ > 0 ) {
    std::cout << "mu2e::CaloCosmics::analyze exiting eventNumber=" << (int)(event.event()) << " / timestamp=" << (int)eventNumber <<std::endl;
  }

}  // analyze()

// ======================================================================
// End Job - Normalize histograms
// ============================================================================
                                         
  void CaloCosmics::endJob(){
    std::cout << "CaloCosmics: Normalizing histos to number of events:" << Ntot << std::endl;
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

DEFINE_ART_MODULE(CaloCosmics)

// ======================================================================
