#include "otsdaq-mu2e-trigger/TablePluginDataFormats/TriggerTableFromJSON.h"
#include "otsdaq/Macros/TablePluginMacros.h"
#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
//#include "otsdaq/tools/otsdaq_load_json_document.cc"

#include <iostream>
#include <fstream>      // std::fstream
#include <stdio.h>
#include <sys/stat.h> 	//for mkdir
#include <regex>

using namespace ots;


#define ARTDAQ_FCL_PATH			std::string(getenv("OTS_SCRATCH")) + "/TriggerConfigurations/"
#define ARTDAQ_FILE_PREAMBLE	"boardReader"

//helpers
#define OUT						out << tabStr << commentStr
#define PUSHTAB					tabStr += "\t"
#define POPTAB					tabStr.resize(tabStr.size()-1)
#define PUSHCOMMENT				commentStr += "# "
#define POPCOMMENT				commentStr.resize(commentStr.size()-2)

#undef __COUT__
#define __COUT__ 			__COUT_TYPE__(TLVL_DEBUG+10) << __COUT_HDR__

//========================================================================================================================
TriggerTableFromJSON::TriggerTableFromJSON(void)
  : TableBase("TriggerTableFromJSON")
{
  //////////////////////////////////////////////////////////////////////
  //WARNING: the names used in C++ MUST match the Table INFO  //
  //////////////////////////////////////////////////////////////////////
  __COUT__ <<"[TriggerTableFromJSON::TriggerTableFromJSON] Initializing the TriggerTableFromJSON plugin..." << __E__;
  //  exit(0);
  __COUT__ << StringMacros::stackTrace() << __E__;
} //end constructor

//========================================================================================================================
TriggerTableFromJSON::~TriggerTableFromJSON(void)
{}

