#include "otsdaq-mu2e-trigger/TablePluginDataFormats/TopLevelTriggerTable.h"
#include "otsdaq/Macros/TablePluginMacros.h"
#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"

#include <iostream>
#include <fstream>      // std::fstream
#include <stdio.h>
#include <sys/stat.h> 	//for mkdir
#include <regex>

using namespace ots;


#define ARTDAQ_FCL_PATH			std::string(getenv("OTS_SCRATCH")) + "/"+ "TriggerConfigurations/"
#define ARTDAQ_FILE_PREAMBLE	"boardReader"

//helpers
#define OUT						out << tabStr << commentStr
#define PUSHTAB					tabStr += "\t"
#define POPTAB					tabStr.resize(tabStr.size()-1)
#define PUSHCOMMENT				commentStr += "# "
#define POPCOMMENT				commentStr.resize(commentStr.size()-2)


//========================================================================================================================
TopLevelTriggerTable::TopLevelTriggerTable(void)
  : TableBase("TopLevelTriggerTable")
{
  //////////////////////////////////////////////////////////////////////
  //WARNING: the names used in C++ MUST match the Table INFO  //
  //////////////////////////////////////////////////////////////////////
  __COUT__ <<"hi" << __E__;
  __COUT__ <<"hi" << __E__;
  __COUT__ <<"hi" << __E__;
  __COUT__ <<"hi" << __E__;
  __COUT__ <<"hi" << __E__;
  __COUT__ <<"hi" << __E__;
  //  exit(0);
  __COUT__ << StringMacros::stackTrace() << __E__;
} //end constructor

//========================================================================================================================
TopLevelTriggerTable::~TopLevelTriggerTable(void)
{}

