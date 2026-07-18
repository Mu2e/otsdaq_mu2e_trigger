#include "otsdaq-mu2e-trigger/TablePlugins/DBServiceCIDTable.h"
#include "otsdaq-mu2e-trigger/TablePlugins/DBServiceTable.h"
#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"

#include <stdio.h>
#include <sys/stat.h>  //for mkdir
#include <fstream>     // std::fstream
#include <iostream>
#include <regex>

using namespace ots;

//Note: this could be used to inspect the path of the trigger offline db (declaration example from otsdaq-mu2e-calorimeter/otsdaq-mu2e-calorimeter/TablePlugins/SubsystemCalorimeterParametersTable_table.cc)
// const std::string SubsystemCalorimeterParametersTable::PATH_TO_TRIGGER_OFFLINE_DB = getenv("PATH_TO_TRIGGER_OFFLINE_DB") ? getenv("PATH_TO_TRIGGER_OFFLINE_DB") : "";

#define ARTDAQ_FCL_PATH \
	std::string(getenv("OTS_SCRATCH")) + "/TriggerConfigurations/"  // no longer used

#define DBSERVICE_VERBOSE \
	std::string(getenv("DBSERVICE_VERBOSE") ? getenv("DBSERVICE_VERBOSE") : "0")

// helpers
#define OUT out << tabStr << commentStr
#define PUSHTAB tabStr += "\t"
#define POPTAB tabStr.resize(tabStr.size() - 1)
#define PUSHCOMMENT commentStr += "# "
#define POPCOMMENT commentStr.resize(commentStr.size() - 2)

//========================================================================================================================
DBServiceTable::DBServiceTable(void) : TableBase("DBServiceTable")
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names used in C++ MUST match the Table INFO  //
	//////////////////////////////////////////////////////////////////////
	__COUTS__(10) << "[DBService::DBService] Initializing the "
	                 "DBServiceTable plugin..."
	              << __E__;
	//  exit(0);
	__COUTS__(10) << StringMacros::stackTrace() << __E__;
}  // end constructor

//========================================================================================================================
DBServiceTable::DBServiceTable(const std::string& tableName) : TableBase(tableName)
{
	__COUTS__(10) << "[DBService::DBService] Initializing the "
	                 "DBServiceTable plugin (as "
	              << tableName << ")..." << __E__;
	__COUTS__(10) << StringMacros::stackTrace() << __E__;
}  // end constructor (with table name)

//========================================================================================================================
DBServiceTable::~DBServiceTable(void) {}

//========================================================================================================================
void DBServiceTable::init(ConfigurationManager* configManager)
{
	isFirstAppInContext_ = configManager->isOwnerFirstAppInContext();

	__COUTV__(isFirstAppInContext_);
	if(!isFirstAppInContext_)
		return;

	if(0)
	{
		// make directory just in case
		mkdir((ARTDAQ_FCL_PATH).c_str(), 0755);

		std::string trigEpilogsDir;
		std::string fcl_dir = "TriggerEpilogs";
		trigEpilogsDir      = ARTDAQ_FCL_PATH + fcl_dir;
		mkdir(trigEpilogsDir.c_str(), 0755);
		std::string outFilename = trigEpilogsDir + "/" + "DBServiceInclude.fcl";

		__COUTS__(10) << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << std::endl;
		__COUTS__(10) << configManager->__SELF_NODE__ << std::endl;

		// Need to generate this:
		// 		services : {
		// 		  DbService : {
		// 		     purpose :  EMPTY
		// 		     version : v0   # ignored
		// 		     dbName : "mu2e_conditions_prd"  # ignored
		// 		     textFile : ["table.txt"]   # provide everything needed
		// 		     verbose : 1
		// 		  }
		// 		}

		// Process childrenMap to extract child pairs and write in required format
		std::ofstream outFile(outFilename);
		if(!outFile.is_open())
		{
			__SS__ << "Offline DBService output file could not be opened at path: "
			       << outFilename << __E__;
			__SS_THROW__;
		}
		outFile << "art.services.DbService : ";
		createTriggerFcl(outFile, configManager);
		outFile.close();
	}

}  // end init()

