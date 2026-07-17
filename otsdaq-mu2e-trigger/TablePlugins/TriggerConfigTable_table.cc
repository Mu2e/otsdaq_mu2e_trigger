#include "otsdaq-mu2e-trigger/TablePlugins/TriggerConfigTable.h"
#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/Macros/StringMacros.h"
#include "otsdaq/Macros/TablePluginMacros.h"
//#include "otsdaq/tools/otsdaq_load_json_document.cc"

#include <stdio.h>
#include <sys/stat.h>  //for mkdir
#include <fstream>     // std::fstream
#include <iostream>
#include <regex>
#include <sstream>

using namespace ots;

// clang-format off
#define ARTDAQ_FCL_PATH 		__ENV__("OTS_SCRATCH") + std::string("/TriggerConfigurations/")
#define ARTDAQ_FILE_PREAMBLE 	"boardReader"

//helpers
#define OUT out << tabStr << commentStr
#define PUSHTAB tabStr += "\t"
#define POPTAB tabStr.resize(tabStr.size() - 1)
#define PUSHCOMMENT commentStr += "# "
#define POPCOMMENT commentStr.resize(commentStr.size() - 2)
// clang-format on

//========================================================================================================================
TriggerConfigTable::TriggerConfigTable(void) : TableBase("TriggerConfigTable")
{
	//////////////////////////////////////////////////////////////////////
	//WARNING: the names used in C++ MUST match the Table INFO  //
	//////////////////////////////////////////////////////////////////////
	__COUTS__(10) << "[TriggerConfigTable::TriggerConfigTable] Initializing the "
	                 "TriggerConfigTable plugin..."
	              << __E__;
	//  exit(0);
	// SC: 2/9/2026 commented out the line below
	// the line below is really confusing when debugging because it looks like an error
	//__COUTS__(10) << StringMacros::stackTrace() << __E__;
}  //end constructor

//========================================================================================================================
TriggerConfigTable::~TriggerConfigTable(void) {}

//========================================================================================================================
void TriggerConfigTable::init(ConfigurationManager* configManager)
{
	__COUTS__(10) << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << __E__;
	__COUTS__(10) << configManager->__SELF_NODE__ << __E__;

	isFirstAppInContext_ = configManager->isOwnerFirstAppInContext();

	__COUTV__(isFirstAppInContext_);
	if(!isFirstAppInContext_)
		return;
}  //end init()

//========================================================================================================================
void TriggerConfigTable::initPrereqsForARTDAQ(const ConfigurationManager* configManager)
{
	__COUTS__(50) << "initPrereqsForARTDAQ()" << __E__;
	__COUTS__(51) << StringMacros::stackTrace() << __E__;

	bool needToGenerate = false;
	//lock for scope to check if need to generate in this thread
	std::lock_guard<std::mutex> lock(prereqsGeneratedMutex_);
	if(!prereqsGenerated_)
		needToGenerate = true;

	if(!needToGenerate)  //already generated in another call; nothing to do
	{
		__COUTS__(10) << "initPrereqsForARTDAQ() already generated!" << __E__;
		return;  //done!
	}

	__COUTS__(10) << "initPrereqsForARTDAQ() generating!" << __E__;
	__COUTS__(11) << StringMacros::stackTrace() << __E__;

	//make directory just in case
	mkdir((ARTDAQ_FCL_PATH).c_str(), 0755);

	auto childrenMap = configManager->__SELF_NODE__.getChildren();
	__COUTS__(10) << "children map size" << childrenMap.size() << __E__;

	if(childrenMap.empty())
	{
		__SS__ << "There is no record in the table '"
		       << configManager->__SELF_NODE__.getTableName()
		       << "' - the first record is required to define the Trigger Menu document "
		          "and tag!"
		       << __E__;
		__SS_THROW__;
	}

	// Seed artdaq system variable defaults for trigger menu fields
	// so ${OTS.artdaq.triggerMenuName} and ${OTS.artdaq.triggerMenuTag}
	// resolve to empty if not set by the user via the web GUI.
	{
		auto& ns = StringMacros::systemVariables_["artdaq"];
		if(ns.find("triggerMenuName") == ns.end())
			ns["triggerMenuName"] = "";
		if(ns.find("triggerMenuTag") == ns.end())
			ns["triggerMenuTag"] = "";
	}

	auto& topLevelPair = childrenMap.at(0);
	__COUTS__(10) << "Main table name '" << topLevelPair.first << "'" << __E__;

	//now download from MONGO-Db the trigger table to be used
	std::string triggerTableName =
	    topLevelPair.second.getNode("TriggerDocName").getValue();  //" testTriggerDoc ";
	std::string triggerTableVersion =
	    topLevelPair.second.getNode("TriggerConfigTag").getValue();  //" 6 ";

	if(triggerTableName.empty() || triggerTableVersion.empty())
	{
		__SS__ << "Empty trigger menu name '" << triggerTableName << "' and/or tag '"
		       << triggerTableVersion << "' from the '" << topLevelPair.first
		       << "' record fields TriggerDocName/TriggerConfigTag! If these fields "
		          "reference system variables like ${OTS.artdaq.triggerMenuName}, "
		          "select the trigger menu via the Trigger Menu Editor web GUI "
		          "before configuring."
		       << __E__;
		__SS_THROW__;
	}

	generateTriggerEpilogs(triggerTableName, triggerTableVersion);

	prereqsGenerated_ = true;
}  //end initPrereqsForARTDAQ()

