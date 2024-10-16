#ifndef _ots_TriggerConfigTable_h_
#define _ots_TriggerConfigTable_h_

#include "otsdaq/TableCore/TableBase.h"
#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include <string>
#include <iostream>
#include <fstream>      // std::fstream

namespace ots
{

  class TriggerConfigTable : public TableBase
{

public:

	TriggerConfigTable(void);
	virtual ~TriggerConfigTable(void);

	//Methods
	void   init                          (ConfigurationManager *configManager);
	void    createTriggerMenuFiles        (std::ofstream& EpilogFclFile, std::string& EpilogDir, std::string& TrigPath, ots::ConfigurationTree  ConfTree);	
	std::string GetModuleNameFromPath    (std::string &TrigPath);


private:
	bool 	isFirstAppInContext_;
};
}
#endif