//========================================================================================================================
void TopLevelTriggerTable::init(ConfigurationManager* configManager)
{
  isFirstAppInContext_ = configManager->isOwnerFirstAppInContext();
	
  __COUTV__(isFirstAppInContext_);
  if(!isFirstAppInContext_) return;

  //make directory just in case
  mkdir((ARTDAQ_FCL_PATH).c_str(), 0755);

  std::string      trigEpilogsDir;
  trigEpilogsDir = ARTDAQ_FCL_PATH + "Trigger_epilogs";
  mkdir(trigEpilogsDir.c_str(), 0755);

  __COUT__ << StringMacros::stackTrace() << __E__;


  //create the fcl file
  std::ofstream      triggerFclFile, epilogFclFile, subEpilogFclFile, allPathsFile;
  std::string        fclFileName, skelethonName, allPathsFileName;
  std::string        epilogName;
  //skelethon to clone
  //	skelethonName = Form("%s/main.fcl", (ARTDAQ_FCL_PATH).c_str());
  skelethonName = ARTDAQ_FCL_PATH + "/main.fcl" ;
  //file to be edited
  //	fclFileName = Form("%s/runTriggerExample.fcl", (ARTDAQ_FCL_PATH).c_str());
  fclFileName = ARTDAQ_FCL_PATH+"/runTriggerExample.fcl";

  //file that will house all the includes necessary to run the trigger paths
  allPathsFileName = trigEpilogsDir+ "/allPaths.fcl";

  std::ifstream      mainFclFile;
  mainFclFile   .open(skelethonName);
  triggerFclFile.open(fclFileName);
  allPathsFile  .open(allPathsFileName);

  std::string        line;
  while (std::getline(mainFclFile, line, '\n') ) triggerFclFile << line << '\n';

  //we need to append the line where include the fcl that will contain all the trigger paths
  triggerFclFile << "#include \"Trigger_epilogs/allPaths.fcl\""<<__E__; 

  __COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << std::endl;
  __COUT__ << configManager->__SELF_NODE__ << std::endl;

  auto childrenMap = configManager->__SELF_NODE__.getChildren();
  __COUT__ <<"printing children content"<<__E__;
  __COUT__ <<"children map size"<<childrenMap.size() << __E__;

  for (auto &topLevelPair : childrenMap)
    {
      __COUT__       << "Main table name '" << topLevelPair.first << "'" << __E__;
      auto triggerPaths = topLevelPair.second.getNode("LinkToTriggerPathsTable").getChildren();
      for (auto &triggerPathPair : triggerPaths)
	{
	  __COUT__       << "children LOOP" << __E__;		
	  __COUT__       << "children Path '" << triggerPathPair.first << "'" << __E__;
	}
    }
  __COUT__ <<"children map printed..." <<__E__;

  for (auto &topLevelPair : childrenMap)
    {
      __COUT__       << "top LOOP" << __E__;
      __COUT__       << "Top Level '" << topLevelPair.first << "'" << __E__;
      // triggerFclFile << "Top Level '" << topLevelPair.first << "'" << __E__; 
      // triggerFclFile << "Node name '" << topLevelPair.second << "'" << __E__; 

      auto triggerPaths = topLevelPair.second.getNode("LinkToTriggerPathsTable").getChildren();
      size_t children_map_size = triggerPaths.size();
      size_t counter(0);
      std::vector<int> list_of_pathIDs;

      for (auto &triggerPathPair : triggerPaths)
	{
	  __COUT__       << "internal LOOP" << __E__;		
	  __COUT__       << "Trigger Path '" << triggerPathPair.first << "'" << __E__;
	  __COUT__       << "Trigger Name '" << triggerPathPair.second.getNode("TriggerName").getValue() << "'" << __E__;

	  std::string trigger_status = triggerPathPair.second.getNode("Status").getValue();
	  if (trigger_status == "Off"){
	    __COUT__       << "Trigger status is Off" << __E__;
	    continue;
	  }
	  ots::ConfigurationTree singlePath = triggerPathPair.second.getNode("LinkToTriggerTable");
	  __COUT__       << "singlePath : " << singlePath << __E__;
	  __COUT__       << "singlePath.isDisconnected : " << singlePath.isDisconnected() << __E__;
	  // __COUT__       << "singlePath.getNode : " << StringMacros::vectorToString(singlePath.getChildrenNames()) << __E__;
	  // __COUT__       << "singlePath.getConfigurationManager " << singlePath.getConfigurationManager() << __E__;
	  // __COUT__       << "singlePath.getConfigurationManager()->getTableByName " << singlePath.getConfigurationManager()->getTableByName("TriggerParameterTable") << __E__;


	  std::string  triggerType = triggerPathPair.second.getNode("TriggerType").getValue<std::string>();
	  int          pathID      = triggerPathPair.second.getNode("PathID").getValue<int>();

	  __COUT__       << "Trigger Type '" << triggerType << "'" << __E__;
	  
	  //create the fcl housing the trigger-path configurations
	  epilogName = trigEpilogsDir + "/" + triggerPathPair.first + ".fcl";
	  allPathsFile << "#include \"Trigger_epilogs/" << triggerPathPair.first<<".fcl\"" << __E__; 
	  //we need to append the line where we instantiate the given TriggerPath
	  //	  allPathsFile << "art.physics." << triggerPathPair.first  << "_trigger  : [ @sequence::Trigger.paths."<< triggerPathPair.first<< " ]\n" << __E__; 
	  // allPathsFile << "art.physics." << triggerPathPair.first  << "_trigger  : [ makeSD, CaloDigiMaker, @sequence::Trigger.paths."<< triggerPathPair.first<< " ]\n" << __E__; 
	  allPathsFile << "art.physics." << triggerPathPair.first  << "_trigger  : [ makeSD, @sequence::Trigger.paths."<< triggerPathPair.first<< " ]" << __E__; 
	  allPathsFile << "art.physics.trigger_paths["<< pathID <<"] : " << triggerPathPair.first  << "_trigger \n"<< __E__; 
	  
	  epilogFclFile.open(epilogName.c_str());

	  //check if the pathID is already usd by another trigger chain
	  for (auto & id : list_of_pathIDs){
	    if (id == pathID){
	      __SS__ << "Attempt to use twice the same PathID : "<< pathID << std::endl;
	      __COUT_ERR__ << ss.str() << std::endl;
	      __SS_THROW__;
	    }
	  }

	  //create the directory that will house all the epilogs of a given triggerPath
	  std::string               singlePathEpilogsDir, singlePathPairFclName;
	  //		singlePathEpilogsDir = "%sTrigger_epilogs/%s", (ARTDAQ_FCL_PATH).c_str(), triggerPathPair.first.c_str());
	  singlePathEpilogsDir = ARTDAQ_FCL_PATH + "Trigger_epilogs/" + triggerPathPair.first;
	  mkdir(singlePathEpilogsDir.c_str(), 0755);
	  __COUT__       << "single path epilogs dir " << singlePathEpilogsDir << __E__; 

	  //set the general prescale factor at the beginning of the path
	  createPrescaleEpilog         (epilogFclFile, singlePathEpilogsDir, triggerPathPair.first, triggerPathPair.second); //singlePath);

	  if (triggerType == "TrackSeed")
	    {
	      //to set up the Tracking filters we need to loop over the children of the corresponding node
	      createTrackingFiltersEpilog  (epilogFclFile, singlePathEpilogsDir, triggerPathPair.first, singlePath);
	    }
	  else if (triggerType == "Helix")
	    {
	      singlePath.getNode("LinkToDigiFilterParameterTable");
	      createHelixFiltersEpilog  (epilogFclFile, singlePathEpilogsDir, triggerPathPair.first, singlePath);
	    }
	  else if (triggerType == "DigiCount")
	    {
	      createDigiCountFiltersEpilog  (epilogFclFile, singlePathEpilogsDir, triggerPathPair.first, singlePath);		    
	    }

	  __COUT__ <<" closing epilogFclFile" <<__E__;
		  
	  epilogFclFile.close();
	  __COUT__ << "epilogFclFile closed... " << __E__;
		
	  ++counter;
	  __COUT__ <<"counter = " << counter <<", map-size = " << children_map_size <<__E__;
	  // if (counter == children_map_size-2) break;
	}//end loop over triggerPathPair
	    
      __COUT__ <<" end of main LOOP" <<__E__;
	   
    }//end loop over topLevelPair

  __COUT__ << "loop completed closed... " << __E__;


  triggerFclFile.close();
  __COUT__ << "triggerFclFile closed... " << __E__;

  //const XDAQContext *contextConfig = configManager->__GET_CONFIG__(XDAQContext);
	
  //	std::vector<const XDAQContext::XDAQContext *> readerContexts =
  //	contextConfig->getBoardReaderContexts();


  //Tree readerConfigNode = contextConfig->getSupervisorConfigNode(configManager,
  //		readerContext->contextUID_, readerContext->applications_[0].applicationUID_);



  //handle fcl file generation, wherever the level of this configuration

  // auto childrenMap = configManager->__SELF_NODE__.getChildren();
  // std::string appUID, buffUID, consumerUID;
  // for (auto &child : childrenMap)
  // {
  // 	const XDAQContext::XDAQContext * thisContext = nullptr;
  // 	for (auto& readerContext : readerContexts) {
  // 		Tree readerConfigNode = contextConfig->getSupervisorConfigNode(configManager,
  // 			readerContext->contextUID_, readerContext->applications_[0].applicationUID_);
  // 		auto dataManagerConfMap = readerConfigNode.getNode("LinkToDataManager").getChildren();
  // 		for (auto dmc : dataManagerConfMap) {
  // 			auto dataBufferConfMap = dmc.second.getNode("LinkToDataBuffer").getChildren();
  // 			for (auto dbc : dataBufferConfMap) {
  // 				auto processorConfUID = dbc.second.getNode("LinkToProcessorUID").getUIDAsString();
  // 				if (processorConfUID == child.second.getValue()) {
  // 					__COUT__ << "Found match for context UID: " << readerContext->contextUID_ << std::endl;
  // 					thisContext = readerContext;
  // 				}
  // 			}
  // 		}
  // 	}

  // 	if (thisContext == nullptr) {
  // 		__COUT_ERR__ << "Could not find matching context for this configuration!" << std::endl;
  // 	}
  // 	else {
  // 		if (child.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
  // 		{
  // 			outputFHICL(configManager, child.second,
  // 				contextConfig->getARTDAQAppRank(thisContext->contextUID_),
  // 				contextConfig->getContextAddress(thisContext->contextUID_),
  // 				contextConfig->getARTDAQDataPort(configManager,thisContext->contextUID_),
  // 				contextConfig);
  // 		}
  // 	}
  // }
}


