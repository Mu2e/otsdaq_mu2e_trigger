#ifndef _ots_FSSRDQMHistos_h_
#define _ots_FSSRDQMHistos_h_

#include <map>
#include <queue>
#include <string>
#include "otsdaq/RootUtilities/DQMHistosBase.h"
#include "otsdaq-trigger/DataDecoders/DataDecoder.h"
// ROOT documentation
// http://root.cern.ch/root/html/index.html

class TFile;
class TCanvas;
class TH1;
class TH1I;
class TH2;
class TProfile;
class TDirectory;
class TObject;

namespace ots
{
class ConfigurationTree;

class FSSRDQMHistos
{
  public:
	FSSRDQMHistos(void);
	// FSSRDQMHistos(std::string supervisorApplicationUID, std::string bufferUID,
	// std::string processorUID);
	virtual ~FSSRDQMHistos(void);
	void book(TDirectory*              myDirectory,
	          const ConfigurationTree& theXDAQContextConfigTree,
	          const std::string&       configurationPath);
	void clear();
	void fill(std::string& buffer, std::map<std::string, std::string> header);
	void load(std::string fileName);

	// Getters
	// TCanvas*  getCanvas (void){return canvas_;}
	// TH1F*     getHisto1D(void){return histo1D_;}
	// TH2F*     getHisto2D(void){return histo2D_;}
	// TProfile* getProfile(void){return profile_;}

  protected:
	DataDecoder          theDataDecoder_;
	std::queue<uint32_t> convertedBuffer_;

	TDirectory* myDir_;
	TDirectory* generalDir_;
	TDirectory* planesDir_;

	// TCanvas*      canvas_; // main canvas
	// TH1F*         histo1D_;// 1-D histogram
	// TH2F*         histo2D_;// 2-D histogram
	// TProfile*     profile_;// profile histogram
	//       IPAddress          port                channel
	std::map<std::string, std::map<std::string, std::map<unsigned int, TH1*>>>
	    planeOccupancies_;
	std::map<std::string, std::map<std::string, std::map<unsigned int, TH2*>>>
	      stationProfiles_;
	TH1I* numberOfTriggers_;
};
}

#endif
