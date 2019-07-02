#include "otsdaq-components/DAQHardware/FSSRFirmwareBase.h"
#include "otsdaq-components/DAQHardware/FSSROtsFirmware.h"
#include "otsdaq-components/DAQHardware/FSSRPurdueFirmware.h"
#include "otsdaq-components/DAQHardware/OtsUDPFirmwareCore.h"
#include "otsdaq-components/DAQHardware/OtsUDPHardware.h"
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/Macros/InterfacePluginMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-trigger/FEInterfaces/FEPurdueNetworkMasterInterface.h"

#include <iostream>
#include <set>
using namespace ots;

FEPurdueNetworkMasterInterface::FEPurdueNetworkMasterInterface(
    const std::string&       interfaceUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       interfaceConfigurationPath)
    : FEVInterface(interfaceUID, theXDAQContextConfigTree, interfaceConfigurationPath)
{
	std::string firmwareType = FSSRFirmwareBase::
	    PURDUE_FIRMWARE_NAME;  // theXDAQContextConfigTree.getNode(interfaceConfigurationPath).getNode("FirmwareType").getValue<std::string>();
	unsigned int firmwareVersion =
	    theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
	        .getNode("FirmwareVersion")
	        .getValue<unsigned int>();
	FSSRFirmware_ = new FSSRPurdueFirmware(firmwareType, firmwareVersion);
	FSSRHardware_ =
	    new OtsUDPHardware(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
	                           .getNode("HostIPAddress")
	                           .getValue<std::string>(),
	                       theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
	                           .getNode("HostPort")
	                           .getValue<unsigned int>(),
	                       theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
	                           .getNode("InterfaceIPAddress")
	                           .getValue<std::string>(),
	                       theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
	                           .getNode("InterfacePort")
	                           .getValue<unsigned int>());
}
//========================================================================================================================
FEPurdueNetworkMasterInterface::~FEPurdueNetworkMasterInterface(void)
{
	delete FSSRFirmware_;
	FSSRFirmware_ = 0;
	delete FSSRHardware_;
	FSSRHardware_ = 0;
}

//========================================================================================================================
void FEPurdueNetworkMasterInterface::configure(void)
{
	std::cout << __COUT_HDR_FL__ << "\tConfigure" << std::endl;
}

//========================================================================================================================
// void FEPurdueNetworkMasterInterface::configureDetector(const ots::DACStream&)
//{
// Network master doesn't configure the detector
//}

//========================================================================================================================
void FEPurdueNetworkMasterInterface::halt(void)
{
	std::cout << __COUT_HDR_FL__ << "\tHalt" << std::endl;
}

//========================================================================================================================
void FEPurdueNetworkMasterInterface::pause(void)
{
	std::cout << __COUT_HDR_FL__ << "\tPause" << std::endl;
}

//========================================================================================================================
void FEPurdueNetworkMasterInterface::resume(void) {}

//========================================================================================================================
void FEPurdueNetworkMasterInterface::start(std::string)  // runNumber)
{
	__CFG_COUT__ << "START NETWORK!" << std::endl;
	// FSSRHardware_->write(FSSRFirmware_->resetSlaveBCO());
}
//========================================================================================================================
void FEPurdueNetworkMasterInterface::stop(void)
{
	std::cout << __COUT_HDR_FL__ << "\tStop" << std::endl;
}

//========================================================================================================================
bool FEPurdueNetworkMasterInterface::running(void)
{
	__CFG_COUT__ << "RESETTING BCO!" << std::endl;

	FSSRHardware_->write(FSSRFirmware_->resetSlaveBCO());

	__CFG_COUT__ << "DONE RESET BCO!" << std::endl;
	return false;
}

DEFINE_OTS_INTERFACE(FEPurdueNetworkMasterInterface)