//----------------------------------------------------------------------------------------------------
// this function creates a string usde to set the module names in a given trigPath
//----------------------------------------------------------------------------------------------------
std::string TopLevelTriggerTable::GetModuleNameFromPath(std::string &TrigPath)
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



//--------------------------------------------------------------------------------------------------------
// this function creates the epilog for the Prescaler module used at the end of each path
//--------------------------------------------------------------------------------------------------------
void   TopLevelTriggerTable::createPrescaleEpilog (std::ofstream& EpilogFclFile, std::string& EpilogsDir,
						   std::string&   TrigPath     , ots::ConfigurationTree  ConfTree)
{
  std::string     singlePathPairFclName;
  std::ofstream   subEpilogFclFile;  
  std::string     varName = "PrescaleFactor";
  __COUT__     << "createPrescaleEpilog starts..." << __E__;

  std::string     filtName = "EventPrescale";

  int             prescaleFactor = ConfTree.getNode(varName).getValue<int>();
  __COUT__  << "node: " << prescaleFactor << __E__;
  __COUTV__(prescaleFactor);
      
  singlePathPairFclName = EpilogsDir+"/"+ TrigPath+ filtName+".fcl";
  EpilogFclFile << "#include \"Trigger_epilogs/" << TrigPath<<"/" << TrigPath << filtName <<".fcl\""<<__E__; 
      
  __COUT__       << "singlePathPairFclName: "<< singlePathPairFclName <<  __E__; 
  std::string     moduleNameHeader = GetModuleNameFromPath(TrigPath);

  subEpilogFclFile.open(singlePathPairFclName);
      
  if (!subEpilogFclFile.is_open())
    {
      __COUT__       << "file: " <<singlePathPairFclName << " not opened" << __E__;
    }
  else
    {
      subEpilogFclFile << "art.physics.filters."<<  moduleNameHeader << filtName << ".nPrescale : " <<  prescaleFactor  << __E__; 
      subEpilogFclFile.close();
    }
  
}