//========================================================================================================================
std::string TriggerConfigTable::getPhysicsMenuJsonFileName(
    const std::string& triggerTableName, const std::string& triggerTableVersion)
{
	// For now, always use physicsMenu.json
	// Can be extended in the future to use:
	// return triggerTableName + "-v" + triggerTableVersion + ".json";
	return "physMenu.json";
}  // end getPhysicsMenuJsonFileName()

//========================================================================================================================
std::string TriggerConfigTable::getStructureAsJSON(
    const ConfigurationManager* configManager)
{
	std::stringstream out;

	std::vector<std::pair<std::string, ConfigurationTree>> records =
	    configManager->getNode(getTableName()).getChildren();

	std::string triggerDoc;
	std::string triggerTag;
	std::string menuFileContent;
	std::string recordName;
	if(!records.empty())
	{
		auto& recordPair = records.at(0);
		recordName       = recordPair.first;
		try
		{
			triggerDoc = recordPair.second.getNode("TriggerDocName").getValue();
		}
		catch(...)
		{
			triggerDoc = "";
		}
		try
		{
			triggerTag = recordPair.second.getNode("TriggerConfigTag").getValue();
		}
		catch(...)
		{
			triggerTag = "";
		}
	}

	// Get the physics menu JSON filename
	std::string menuFilePath =
	    ARTDAQ_FCL_PATH + getPhysicsMenuJsonFileName(triggerDoc, triggerTag);

	// Download trigger menu from MongoDB (in case we're on a different machine)
	downloadTriggerMenuFromMongoDB(triggerDoc, triggerTag, menuFilePath);

	// Read physics menu JSON content
	{
		std::ifstream menuFile(menuFilePath, std::ios::in);
		if(!menuFile.good())
		{
			__SS__ << "Could not open menu file: " << menuFilePath
			       << " - Make sure initPrereqsForARTDAQ() was called successfully!"
			       << __E__;
			__SS_THROW__;
		}

		std::stringstream menuBuffer;
		menuBuffer << menuFile.rdbuf();
		menuFileContent = menuBuffer.str();
		menuFile.close();

		__COUT__ << "Read " << menuFileContent.size() << " bytes from menu file" << __E__;

		// Trim whitespace
		menuFileContent.erase(0, menuFileContent.find_first_not_of(" \t\n\r"));
		menuFileContent.erase(menuFileContent.find_last_not_of(" \t\n\r") + 1);

		if(menuFileContent.empty())
		{
			__SS__ << "Menu file is empty: " << menuFilePath << __E__;
			__SS_THROW__;
		}

		// Validate it starts with { or [ (basic JSON check)
		if(menuFileContent[0] != '{' && menuFileContent[0] != '[')
		{
			__SS__ << "Menu file content doesn't start with valid JSON: "
			       << menuFileContent.substr(0, 50) << __E__;
			__SS_THROW__;
		}

		// Check for null bytes or other control characters that could break JSON
		for(size_t i = 0; i < menuFileContent.size(); ++i)
		{
			unsigned char c = menuFileContent[i];
			if(c == 0)  // null byte
			{
				__SS__ << "Found null byte at position " << i << " in menu file!"
				       << __E__;
				__SS_THROW__;
			}
		}
	}

	out << "{";
	out << "\"alias\": \"" << StringMacros::escapeJSONStringEntities(recordName) << "\",";
	out << "\"name\": \"" << StringMacros::escapeJSONStringEntities(triggerDoc) << "\",";
	out << "\"version\": \"" << StringMacros::escapeJSONStringEntities(triggerTag)
	    << "\",";
	//out << "\"file\": \""
	//    << StringMacros::escapeJSONStringEntities(menuFilePath) << "\",";
	out << "\"content\": " << menuFileContent;
	out << "}";

	return out.str();
}  // end getStructureAsJSON()

