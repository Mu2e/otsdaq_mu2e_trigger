#ifndef _ots_TopLevelTriggerTable_h_
#define _ots_TopLevelTriggerTable_h_

#include "otsdaq/TableCore/TableBase.h"
#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include <string>
#include <iostream>
#include <fstream>      // std::fstream

namespace ots
{

  class TopLevelTriggerTable : public TableBase
{

public:

	TopLevelTriggerTable(void);
	virtual ~TopLevelTriggerTable(void);

	//Methods
	void   init                          (ConfigurationManager *configManager);
	void   createPrescaleEpilog          (std::ofstream& EpilogFclFile, std::string& EpilogDir, std::string& TrigPath, ots::ConfigurationTree  ConfTree);
	void   createTrackingFiltersEpilog   (std::ofstream& EpilogFclFile, std::string& EpilogDir, std::string& TrigPath, ots::ConfigurationTree  ConfTree);
	void   createHelixFiltersEpilog      (std::ofstream& EpilogFclFile, std::string& EpilogDir, std::string& TrigPath, ots::ConfigurationTree  ConfTree);
	void   createDigiCountFiltersEpilog  (std::ofstream& EpilogFclFile, std::string& EpilogDir, std::string& TrigPath, ots::ConfigurationTree  ConfTree);

private:
	bool 	isFirstAppInContext_;
};
}
#endif