//---------------------------------------------------------------------------------------------------------
//this function creates the epilog with the parameters from a LinkToTimeClusterFilterTable
//---------------------------------------------------------------------------------------------------------
void   TopLevelTriggerTable::createTrackingFiltersEpilog(std::ofstream& EpilogFclFile, std::string& EpilogsDir,
							 std::string&   TrigPath     , ots::ConfigurationTree  ConfTree)
{
  std::string     singlePathPairFclName;
  std::ofstream   subEpilogFclFile;
  
  const int       nFilters(5);
  std::string     varNames [nFilters] = {"LinkToDigiFilterParameterTable",
					 "LinkToTimeClusterFilterParameterTable",
					 "LinkToHelixFilterParameterTable",
					 "LinkToTrackSeedFilterParameterTable",
					 "NOPARAMS"};
  std::string     filtNames[nFilters] = {"SDCountFilter","TCFilter", "HSFilter", "TSFilter","TriggerInfoMerger"};

  __COUT__       << "createTrackingFiltersEpilog starts..." << __E__;

  std::string     moduleNameHeader = GetModuleNameFromPath(TrigPath);

  for (int i=0; i<nFilters; ++i)
    {
      singlePathPairFclName = EpilogsDir + "/" + TrigPath + filtNames[i]+ ".fcl";
      EpilogFclFile << "#include \"Trigger_epilogs/" << TrigPath<<"/" << TrigPath << filtNames[i] << ".fcl\""<<__E__; 
  
      __COUT__       << "singlePathPairFclName: "<< singlePathPairFclName <<  __E__; 

      subEpilogFclFile.open(singlePathPairFclName);

      if (!subEpilogFclFile.is_open())
	{
	  __COUT__       << "file: " <<singlePathPairFclName << " not opened" << __E__;
	}
      else
	{
	  if (i<nFilters-1){
	    ots::ConfigurationTree  timeClusterConf = ConfTree.getNode(varNames[i]);
	    auto   filterConf = timeClusterConf.getChildren();
	    for (auto &params: filterConf) 
	      {
		__COUT__ << filtNames[i] <<" conf: " << params.first  << __E__;
		ots::ConfigurationTree    valName = params.second.getNode("name");
		__COUT__ << filtNames[i] << " param name: "    << valName << __E__;
		ots::ConfigurationTree    valNode = params.second.getNode("value");
		std::string    val =  valNode.getValue<std::string>();
		__COUT__ << filtNames[i] << " param value: "    << val<< __E__;
		subEpilogFclFile << "art.physics.filters."<<  moduleNameHeader <<filtNames[i] <<"."<< valName<<" : " <<  val  << __E__; 
	      }
	  }
	  else {
	    subEpilogFclFile << "art.physics.producers."<<  moduleNameHeader <<filtNames[i] <<" : "<< " { module_type : MergeTriggerInfo }" <<  __E__; 
	  }
	  subEpilogFclFile.close();
	}
    }
}

