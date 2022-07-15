#ifndef _ots_WireChamberDQMHistos_h_
#define _ots_WireChamberDQMHistos_h_

#include <map>
#include <queue>
#include <string>
#include "otsdaq/RootUtilities/DQMHistosBase.h"

static const int          MAX_CHAMBERS = 4;  // only support six chambers for now
static const int          MAX_WIRES    = 128;
static const unsigned int MAX_HITS     = 1280;

// ROOT documentation
// http://root.cern.ch/root/html/index.html

// class TFile;
// class TCanvas;
class TH1;
class TH2;
// class TH1I;
// class TH1F;
// class TH2F;
// class TProfile;
class TDirectory;
// class TObject;

namespace ots
{
struct TDCHeaderStruct
{
	unsigned int tdcSpillWordCount;
	unsigned int tdcNumber;
	unsigned int tdcSpillTriggerCount;
	unsigned int tdcSpillStatus;
	unsigned int tdcWords;
	unsigned int tdcHeaderIndex;
};

struct TDCEvent
{
	unsigned int              wordCount;
	unsigned int              tdcNumber;
	unsigned int              eventStatus;
	unsigned int              triggerNumber;  // 32 bit number
	unsigned int              triggerType;
	unsigned int              controllerEventTimeStamp;
	unsigned int              tdcEventTimeStamp;  // 32 bit number
	unsigned int              dataWords;
	unsigned int              words;
	std::vector<unsigned int> tdcData;
};

class ConfigurationTree;

class WireChamberDQMHistos
{
  public:
	WireChamberDQMHistos(void);
	virtual ~WireChamberDQMHistos(void);
	void book(TDirectory*              myDirectory,
	          const ConfigurationTree& theXDAQContextConfigTree,
	          const std::string&       configurationPath);
	void clear();
	void fill(std::string& buffer, std::map<std::string, std::string> header);
	void load(std::string fileName);
	void convertSpillData(const std::string& spillData);

  protected:
	std::vector<TDCHeaderStruct> vectorOfTDCHeaders_;
	std::vector<TDCEvent>        vectorOfTDCEvents_;
	unsigned int                 NUMBER_OF_TDCs = 16;

	TDirectory* myDir_;
	TDirectory* generalDir_;
	TDirectory* planesDir_;
	TH2*        h2_profile[MAX_CHAMBERS];
	TH1*        h_xtdc[MAX_CHAMBERS];
	TH1*        h_ytdc[MAX_CHAMBERS];
	TH1*        h_tdc[MAX_CHAMBERS][4];  // [chambers][module_in_chamber]
	TH1*        h_xslope;
	TH1*        h_xintercept;
	TH1*        h_yslope;
	TH1*        h_yintercept;
};
}

#endif