//========================================================================================================================
void TriggerTableFromJSON::init(ConfigurationManager* configManager)
{
  isFirstAppInContext_ = configManager->isOwnerFirstAppInContext();
	
  __COUTV__(isFirstAppInContext_);
  if(!isFirstAppInContext_) return;

  //make directory just in case
  mkdir((ARTDAQ_FCL_PATH).c_str(), 0755);

  std::string      trigEpilogsDir;
  std::string      fcl_dir = "TriggerEpilogs";
  trigEpilogsDir = ARTDAQ_FCL_PATH + fcl_dir;
  mkdir(trigEpilogsDir.c_str(), 0755);

  //now download from MONGO-Db the trigger table to be used
  std::string    getTableFromMongoDb = "otsdaq_load_json_document ";
  std::string    triggerTableName    = " testTriggerDoc ";
  std::string    triggerTableVersion = " 0 ";
  std::string    outputFileName      = ARTDAQ_FCL_PATH + "trigger_table.json";
  
  getTableFromMongoDb += triggerTableName + triggerTableVersion + triggerTableVersion + outputFileName;
  system(getTableFromMongoDb.c_str());
  

  __COUT__ << StringMacros::stackTrace() << __E__;

  std::string      command  = "python mu2e_trig_config/python/generateMenuFromJSON.py";
  std::string      menuFile = " -mf " + outputFileName;//"mu2e_trig_config/data/physMenu.json";
  std::string      output   = " -o " + trigEpilogsDir;
  std::string      evtMode  = " -evtMode All";
  
  command += menuFile + output + evtMode;
  system(command.c_str());

  // //create the fcl file
  // std::ofstream      triggerFclFile, epilogFclFile, subEpilogFclFile, allPathsFile;
  // std::string        fclFileName, skelethonName, allPathsFileName;
  // std::string        epilogName;
  // //skelethon to clone
  // //	skelethonName = Form("%s/main.fcl", (ARTDAQ_FCL_PATH).c_str());
  // skelethonName = ARTDAQ_FCL_PATH + "/main.fcl" ;
  // //file to be edited
  // //	fclFileName = Form("%s/runTriggerExample.fcl", (ARTDAQ_FCL_PATH).c_str());
  // fclFileName = ARTDAQ_FCL_PATH+"/runTriggerExample.fcl";

  // //file that will house all the includes necessary to run the trigger paths
  // allPathsFileName = trigEpilogsDir+ "/allPaths.fcl";

  // std::ifstream      mainFclFile;
  // mainFclFile   .open(skelethonName);
  // triggerFclFile.open(fclFileName);
  // allPathsFile  .open(allPathsFileName);

  // std::string        line;
  // while (std::getline(mainFclFile, line, '\n') ) triggerFclFile << line << '\n';

  // //we need to append the line where include the fcl that will contain all the trigger paths
  // triggerFclFile << "#include \"Trigger_epilogs/allPaths.fcl\""<<__E__; 

  // __COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << std::endl;
  // __COUT__ << configManager->__SELF_NODE__ << std::endl;

  // auto childrenMap = configManager->__SELF_NODE__.getChildren();
  // __COUT__ <<"printing children content"<<__E__;
  // __COUT__ <<"children map size"<<childrenMap.size() << __E__;

  // for (auto &topLevelPair : childrenMap)
  //   {
  //     __COUT__       << "Main table name '" << topLevelPair.first << "'" << __E__;
  //     auto triggerPaths = topLevelPair.second.getNode("LinkToTriggerPathsTable").getChildren();
  //     for (auto &triggerPathPair : triggerPaths)
  // 	{
  // 	  __COUT__       << "children LOOP" << __E__;		
  // 	  __COUT__       << "children Path '" << triggerPathPair.first << "'" << __E__;
  // 	}
  //   }
  // __COUT__ <<"children map printed..." <<__E__;

  // for (auto &topLevelPair : childrenMap)
  //   {
  //     __COUT__       << "top LOOP" << __E__;
  //     __COUT__       << "Top Level '" << topLevelPair.first << "'" << __E__;
  //     // triggerFclFile << "Top Level '" << topLevelPair.first << "'" << __E__; 
  //     // triggerFclFile << "Node name '" << topLevelPair.second << "'" << __E__; 

  //     auto triggerPaths = topLevelPair.second.getNode("LinkToTriggerPathsTable").getChildren();
  //     size_t children_map_size = triggerPaths.size();
  //     size_t counter(0);
  //     std::vector<int> list_of_pathIDs;

  //     for (auto &triggerPathPair : triggerPaths)
  // 	{
  // 	  __COUT__       << "internal LOOP" << __E__;		
  // 	  __COUT__       << "Trigger Path '" << triggerPathPair.first << "'" << __E__;
  // 	  __COUT__       << "Trigger Name '" << triggerPathPair.second.getNode("TriggerName").getValue() << "'" << __E__;

  // 	  std::string trigger_status = triggerPathPair.second.getNode("Status").getValue();
  // 	  if (trigger_status == "Off"){
  // 	    __COUT__       << "Trigger status is Off" << __E__;
  // 	    continue;
  // 	  }
  // 	  ots::ConfigurationTree singlePath = triggerPathPair.second.getNode("LinkToTriggerTable");
  // 	  __COUT__       << "singlePath : " << singlePath << __E__;
  // 	  __COUT__       << "singlePath.isDisconnected : " << singlePath.isDisconnected() << __E__;
  // 	  // __COUT__       << "singlePath.getNode : " << StringMacros::vectorToString(singlePath.getChildrenNames()) << __E__;
  // 	  // __COUT__       << "singlePath.getConfigurationManager " << singlePath.getConfigurationManager() << __E__;
  // 	  // __COUT__       << "singlePath.getConfigurationManager()->getTableByName " << singlePath.getConfigurationManager()->getTableByName("TriggerParameterTable") << __E__;


  // 	  std::string  triggerType = triggerPathPair.second.getNode("TriggerType").getValue<std::string>();
  // 	  int          pathID      = triggerPathPair.second.getNode("PathID").getValue<int>();

  // 	  __COUT__       << "Trigger Type '" << triggerType << "'" << __E__;
	  
  // 	  //create the fcl housing the trigger-path configurations
  // 	  epilogName = trigEpilogsDir + "/" + triggerPathPair.first + ".fcl";
  // 	  allPathsFile << "#include \"Trigger_epilogs/" << triggerPathPair.first<<".fcl\"" << __E__; 
  // 	  //we need to append the line where we instantiate the given TriggerPath
  // 	  //	  allPathsFile << "art.physics." << triggerPathPair.first  << "_trigger  : [ @sequence::Trigger.paths."<< triggerPathPair.first<< " ]\n" << __E__; 
  // 	  // allPathsFile << "art.physics." << triggerPathPair.first  << "_trigger  : [ makeSD, CaloDigiMaker, @sequence::Trigger.paths."<< triggerPathPair.first<< " ]\n" << __E__; 
  // 	  //allPathsFile << "art.physics." << triggerPathPair.first  << "_trigger  : [ dtcEventVerifier, artFragFromDTCEvents, makeSD, @sequence::Trigger.paths."<< triggerPathPair.first<< " ]" << __E__; 
  // 	  allPathsFile << "art.physics." << triggerPathPair.first  << "_trigger  : [ artFragFromDTCEvents, makeSD, @sequence::Trigger.paths."<< triggerPathPair.first<< " ]" << __E__; 
  // 	  allPathsFile << "art.physics.trigger_paths["<< pathID <<"] : " << triggerPathPair.first  << "_trigger \n"<< __E__; 
	  
  // 	  epilogFclFile.open(epilogName.c_str());

  // 	  //check if the pathID is already usd by another trigger chain
  // 	  for (auto & id : list_of_pathIDs){
  // 	    if (id == pathID){
  // 	      __SS__ << "Attempt to use twice the same PathID : "<< pathID << std::endl;
  // 	      __COUT_ERR__ << ss.str() << std::endl;
  // 	      __SS_THROW__;
  // 	    }
  // 	  }

  // 	  //create the directory that will house all the epilogs of a given triggerPath
  // 	  std::string               singlePathEpilogsDir, singlePathPairFclName;
  // 	  //		singlePathEpilogsDir = "%sTrigger_epilogs/%s", (ARTDAQ_FCL_PATH).c_str(), triggerPathPair.first.c_str());
  // 	  singlePathEpilogsDir = ARTDAQ_FCL_PATH + "Trigger_epilogs/" + triggerPathPair.first;
  // 	  mkdir(singlePathEpilogsDir.c_str(), 0755);
  // 	  __COUT__       << "single path epilogs dir " << singlePathEpilogsDir << __E__; 

  // 	  //set the general prescale factor at the beginning of the path
  // 	  createPrescaleEpilog         (epilogFclFile, singlePathEpilogsDir, triggerPathPair.first, triggerPathPair.second); //singlePath);

  // 	  if (triggerType == "TrackSeed")
  // 	    {
  // 	      //to set up the Tracking filters we need to loop over the children of the corresponding node
  // 	      createTrackingFiltersEpilog  (epilogFclFile, singlePathEpilogsDir, triggerPathPair.first, singlePath);
  // 	    }
  // 	  else if (triggerType == "Helix")
  // 	    {
  // 	      singlePath.getNode("LinkToDigiFilterParameterTable");
  // 	      createHelixFiltersEpilog  (epilogFclFile, singlePathEpilogsDir, triggerPathPair.first, singlePath);
  // 	    }
  // 	  else if (triggerType == "DigiCount")
  // 	    {
  // 	      createDigiCountFiltersEpilog  (epilogFclFile, singlePathEpilogsDir, triggerPathPair.first, singlePath);		    
  // 	    }

  // 	  __COUT__ <<" closing epilogFclFile" <<__E__;
		  
  // 	  epilogFclFile.close();
  // 	  __COUT__ << "epilogFclFile closed... " << __E__;
		
  // 	  ++counter;
  // 	  __COUT__ <<"counter = " << counter <<", map-size = " << children_map_size <<__E__;
  // 	  // if (counter == children_map_size-2) break;
  // 	}//end loop over triggerPathPair
	    
  //     __COUT__ <<" end of main LOOP" <<__E__;
	   
  //   }//end loop over topLevelPair

  // __COUT__ << "loop completed closed... " << __E__;


  // triggerFclFile.close();
  // __COUT__ << "triggerFclFile closed... " << __E__;

}