//---------------------------------------------------------------------------------------------------------
//this function creates the epilog with the parameters from a LinkToTimeClusterFilterTable
//---------------------------------------------------------------------------------------------------------
void   TopLevelTriggerTable::createHelixFiltersEpilog(std::ofstream& EpilogFclFile, std::string& EpilogsDir,
						      std::string&   TrigPath     , ots::ConfigurationTree  ConfTree)
{
  std::string          singlePathPairFclName;
  std::ofstream   subEpilogFclFile;
  
  const int       nFilters(4);
  std::string     varNames [nFilters] = {"LinkToDigiFilterParameterTable",
					 "LinkToTimeClusterFilterParameterTable",
					 "LinkToHelixFilterParameterTable","NOPARAMS"};
  std::string     filtNames[nFilters] = {"SDCountFilter", "TCFilter", "HSFilter","TriggerInfoMerger"};

  __COUT__       << "createHelixFiltersEpilog starts..." << __E__;
  std::string     moduleNameHeader = GetModuleNameFromPath(TrigPath);

  for (int i=0; i<nFilters; ++i)
    {
      __COUT__ << "varName["<<i << "] = " << varNames[i] << __E__;

      singlePathPairFclName = EpilogsDir + "/" + TrigPath.c_str() + filtNames[i] + ".fcl";
      EpilogFclFile << "#include \"Trigger_epilogs/" << TrigPath<<"/" << TrigPath << filtNames[i] << ".fcl\""<<__E__; 
  
      __COUT__       << "singlePathPairFclName: "<< singlePathPairFclName <<  __E__;

      subEpilogFclFile.open(singlePathPairFclName);

      if (!subEpilogFclFile.is_open())
	{
	  __COUT__       << "file: " <<singlePathPairFclName << " not opened" << __E__;
	}
      else
	{
	  if (i<nFilters-1){//filter config
	    ots::ConfigurationTree  timeClusterConf = ConfTree.getNode(varNames[i]);
	    auto   filterConf = timeClusterConf.getChildren();
	    for (auto &params: filterConf) 
	      {
		__COUT__ << filtNames[i] <<" conf: " << params.first  << __E__;
		ots::ConfigurationTree    valName = params.second.getNode("name");
		__COUT__ << filtNames[i] << " param name: "    << valName << __E__;
		ots::ConfigurationTree    valNode = params.second.getNode("value");
		std::string    val =  valNode.getValue<std::string>();
		__COUT__ << filtNames[i] << " param value: "    << val<< __E__;
		subEpilogFclFile << "art.physics.filters."<<  moduleNameHeader <<filtNames[i] <<"."<< valName<<" : " <<  val  << __E__; 
	      }
	  }else 
	    {
	      subEpilogFclFile << "art.physics.producers."<<  moduleNameHeader <<filtNames[i] <<" : "<< " { module_type : MergeTriggerInfo }" <<  __E__; 
	    }
	  subEpilogFclFile.close();
	}
    }
}
//---------------------------------------------------------------------------------------------------------
//this function creates the epilog with the parameters from a LinkToTimeClusterFilterTable
//---------------------------------------------------------------------------------------------------------
void   TopLevelTriggerTable::createDigiCountFiltersEpilog(std::ofstream& EpilogFclFile, std::string& EpilogsDir,
							  std::string&   TrigPath     , ots::ConfigurationTree  ConfTree)
{
  std::string            singlePathPairFclName;
  std::ofstream   subEpilogFclFile;
  
  const int       nFilters(1);
  std::string     varNames [nFilters] = {"LinkToDigiFilterParameterTable"};
  std::string     filtNames[nFilters] = {"SDCountFilter"};

  __COUT__       << "createDigiCountiltersEpilog starts..." << __E__;
  std::string     moduleNameHeader = GetModuleNameFromPath(TrigPath);

  for (int i=0; i<nFilters; ++i)
    {
      ots::ConfigurationTree  timeClusterConf = ConfTree.getNode(varNames[i]);
      
      singlePathPairFclName = EpilogsDir + "/" + TrigPath + filtNames[i]+ ".fcl";
      EpilogFclFile << "#include \"Trigger_epilogs/" << TrigPath<<"/" << TrigPath << filtNames[i] << ".fcl\""<<__E__; 
  
      __COUT__       << "singlePathPairFclName: "<< singlePathPairFclName <<  __E__; 

      subEpilogFclFile.open(singlePathPairFclName);

      __COUT__    << "singlePathPairFclName opened" << __E__;

      if (!subEpilogFclFile.is_open())
	{
	  __COUT__       << "file: " <<singlePathPairFclName << " not opened" << __E__;
	}
      else
	{
	  auto   filterConf = timeClusterConf.getChildren();
	  __COUT__    << "took the children from TimeClusterConf" <<__E__;
	  
	  for (auto &params: filterConf) 
	    {
	      __COUT__ << filtNames[i] <<" conf: " << params.first  << __E__;
	      ots::ConfigurationTree    valName = params.second.getNode("name");
	      __COUT__ << filtNames[i] << " param name: "    << valName << __E__;
	      ots::ConfigurationTree    valNode = params.second.getNode("value");
	      std::string    val =  valNode.getValue<std::string>();
	      __COUT__ << filtNames[i] << " param value: "    << val<< __E__;
	      subEpilogFclFile << "art.physics.filters."<<  moduleNameHeader <<filtNames[i] <<"."<< valName<<" : " <<  val  << __E__; 
	    }
	  subEpilogFclFile.close();
	}

      __COUT__    << "singlePathPairFclName closed..." << __E__;


    }
}