//========================================================================================================================
void DBServiceTable::createTriggerFcl(std::ostream&               outFile,
                                      const ConfigurationManager* configManager) const
{
	auto childrenMap = configManager->__SELF_NODE__.getChildren();

	std::string tabStr     = "";
	std::string commentStr = "";

	outFile << tabStr << "{" << std::endl;
	PUSHTAB;
	outFile << tabStr << "purpose :  EMPTY" << std::endl;
	outFile << tabStr << "version : v0   # ignored" << std::endl;
	outFile << tabStr << "dbName : \"mu2e_conditions_prd\"  # ignored" << std::endl;

	// Create textFile array with first_second.txt format
	outFile << tabStr << "textFile : [";
	bool first = true;
	for(const auto& pair : childrenMap)
	{
		if(!first)
		{
			outFile << ", ";
		}

		std::string cid = pair.second.getNode("CID").getValue<std::string>();
		// Empty/default/online CID means it uses an online generated table (no _CID suffix).
		bool includeCid =
		    !cid.empty() &&
		    !std::regex_match(
		        cid, std::regex("(default|online)", std::regex_constants::icase));

		outFile << "\"" << pair.first;
		if(includeCid)
			outFile << "_" << cid;
		outFile << ".txt\"";
		first = false;
	}
	outFile << "]   # provide everything needed" << std::endl;

	outFile << tabStr << "verbose : " << DBSERVICE_VERBOSE << std::endl;
	POPTAB;
	outFile << tabStr << "}" << std::endl;

}  //end createTriggerFcl()

//========================================================================================================================
std::string DBServiceTable::getFclValueForARTDAQ(
    const ConfigurationManager* configManager, const std::string& field) const
{
	if(field != "DbService")
	{
		__SS__ << "Unexpected field requested for getFclValueForARTDAQ()" << __E__;
		__SS_THROW__;
	}

	// Need to generate this
	//		  {
	// 		     purpose :  EMPTY
	// 		     version : v0   # ignored
	// 		     dbName : "mu2e_conditions_prd"  # ignored
	// 		     textFile : ["table.txt"]   # provide everything needed
	// 		     verbose : 1
	// 		  }

	std::stringstream ss;
	createTriggerFcl(ss, configManager);
	return ss.str();
}  //end getFclValueForARTDAQ()

//========================================================================================================================
std::string DBServiceTable::getStructureAsJSON(const ConfigurationManager* configManager)
{
	std::stringstream out;

	std::vector<std::pair<std::string, ConfigurationTree>> records =
	    configManager->getNode(getTableName()).getChildren();

	out << "[";

	bool firstRecord = true;

	for(auto& recordPair : records)
	{
		std::string name;
		std::string cid;
		const auto& recordName = recordPair.first;
		try
		{
			name = recordPair.second.getNode("Name").getValue();
		}
		catch(...)
		{
			__SS__ << "Missing required field 'Name' in " << getTableName() << " record '"
			       << recordName << "'." << __E__;
			__SS_THROW__;
		}
		try
		{
			cid = recordPair.second.getNode("CID").getValue();
		}
		catch(...)
		{
			__SS__ << "Missing required field 'CID' in " << getTableName() << " record '"
			       << recordName << "'." << __E__;
			__SS_THROW__;
		}
		if(!firstRecord)
		{
			out << ", ";
		}
		out << "{";
		out << "\"name\": \"" << StringMacros::escapeJSONStringEntities(name) << "\",";
		out << "\"cid\": \"" << StringMacros::escapeJSONStringEntities(cid) << "\"";
		out << "}";
		firstRecord = false;
	}
	out << "]";

	return out.str();
}  // end getStructureAsJSON()

//========================================================================================================================
DBServiceCIDTable::DBServiceCIDTable(void)
    : DBServiceTable("DBServiceCIDTable") {}  // end constructor

//========================================================================================================================
DBServiceCIDTable::~DBServiceCIDTable(void) {}
