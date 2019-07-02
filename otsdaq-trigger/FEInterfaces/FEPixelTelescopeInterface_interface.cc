#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/Macros/InterfacePluginMacros.h"
#include "otsdaq-trigger/FEInterfaces/FEPixelTelescopeInterface.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>  //for memcpy
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

using namespace ots;

//========================================================================================================================
ots::FEPixelTelescopeInterface::FEPixelTelescopeInterface(
    const std::string&       interfaceUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       interfaceConfigurationPath)
    : Socket(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                 .getNode("HostIPAddress")
                 .getValue<std::string>(),
             theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                 .getNode("HostPort")
                 .getValue<unsigned int>())
    , FEVInterface(interfaceUID, theXDAQContextConfigTree, interfaceConfigurationPath)
    , interfaceSocket_(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                           .getNode("InterfaceIPAddress")
                           .getValue<std::string>(),
                       theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                           .getNode("InterfacePort")
                           .getValue<unsigned int>())
{
	Socket::initialize();
	universalAddressSize_ = 8;
	universalDataSize_    = 8;
}

//========================================================================================================================
ots::FEPixelTelescopeInterface::~FEPixelTelescopeInterface(void) {}

//========================================================================================================================
void ots::FEPixelTelescopeInterface::send(std::string buffer) throw(
    std::runtime_error) try
{
	bool verbose = false;
	if(TransceiverSocket::send(interfaceSocket_, buffer, verbose) < 0)
	{
		__SS__ << "Write failed to IP:Port " << interfaceSocket_.getIPAddress() << ":"
		       << interfaceSocket_.getPort() << __E__;
		__SS_THROW__;
	}
}
catch(std::runtime_error& e)
{
	throw;
}
catch(...)
{
	__SS__ << "Unrecognized exception caught!" << std::endl;
	__SS_THROW__;
}

//========================================================================================================================
void ots::FEPixelTelescopeInterface::halt(void) { send("H"); }

//========================================================================================================================
void ots::FEPixelTelescopeInterface::pause(void) { send("P"); }

//========================================================================================================================
void ots::FEPixelTelescopeInterface::resume(void) { send("R"); }
//========================================================================================================================
void ots::FEPixelTelescopeInterface::start(std::string runNumber)
{
	//__COUT__ << "Starting Pixel Telescope" << std::endl;
	send(std::string("R ") + runNumber + '\n');
}

//========================================================================================================================
void ots::FEPixelTelescopeInterface::stop(void)
{
	//__COUT__ << "Stopping Pixel Telescope" << std::endl;
	send("H");
}

//========================================================================================================================
void ots::FEPixelTelescopeInterface::configure(void) { send("H"); }

//========================================================================================================================
// bool ots::FEPixelTelescopeInterface::running(void)
//{
//	return WorkLoop::continueWorkLoop_;//otherwise it stops!!!!!
//}

DEFINE_OTS_INTERFACE(ots::FEPixelTelescopeInterface)