// ////========================================================================================================================
// //void TopLevelTriggerConfiguration::getBoardReaderParents(const ConfigurationTree &readerNode,
// //		const ConfigurationTree &contextNode, std::string &applicationUID,
// //		std::string &bufferUID, std::string &consumerUID)
// //{
// //	//search through all board reader contexts
// //	contextNode.getNode()
// //}

// //========================================================================================================================
// std::string TopLevelTriggerConfiguration::getFHICLFilename(const ConfigurationTree &boardReaderNode)
// {
// 	__COUT__ << "ARTDAQ BoardReader UID: " << boardReaderNode.getValue() << std::endl;
// 	std::string filename = ARTDAQ_FCL_PATH + ARTDAQ_FILE_PREAMBLE + "-";
// 	std::string uid = boardReaderNode.getValue();
// 	for (unsigned int i = 0; i < uid.size(); ++i)
// 		if ((uid[i] >= 'a' && uid[i] <= 'z') ||
// 			(uid[i] >= 'A' && uid[i] <= 'Z') ||
// 			(uid[i] >= '0' && uid[i] <= '9')) //only allow alpha numeric in file name
// 			filename += uid[i];

// 	filename += ".fcl";

// 	__COUT__ << "fcl: " << filename << std::endl;

// 	return filename;
// }

// //========================================================================================================================
// void TopLevelTriggerConfiguration::outputFHICL(ConfigurationManager* configManager,
// 		const ConfigurationTree &boardReaderNode, unsigned int selfRank, std::string selfHost, unsigned int selfPort,
// 	const XDAQContextConfiguration *contextConfig)
// {
// 	/*
// 		the file will look something like this:

// 		  daq: {
// 			  fragment_receiver: {
// 				mpi_sync_interval: 50

// 				# CommandableFragmentGenerator Configuration:
// 			fragment_ids: []
// 			fragment_id: -99 # Please define only one of these

// 			sleep_on_stop_us: 0

// 			requests_enabled: false # Whether to set up the socket for listening for trigger messages
// 			request_mode: "Ignored" # Possible values are: Ignored, Single, Buffer, Window

// 			data_buffer_depth_fragments: 1000
// 			data_buffer_depth_mb: 1000

// 			request_port: 3001
// 			request_address: "227.128.12.26" # Multicast request address

// 			request_window_offset: 0 # Request message contains tzero. Window will be from tzero - offset to tzero + width
// 			request_window_width: 0
// 			stale_request_timeout: "0xFFFFFFFF" # How long to wait before discarding request messages that are outside the available datae
// 			request_windows_are_unique: true # If request windows are unique, avoids a copy operation, but the same data point cannot be used for two requests. If this is not anticipated, leave set to "true"

// 			separate_data_thread: false # MUST be true for triggers to be applied! If triggering is not desired, but a separate readout thread is, set this to true, triggers_enabled to false and trigger_mode to ignored.
// 			separate_monitoring_thread: false # Whether a thread should be started which periodically calls checkHWStatus_, a user-defined function which should be used to check hardware status registers and report to MetricMan.
// 			poll_hardware_status: false # Whether checkHWStatus_ will be called, either through the thread or at the start of getNext
// 			hardware_poll_interval_us: 1000000 # If hardware monitoring thread is enabled, how often should it call checkHWStatus_


// 				# Generated Parameters:
// 				generator: ToySimulator
// 				fragment_type: TOY1
// 				fragment_id: 0
// 				board_id: 0
// 				starting_fragment_id: 0
// 				random_seed: 5780
// 				sleep_on_stop_us: 500000

// 				# Generator-Specific Configuration:

// 			nADCcounts: 40

// 			throttle_usecs: 100000

// 			distribution_type: 1

// 			timestamp_scale_factor: 1