//========================================================================================================================
void TriggerConfigTable::downloadTriggerMenuFromMongoDB(
    const std::string& triggerTableName,
    const std::string& triggerTableVersion,
    const std::string& outputFileName)
{
	__COUT__ << "downloadTriggerMenuFromMongoDB() for " << triggerTableName << " v"
	         << triggerTableVersion << __E__;

	//sanitize values for system call
	for(size_t c = 0; c < triggerTableName.size(); ++c)
		if(!((triggerTableName[c] >= 'a' && triggerTableName[c] <= 'z') ||
		     (triggerTableName[c] >= 'A' && triggerTableName[c] <= 'Z') ||
		     (triggerTableName[c] >= '0' && triggerTableName[c] <= '9') ||
		     triggerTableName[c] == '_' || triggerTableName[c] == '-'))
		{
			__SS__ << "Illegal character found in triggerTableName '" << triggerTableName
			       << "' ... only alpha-numeric, dashes, and underscores allowed."
			       << __E__;
			__SS_THROW__;
		}

	for(size_t c = 0; c < triggerTableVersion.size(); ++c)
		if(!((triggerTableVersion[c] >= '0' && triggerTableVersion[c] <= '9')))
		{
			__SS__ << "Illegal character found in triggerTableVersion '"
			       << triggerTableVersion << "' ... only numeric characters allowed."
			       << __E__;
			__SS_THROW__;
		}

	// Sanitize outputFileName for system call
	// Allow only alpha-numeric characters, dashes, underscores, dots, and slashes
	for(size_t c = 0; c < outputFileName.size(); ++c)
		if(!((outputFileName[c] >= 'a' && outputFileName[c] <= 'z') ||
		     (outputFileName[c] >= 'A' && outputFileName[c] <= 'Z') ||
		     (outputFileName[c] >= '0' && outputFileName[c] <= '9') ||
		     outputFileName[c] == '_' || outputFileName[c] == '-' ||
		     outputFileName[c] == '.' || outputFileName[c] == '/'))
		{
			__SS__ << "Illegal character found in outputFileName '" << outputFileName
			       << "' ... only alpha-numeric characters, dashes, underscores, dots, "
			          "and slashes allowed."
			       << __E__;
			__SS_THROW__;
		}
	std::string getTableFromMongoDb = "otsdaq_load_json_document ";
	getTableFromMongoDb +=
	    triggerTableName + " " + triggerTableVersion + " " + outputFileName;
	__COUTS__(10) << "otsdaq_load_json command: " << getTableFromMongoDb << __E__;

	// Remove old output file to ensure we detect a fresh one
	remove(outputFileName.c_str());

	std::string loadJsonResult = StringMacros::exec(getTableFromMongoDb.c_str());
	__COUTVS__(10, loadJsonResult.size());
	__COUT_MULTI__(10, loadJsonResult);

	// Verify the output file was created
	std::ifstream checkOutputFile(outputFileName);
	if(!checkOutputFile.good())
	{
		__SS__ << "otsdaq_load_json_document failed to create output file '"
		       << outputFileName << "'\n"
		       << "Command: " << getTableFromMongoDb << "\n"
		       << "Output:\n"
		       << loadJsonResult << __E__;
		__SS_THROW__;
	}
}  // end downloadTriggerMenuFromMongoDB()

//========================================================================================================================
void TriggerConfigTable::generateTriggerEpilogs(const std::string& triggerTableName,
                                                const std::string& triggerTableVersion)
{
	__COUT__ << "generateTriggerEpilogs() for " << triggerTableName << " v"
	         << triggerTableVersion << __E__;

	// Create trigger epilogs directory
	std::string fcl_dir        = "TriggerEpilogs";
	std::string trigEpilogsDir = ARTDAQ_FCL_PATH + fcl_dir;
	mkdir(trigEpilogsDir.c_str(), 0755);

	std::string outputFileName =
	    ARTDAQ_FCL_PATH +
	    getPhysicsMenuJsonFileName(triggerTableName, triggerTableVersion);

	// Download the trigger menu from MongoDB
	downloadTriggerMenuFromMongoDB(triggerTableName, triggerTableVersion, outputFileName);

	std::string menuFile = " -mf " + outputFileName;
	std::string output   = " -o " + trigEpilogsDir;
	std::string evtMode  = " -evtMode all";

	std::string command = "generateMenuFromJSON.py" + menuFile + output + evtMode;
	__COUTS__(10) << "generateMenuFromJSON command: " << command << __E__;
	const std::string exitMarker    = "__OTS_CMD_EXIT__=";
	std::string       cmdWithStatus = command + " ; echo " + exitMarker + "$?";
	std::string       genMenuResult = StringMacros::exec(cmdWithStatus.c_str());

	int  exitCode  = -1;
	auto markerPos = genMenuResult.rfind(exitMarker);
	if(markerPos != std::string::npos)
	{
		std::string codeStr = genMenuResult.substr(markerPos + exitMarker.size());
		codeStr.erase(0, codeStr.find_first_not_of(" \t\r\n"));
		codeStr.erase(codeStr.find_last_not_of(" \t\r\n") + 1);
		exitCode = std::atoi(codeStr.c_str());
		genMenuResult.erase(markerPos);
		// trim trailing whitespace after removing marker
		genMenuResult.erase(genMenuResult.find_last_not_of(" \t\r\n") + 1);
	}

	__COUTVS__(10, genMenuResult.size());
	__COUT_MULTI__(10, genMenuResult);

	if(exitCode != 0)
	{
		__SS__ << "generateMenuFromJSON failed (exit code " << exitCode << ")\n"
		       << "Command: " << command << "\n"
		       << "Output:\n"
		       << genMenuResult << __E__;
		__SS_THROW__;
	}
}  //end generateTriggerEpilogs()

DEFINE_OTS_TABLE(TriggerConfigTable)
