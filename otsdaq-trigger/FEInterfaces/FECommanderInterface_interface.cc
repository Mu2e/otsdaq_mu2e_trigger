#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/Macros/InterfacePluginMacros.h"
#include "otsdaq-trigger/FEInterfaces/FECommanderInterface.h"

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
ots::FECommanderInterface::FECommanderInterface(
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
ots::FECommanderInterface::~FECommanderInterface(void) {}

//========================================================================================================================
void ots::FECommanderInterface::send(std::string buffer) throw(std::runtime_error) try
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
void ots::FECommanderInterface::halt(void)
{
	// MESSAGE = "PhysicsRuns0,Halt"
	send("HALT");
}

//========================================================================================================================
void ots::FECommanderInterface::pause(void)
{
	// send("PAUSE");
}

//========================================================================================================================
void ots::FECommanderInterface::resume(void)
{
	// send("RESUME");
}
//========================================================================================================================
void ots::FECommanderInterface::start(std::string runNumber)
{
	// MESSAGE = "PhysicsRuns0,Start, %i" % (int(run)) #"PhysicsRuns0,Start"
	send(std::string("R ") + runNumber + '\n');
	// data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
	// print "received message:", data
}

//========================================================================================================================
void ots::FECommanderInterface::stop(void)
{
	// MESSAGE = "PhysicsRuns0,Stop"
	send("H");
}

//========================================================================================================================
void ots::FECommanderInterface::configure(void)
{
	std::cout << __COUT_HDR__ << "\tConfigure" << std::endl;
	// MESSAGE = "PhysicsRuns0,Configure,FQNETConfig"
	// sendToVME("CONFIGURE");
}

//========================================================================================================================
// bool ots::FECommanderInterface::running(void)
//{
//	return WorkLoop::continueWorkLoop_;//otherwise it stops!!!!!
//}

DEFINE_OTS_INTERFACE(ots::FECommanderInterface)