// 				destinations: {
// 				  d2: { transferPluginType: MPI destination_rank: 2 max_fragment_size_words: 2097152}
// 			d3: { transferPluginType: MPI destination_rank: 3 max_fragment_size_words: 2097152}

// 				}
// 			  }

// 			  metrics: {
// 				brFile: {
// 				  metricPluginType: "file"
// 				  level: 3
// 				  fileName: "/tmp/boardreader/br_%UID%_metrics.log"
// 				  uniquify: true
// 				}
// 				# ganglia: {
// 				#   metricPluginType: "ganglia"
// 				#   level: %{ganglia_level}
// 				#   reporting_interval: 15.0
// 				#
// 				#   configFile: "/etc/ganglia/gmond.conf"
// 				#   group: "ARTDAQ"
// 				# }
// 				# msgfac: {
// 				#    level: %{mf_level}
// 				#    metricPluginType: "msgFacility"
// 				#    output_message_application_name: "ARTDAQ Metric"
// 				#    output_message_severity: 0
// 				# }
// 				# graphite: {
// 				#   level: %{graphite_level}
// 				#   metricPluginType: "graphite"
// 				#   host: "localhost"
// 				#   port: 20030
// 				#   namespace: "artdaq."
// 				# }
// 			  }
// 			}

// 	 */

// 	std::string filename = getFHICLFilename(boardReaderNode);

// 	/////////////////////////
// 	//generate xdaq run parameter file
// 	std::fstream out;

// 	std::string tabStr = "";
// 	std::string commentStr = "";

// 	out.open(filename, std::fstream::out | std::fstream::trunc);
// 	if (out.fail())
// 	{
// 		__SS__ << "Failed to open ARTDAQ Builder fcl file: " << filename << std::endl;
// 		__SS_THROW__;
// 	}

// 	//no primary link to configuration tree for reader node!
// 	try
// 	{
// 		if (boardReaderNode.isDisconnected())
// 		{
// 			//create empty fcl
// 			OUT << "{}\n\n";
// 			out.close();
// 			return;
// 		}
// 	}
// 	catch (const std::runtime_error)
// 	{
// 		__COUT__ << "Ignoring error, assume this a valid UID node." << std::endl;
// 		//error is expected here for UIDs.. so just ignore
// 		// this check is valuable if source node is a unique-Link node, rather than UID
// 	}

// 	//--------------------------------------
// 	//handle daq
// 	OUT << "daq: {\n";

// 	//fragment_receiver
// 	PUSHTAB;
// 	OUT << "fragment_receiver: {\n";

// 	PUSHTAB;
// 	{
// 		//plugin type and fragment data-type
// 		OUT << "generator" <<
// 			": " <<
// 			boardReaderNode.getNode("daqGeneratorPluginType").getValue() <<
// 			("\t #daq generator plug-in type") <<
// 			"\n";
// 		OUT << "fragment_type" <<
// 			": " <<
// 			boardReaderNode.getNode("daqGeneratorFragmentType").getValue() <<
// 			("\t #generator data fragment type") <<
// 			"\n\n";

// 		//shared and unique parameters
// 		auto parametersLink = boardReaderNode.getNode("daqParametersLink");
// 		if (!parametersLink.isDisconnected())
// 		{

// 			auto parameters = parametersLink.getChildren();
// 			for (auto &parameter : parameters)
// 			{
// 				if (!parameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
// 					PUSHCOMMENT;

// 				//				__COUT__ << parameter.second.getNode("daqParameterKey").getValue() <<
// 				//						": " <<
// 				//						parameter.second.getNode("daqParameterValue").getValue()
// 				//						<<
// 				//						"\n";

// 				auto comment = parameter.second.getNode("CommentDescription");
// 				OUT << parameter.second.getNode("daqParameterKey").getValue() <<
// 					": " <<
// 					parameter.second.getNode("daqParameterValue").getValue()
// 					<<
// 					(comment.isDefaultValue() ? "" : ("\t # " + comment.getValue())) <<
// 					"\n";