//----------------------------------------------------------------------------------------------------
// this function creates a string usde to set the module names in a given trigPath
//----------------------------------------------------------------------------------------------------
std::string TriggerTableFromJSON::GetModuleNameFromPath(std::string &TrigPath)
{
  std::string del("_");
  std::string name(TrigPath);
  std::string newName = "";
  __COUT__     << "TrigPath = " << TrigPath << __E__;
  
  auto pos = name.find(del);
  if(pos == std::string::npos) {
    newName = name;
  }else {
    newName = name.substr(0,pos);
    name.erase(0, pos+del.length());
    __COUT__     << "newName = " <<newName << ", name = "<< name<< __E__;
    
    
    //while(name.length() >0)
    do
      {
	auto pos = name.find(del);
	std::string token = name.substr(0, pos);
	if ( (pos == std::string::npos) && (name.length()>0)){
	  token = name;
	  name  = "";
	}
	if ( (newName != "") && (token.find("timing") == std::string::npos) ) //convert to upper case the first letter
	  {
	    token[0] = token[0] - 32;
	    newName += token;
	  }
	name.erase(0, pos+del.length());
	__COUT__     << "[while ] newName = " <<newName << ", name = "<< name<< __E__;

      }     while(name.length() >0); //name.find(del) != std::string::npos);
  }
  return newName;
}


DEFINE_OTS_TABLE(TriggerTableFromJSON)