// 				if (!parameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
// 					POPCOMMENT;
// 			}
// 		}
// 		OUT << "\n";	//end daq board reader parameters
// 	}
// 	//	{	//unique parameters
// 	//		auto parametersLink = boardReaderNode.getNode("daqUniqueParametersLink");
// 	//		if(!parametersLink.isDisconnected())
// 	//		{
// 	//
// 	//			auto parameters = parametersLink.getChildren();
// 	//			for(auto &parameter:parameters)
// 	//			{
// 	//				if(!parameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
// 	//					PUSHCOMMENT;
// 	//
// 	//				auto comment = parameter.second.getNode("CommentDescription");
// 	//				OUT << parameter.second.getNode("daqParameterKey").getValue() <<
// 	//						": " <<
// 	//						parameter.second.getNode("daqParameterValue").getValue()
// 	//						<<
// 	//						(comment.isDefaultValue()?"":("\t # " + comment.getValue())) <<
// 	//						"\n";
// 	//
// 	//				if(!parameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
// 	//					POPCOMMENT;
// 	//			}
// 	//		}
// 	//		OUT << "\n";	//end shared daq board reader parameters
// 	//	}

// 	OUT << "destinations: {\n";

// 	PUSHTAB;
// 	auto destinationsGroup = boardReaderNode.getNode("daqDestinationsLink");
// 	if (!destinationsGroup.isDisconnected())
// 	{
// 		try
// 		{
// 			auto destinations = destinationsGroup.getChildren();
// 			for (auto &destination : destinations)
// 			{
// 				auto destinationContextUID = destination.second.getNode("destinationARTDAQContextLink").getValueAsString();

// 				unsigned int destinationRank = contextConfig->getARTDAQAppRank(destinationContextUID);
// 				std::string host = contextConfig->getContextAddress(destinationContextUID);
// 				unsigned int port = contextConfig->getARTDAQDataPort(configManager,destinationContextUID);
				
// 				OUT << destination.second.getNode("destinationKey").getValue() <<
// 					": {" <<
// 					" transferPluginType: " <<
// 					destination.second.getNode("transferPluginType").getValue() <<
// 					" destination_rank: " <<
// 					destinationRank <<
// 					" max_fragment_size_words: " <<
// 					destination.second.getNode("ARTDAQGlobalConfigurationLink/maxFragmentSizeWords").getValue<unsigned int>() <<
// 					" host_map: [{rank: " << destinationRank << " host: \"" << host << "\" portOffset: " << std::to_string(port) << "}, " <<
// 					"{rank: " << selfRank << " host: \"" << selfHost << "\" portOffset: " << std::to_string(selfPort) << "}]" <<
// 					"}\n";
// 			}
// 		}
// 		catch (const std::runtime_error& e)
// 		{
// 			__SS__ << "Are the DAQ destinations valid? Error occurred looking for Board Reader DAQ sources for UID '" <<
// 				boardReaderNode.getValue() << "': " << e.what() << std::endl;
// 			__COUT_ERR__ << ss.str() << std::endl;
// 			__SS_THROW__;
// 		}
// 	}
// 	POPTAB;
// 	OUT << "}\n\n";	//end destinations

// 	POPTAB;
// 	OUT << "}\n\n";	//end fragment_receiver


// 	OUT << "metrics: {\n";

// 	PUSHTAB;
// 	auto metricsGroup = boardReaderNode.getNode("daqMetricsLink");
// 	if (!metricsGroup.isDisconnected())
// 	{
// 		auto metrics = metricsGroup.getChildren();

// 		for (auto &metric : metrics)
// 		{
// 			if (!metric.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
// 				PUSHCOMMENT;

// 			OUT << metric.second.getNode("metricKey").getValue() <<
// 				": {\n";
// 			PUSHTAB;

// 			OUT << "metricPluginType: " <<
// 				metric.second.getNode("metricPluginType").getValue()
// 				<< "\n";
// 			OUT << "level: " <<
// 				metric.second.getNode("metricLevel").getValue()
// 				<< "\n";

// 			auto metricParametersGroup = metric.second.getNode("metricParametersLink");
// 			if (!metricParametersGroup.isDisconnected())
// 			{
// 				auto metricParameters = metricParametersGroup.getChildren();
// 				for (auto &metricParameter : metricParameters)
// 				{
// 					if (!metricParameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
// 						PUSHCOMMENT;

// 					OUT << metricParameter.second.getNode("metricParameterKey").getValue() <<
// 						": " <<
// 						metricParameter.second.getNode("metricParameterValue").getValue()
// 						<< "\n";

// 					if (!metricParameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
// 						POPCOMMENT;

// 				}
// 			}
// 			POPTAB;
// 			OUT << "}\n\n";	//end metric

// 			if (!metric.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
// 				POPCOMMENT;
// 		}
// 	}
// 	POPTAB;
// 	OUT << "}\n\n";	//end metrics

// 	POPTAB;
// 	OUT << "}\n\n";	//end daq


// 	out.close();
// }

DEFINE_OTS_TABLE(TopLevelTriggerTable)
