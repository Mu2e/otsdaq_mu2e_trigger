#include "otsdaq-trigger/FEInterfaces/FSSRInterface.h"

#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/Macros/InterfacePluginMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"

#include "otsdaq-components/DAQHardware/FrontEndHardwareBase.h"
#include "otsdaq-components/DAQHardware/OtsUDPHardware.h"
// Where is the purdue hardware? both UDP so can use same hardware class

#include "otsdaq-components/DAQHardware/OtsUDPFirmwareCore.h"

#include "otsdaq-components/DAQHardware/FSSRFirmwareBase.h"
#include "otsdaq-components/DAQHardware/FSSROtsFirmware.h"
#include "otsdaq-components/DAQHardware/FSSRPurdueFirmware.h"

#include "otsdaq-components/DetectorConfiguration/DACStream.h"

#include <unistd.h>
#include <iostream>
#include <set>

using namespace ots;

//========================================================================================================================
FSSRInterface::FSSRInterface(const std::string&       interfaceUID,
                             const ConfigurationTree& theXDAQContextConfigTree,
                             const std::string&       interfaceConfigurationPath)
    : FEVInterface(interfaceUID, theXDAQContextConfigTree, interfaceConfigurationPath)
    , FSSRFirmware_(nullptr)
    , FSSRHardware_(nullptr)
    , hardwareType_(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                        .getNode("HardwareType")
                        .getValue<std::string>())
    , firmwareType_(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                        .getNode("FirmwareType")
                        .getValue<std::string>())
    , firmwareVersion_(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                           .getNode("FirmwareVersion")
                           .getValue<unsigned int>())
{
	__CFG_COUT__ << "Constructing FSSRInterface. "
	                "=================================================================="
	             << std::endl;

	universalAddressSize_ = 8;
	universalDataSize_    = 8;

	// Setup firmware instance
	__CFG_COUT__ << "FirmwareType: " << firmwareType_ << std::endl;
	__CFG_COUT__ << "FirmwareVersion: " << firmwareVersion_ << std::endl;

	// choose: PurdueFSSRFirmware or OtsUDPFSSRFirmware (Right now we are running with the
	// OtsUDPFSSRFirmware)
	if(firmwareType_ == FSSRFirmwareBase::PURDUE_FIRMWARE_NAME)
		FSSRFirmware_ = new FSSRPurdueFirmware(firmwareType_, firmwareVersion_);
	else if(firmwareType_ == FSSRFirmwareBase::OTS_FIRMWARE_NAME)
		FSSRFirmware_ = new FSSROtsFirmware(firmwareType_, firmwareVersion_);
	else
	{
		__SS__ << "Unknown applicationFirmwareType type choice: " << firmwareType_
		       << std::endl;
		__CFG_COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}

	// Setup hardware instance
	__CFG_COUT__ << "HardwareType: " << hardwareType_ << std::endl;

	// choose: OtsUDPHardware or OtsUDPHardware ?? //FIXME? ? why does purdue use same
	// hardware? (this is how interface was ... why?)
	if(hardwareType_ == FrontEndHardwareBase::PURDUE_HARDWARE_NAME)
		FSSRHardware_ = new OtsUDPHardware(
		    theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
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
	else if(hardwareType_ == FrontEndHardwareBase::OTS_HARDWARE_NAME)
		FSSRHardware_ = new OtsUDPHardware(
		    theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
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
	else
	{
		__SS__ << "Unknown hardware type choice: " << hardwareType_ << std::endl;
		__CFG_COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}

	__CFG_COUT__ << "Constructor complete. "
	                "=================================================================="
	             << std::endl;
}

//========================================================================================================================
FSSRInterface::~FSSRInterface(void)
{
	delete FSSRFirmware_;
	FSSRFirmware_ = nullptr;
	delete FSSRHardware_;
	FSSRHardware_ = nullptr;
}

//========================================================================================================================
void FSSRInterface::configure(void)
{
	__CFG_COUT__
	    << "============================================================================"
	    << std::endl;
	__CFG_COUT__
	    << "Configure =================================================================="
	    << std::endl;
	std::string writeBuffer;
	std::string readBuffer;
	uint64_t    tmp;

	if(firmwareType_ == FSSRFirmwareBase::OTS_FIRMWARE_NAME)
	{
		__CFG_COUT__ << "Resetting Ethernet!" << std::endl;
		((OtsUDPFirmwareCore*)(FSSRFirmware_->communicationFirmwareInstance_))
		    ->softEthernetReset(writeBuffer);
		FSSRHardware_->write(writeBuffer);
		((OtsUDPFirmwareCore*)(FSSRFirmware_->communicationFirmwareInstance_))
		    ->clearEthernetReset(writeBuffer);
		FSSRHardware_->write(writeBuffer);
		//__CFG_COUT__  << "Sleeping 1 second..." << std::endl;
		// sleep(1); //seconds
	}

	__CFG_COUT__ << "Clearing receive socket buffer: " << FSSRHardware_->flushRead()
	             << " packets cleared." << std::endl;

	std::string streamToIP = theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                             .getNode("StreamToIPAddress")
	                             .getValue<std::string>();
	unsigned int streamToPort = theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                                .getNode("StreamToPort")
	                                .getValue<unsigned int>();

	__CFG_COUT__ << "Setting destination IP:   " << streamToIP << std::endl;
	__CFG_COUT__ << "Setting destination port: " << streamToPort << std::endl;

	FSSRFirmware_->communicationFirmwareInstance_->setDataDestination(
	    writeBuffer, streamToIP, streamToPort);
	FSSRHardware_->write(writeBuffer);
	//__CFG_COUT__ << "Data destination set!" << std::endl;

	/*
	try
	{
	    if (firmwareType_ == FSSRFirmwareBase::OTS_FIRMWARE_NAME)
	    {

	        __CFG_COUT__ << "Reading back burst dest MAC/IP/Port: "  << std::endl;

	        ((OtsUDPFirmwareCore*)(FSSRFirmware_->communicationFirmwareInstance_))->readDataDestinationMAC(writeBuffer);
	        FSSRHardware_->read(writeBuffer, readBuffer);
	        __CFG_COUT__ << "Destination MAC Address: ";
	        for (uint32_t i = 0; i < readBuffer.size(); i++)
	            printf("%2.2X-", (((int16_t)readBuffer[i]) & 0xFF));
	        std::cout << std::endl;

	        ((OtsUDPFirmwareCore*)(FSSRFirmware_->communicationFirmwareInstance_))->readDataDestinationIP(writeBuffer);
	        FSSRHardware_->read(writeBuffer, readBuffer);
	        __CFG_COUT__ << "Destination IP: ";
	        for (uint32_t i = 0; i < readBuffer.size(); i++)
	            printf("%2.2X-", (((int16_t)readBuffer[i]) & 0xFF));
	        std::cout << std::endl;

	        ((OtsUDPFirmwareCore*)(FSSRFirmware_->communicationFirmwareInstance_))->readDataDestinationPort(writeBuffer);
	        FSSRHardware_->read(writeBuffer, readBuffer);
	        __CFG_COUT__ << "Destination Port: ";
	        for (uint32_t i = 0; i < readBuffer.size(); i++)
	            printf("%2.2X-", (((int16_t)readBuffer[i]) & 0xFF));
	        std::cout << std::endl;
	    }
	}
	catch (...)
	{
	    __CFG_COUT__ << "Error reading while configuring." << std::endl;
	    throw;
	}
*/
	__CFG_COUT__ << "Done configuring Ethernet block." << std::endl;

	std::string value;
	std::string CSRRegister = FSSRFirmware_->readCSRRegister();

	FSSRHardware_->read(CSRRegister, value);
	uint32_t registerValue = FSSRFirmware_->createRegisterFromValue(CSRRegister, value);
	__CFG_COUT__ << theXDAQContextConfigTree_.getBackNode(theConfigurationPath_)
	                    .getNode("LinkToFEToDetectorTable")
	             << " -> Initial STRIP CSR Register value: 0x" << std::hex
	             << registerValue << std::dec << std::endl;

	FSSRFirmware_->setCSRRegister(0);  // registerValue);//WHY 0?????????

	//	return;

	/////////////////////////////////////////////////
	__CFG_COUT__ << "Configuring clocks..." << std::endl;
	__CFG_COUT__ << "Clock source:    "
	             << theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                    .getNode("ClockSelect")
	                    .getValue<std::string>()
	             << std::endl;
	__CFG_COUT__ << "Clock frequency: "
	             << theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                    .getNode("ClockSpeedMHz")
	                    .getValue<float>()
	             << " MHz" << std::endl;
	FSSRHardware_->write(FSSRFirmware_->configureClocks(
	    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	        .getNode("ClockSelect")
	        .getValue<std::string>(),
	    //"Internal",
	    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	        .getNode("ClockSpeedMHz")
	        .getValue<float>()));

	// FSSRHardware_->writeAndAcknowledge(FSSRFirmware_->resetDetector(),10);

	//
	//	__CFG_COUT__ << std::endl;
	//
	usleep(200000);
	// LORE 2018/10/22 Commented out because it is done inside the configure clocks twice!
	// writeBuffer.resize(0);
	// FSSRFirmware_->resetDCM(writeBuffer);
	// FSSRHardware_->write(writeBuffer);
	// usleep(100000);

	writeBuffer.resize(0);
	//////////////////////////////////////////////////////////////////
	// ORIGINAL TILL 2018/05/30 -> the explanation on how to align the readout is inside
	// the method  FSSRFirmware_->alignReadOut(writeBuffer, 0x1e);//SEEMED TO BE USELESS
	// for(unsigned int i=0; i<numberOfTicks; i++)
	//{
	//  FSSRFirmware_->alignReadOut(writeBuffer, sensor, chip);
	//  FSSRHardware_->write(writeBuffer);
	//}
	//////////////////////////////////////////////////////////////////
	FSSRHardware_->write(FSSRFirmware_->resetDetector());
	usleep(50000);  // Need sometime to clear

	// FSSRHardware_->read(CSRRegister, value);
	// registerValue = FSSRFirmware_->createRegisterFromValue(CSRRegister, value);
	//__CFG_COUT__ << "STRIP CSR Register value: 0x" << std::hex << registerValue <<
	// std::dec << std::endl;

	configureDetector();

	FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), value);
	CSRRegister   = FSSRFirmware_->readCSRRegister();
	registerValue = FSSRFirmware_->createRegisterFromValue(CSRRegister, value);
	__CFG_COUT__ << theXDAQContextConfigTree_.getBackNode(theConfigurationPath_)
	                    .getNode("LinkToFEToDetectorTable")
	             << " -> STRIP CSR Register value: 0x" << std::hex << registerValue
	             << std::dec << std::endl;
	__CFG_COUT__ << theXDAQContextConfigTree_.getBackNode(theConfigurationPath_)
	                    .getNode("LinkToFEToDetectorTable")
	             << " -> STRIP CSR Register value: 0x" << std::hex << registerValue
	             << std::dec << std::endl;
	__CFG_COUT__ << theXDAQContextConfigTree_.getBackNode(theConfigurationPath_)
	                    .getNode("LinkToFEToDetectorTable")
	             << " -> STRIP CSR Register value: 0x" << std::hex << registerValue
	             << std::dec << std::endl;
	__CFG_COUT__ << theXDAQContextConfigTree_.getBackNode(theConfigurationPath_)
	                    .getNode("LinkToFEToDetectorTable")
	             << " -> STRIP CSR Register value: 0x" << std::hex << registerValue
	             << std::dec << std::endl;
	__CFG_COUT__ << "Configure done "
	                "================================================================"
	             << std::endl;
}

//========================================================================================================================
void FSSRInterface::configureDetector(void)
{
	__CFG_COUT__ << "Configuring Detector: "
	             << theXDAQContextConfigTree_.getBackNode(theConfigurationPath_)
	                    .getNode("LinkToFEToDetectorTable")
	             << "  =========================================" << std::endl;
	//__CFG_COUT__ <<
	//	for(const auto& interface:
	// theXDAQContextConfigTree_.getBackNode(theConfigurationPath_).getNode("LinkToFEToDetectorTable").getChildren())
	//	{
	//		__CFG_COUT__ << interface.first << std::endl;
	//		__CFG_COUT__ <<
	// interface.second.getNode("FEWriterDetectorAddress").getValue<unsigned int>() <<
	// std::endl;
	//	}

	//__CFG_COUT__ << std::endl;

	DACStream theDACStream;
	theDACStream.makeStream(theXDAQContextConfigTree_.getBackNode(theConfigurationPath_)
	                            .getNode("LinkToFEToDetectorTable"));
	//__CFG_COUT__ << std::endl;
	// NOTE This way we first upload all registers to the FPGA and then all together to
	// the ROCS
	std::set<unsigned int> fecChannel;
	bool                   anyOn = false;

	//__CFG_COUT__ << std::endl;
	for(DACStream::const_iterator it = theDACStream.getChannelStreamMap().begin();
	    it != theDACStream.getChannelStreamMap().end();
	    it++)
	{
		//__CFG_COUT__ << std::endl;
		if(it->second.getROCStatus())
		{
			anyOn = true;
			// FIXME This is not an hardware task since the hardware can be used by FEW
			// and FER  theHardware_.uploadDACsToFEW(it->first,it->second);//TODO Maybe
			// add in the firmware the DAC version so there is no need to copy the DACs
			// again std::vector<std::string> sendBuffer; std::vector<std::string>
			// receiveBuffer;
			std::vector<std::string> sendBuffer;
			// std::string sendBuffer;
			// std::string receiveBuffer;
			// string buffer;
			FSSRFirmware_->makeDACBuffer(sendBuffer, it->first, it->second);

			//__CFG_COUT__ << std::endl;
			FSSRHardware_->write(sendBuffer);
			//__CFG_COUT__ << std::endl;
			usleep(100000);

			//			FSSRHardware_->read(sendBuffer,receiveBuffer);
			//			std::vector<std::string>::iterator sendIt    = sendBuffer.begin();
			//			std::vector<std::string>::iterator receiveIt =
			// receiveBuffer.begin(); 			for(; sendIt!=sendBuffer.end(); sendIt++,
			// receiveIt++)
			//			{
			//				std::string toFix =
			// FSSRFirmware_->compareSendAndReceive(*sendIt, *receiveIt);
			// if(!toFix.empty())
			//				{
			//					std::string receivedFixed;
			//					FSSRHardware_->read(toFix,receivedFixed);
			//					std::string toFixAgain =
			// FSSRFirmware_->compareSendAndReceive(toFix, receivedFixed);
			//					if(!toFixAgain.empty())
			//					{
			//						__CFG_COUT__ << "ERROR: I tried to re-send " <<
			// std::endl;
			//
			//						for (unsigned int i = 0; i < toFixAgain.size(); i++)
			//							__CFG_COUT__ << (unsigned int)toFixAgain[i] <<
			// std::endl;
			//
			//						__CFG_COUT__ << " and got an error again!" <<
			// std::endl;
			//					}
			//				}
			//			}

			std::string maskBuffer;
			//__CFG_COUT__ << std::endl;
			FSSRFirmware_->makeMaskBuffer(maskBuffer, it->first, it->second);
			usleep(100000);

			//__CFG_COUT__ << std::endl;
			//			FSSRHardware_->writeAndAcknowledge(maskBuffer);
			FSSRHardware_->write(maskBuffer);
			//__CFG_COUT__ << std::endl;

			fecChannel.insert(it->first);
			// conbinedBuffer += buffer;
		}
	}

	//__CFG_COUT__ << std::endl;

	if(!anyOn)
		__CFG_COUT__ << "NOTE: No ROCs are on!" << std::endl;
	// FIXME This is not an hardware task since the hardware can be used by FEW and FER
	//    for(set<unsigned int>::const_iterator it=fecChannel.begin();
	//    it!=fecChannel.end(); it++)
	// uploadDACsToDetector(*it);

	__CFG_COUT__
	    << "Done Configuring Detector   ============================================="
	    << std::endl;
}

//========================================================================================================================
void FSSRInterface::halt(void)
{
	__CFG_COUT__ << "\tHalt" << std::endl;
	stop();
}

//========================================================================================================================
void FSSRInterface::pause(void)
{
	__CFG_COUT__ << "\tPause" << std::endl;
	stop();
}

//========================================================================================================================
void FSSRInterface::resume(void)
{
	__CFG_COUT__ << "\tResume" << std::endl;
	start("");
}

//========================================================================================================================
void FSSRInterface::start(std::string)  // runNumber)
{
	unsigned int i = VStateMachine::getIterationIndex();
	if(i == 0)
	{
		VStateMachine::indicateIterationWork();
		__CFG_COUT__ << "\tStart" << std::endl;
		//__CFG_COUT__ << "FE Detector config link = " <<
		// theXDAQContextConfigTree_.getBackNode(theConfigurationPath_).getNode("LinkToFEToDetectorTable")
		//<< std::endl;

		std::string csrRegisterBuffer;
		std::string csrRegisterRead;
		uint32_t    csrRegisterValue;

		FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
		csrRegisterValue =
		    FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer, csrRegisterRead);
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 0 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 0 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 0 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		if(csrRegisterValue == 0)
			__CFG_COUT__ << "\tERROR - Problem with register! See above" << std::endl;
		FSSRFirmware_->setCSRRegister(csrRegisterValue);

		FSSRHardware_->write(FSSRFirmware_->enableTrigger());
		csrRegisterRead.clear();
		FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
		csrRegisterValue =
		    FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer, csrRegisterRead);
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 1 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 1 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 1 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;

		//	usleep(50000);
		// NOW
		// FSSRHardware_->write(FSSRFirmware_->resetBCO());
		// usleep(50000);

		FSSRHardware_->write(FSSRFirmware_->armBCOReset());
		csrRegisterRead.clear();
		FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
		csrRegisterValue =
		    FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer, csrRegisterRead);
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 2 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 2 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 2 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		// std::string buffer;
		////LORE THIS ONE NEEDED BY RYAN OTS FSSR
		//((OtsUDPFirmwareCore*)FSSRFirmware_)->startBurst(buffer);
		// FSSRHardware_->write(buffer);

		csrRegisterRead.clear();
		FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
		csrRegisterValue =
		    FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer, csrRegisterRead);
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 3 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 3 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 3 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;

		// NOW
		FSSRHardware_->write(FSSRFirmware_->startStream(
		    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
		        .getNode("ChannelStatus0")
		        .getValue<bool>(),
		    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
		        .getNode("ChannelStatus1")
		        .getValue<bool>(),
		    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
		        .getNode("ChannelStatus2")
		        .getValue<bool>(),
		    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
		        .getNode("ChannelStatus3")
		        .getValue<bool>(),
		    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
		        .getNode("ChannelStatus4")
		        .getValue<bool>(),
		    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
		        .getNode("ChannelStatus5")
		        .getValue<bool>()));

		csrRegisterRead.clear();
		FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
		csrRegisterValue =
		    FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer, csrRegisterRead);
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 4 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 4 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 4 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;

		{
			std::string buffer;
			// LORE THIS ONE NEEDED BY RYAN OTS FSSR
			((OtsUDPFirmwareCore*)FSSRFirmware_)->startBurst(buffer);
			FSSRHardware_->write(buffer);
		}
		csrRegisterRead.clear();
		FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
		csrRegisterValue =
		    FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer, csrRegisterRead);
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 5 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 5 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> START 5 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
	}
	else if(i == 1)
	{
		//	return false;
		std::string csrRegisterBuffer;
		std::string csrRegisterRead;
		uint32_t    csrRegisterValue;

		csrRegisterBuffer = FSSRFirmware_->readCSRRegister();

		csrRegisterRead.clear();
		FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
		csrRegisterValue =
		    FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer, csrRegisterRead);
		FSSRFirmware_->setCSRRegister(csrRegisterValue);

		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> RUN   0 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> RUN   0 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> RUN   0 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		usleep(100000);

		csrRegisterRead.clear();
		FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
		csrRegisterValue =
		    FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer, csrRegisterRead);
		FSSRFirmware_->setCSRRegister(csrRegisterValue);
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> RUN   1 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> RUN   1 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> RUN   1 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;

		if(csrRegisterValue & 0x00080000)
		{
			csrRegisterRead.clear();
			FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
			csrRegisterValue = FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer,
			                                                          csrRegisterRead);
			FSSRFirmware_->setCSRRegister(csrRegisterValue);
			__CFG_COUT__ << FEVInterface::interfaceUID_
			             << " -> RUN   2 STRIP CSR Register value: 0x" << std::hex
			             << csrRegisterValue << std::dec << std::endl;
			__CFG_COUT__ << FEVInterface::interfaceUID_
			             << " -> RUN   2 STRIP CSR Register value: 0x" << std::hex
			             << csrRegisterValue << std::dec << std::endl;
			__CFG_COUT__ << FEVInterface::interfaceUID_
			             << " -> RUN   2 STRIP CSR Register value: 0x" << std::hex
			             << csrRegisterValue << std::dec << std::endl;
			usleep(1000000);
			if(VStateMachine::getSubIterationIndex() < 5)
			{
				VStateMachine::indicateSubIterationWork();
				return;
			}
		}

		csrRegisterRead.clear();
		FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
		csrRegisterValue =
		    FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer, csrRegisterRead);
		FSSRFirmware_->setCSRRegister(csrRegisterValue);
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> RUNNING 3 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> RUNNING 3 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> RUNNING 3 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;

		FSSRHardware_->write(FSSRFirmware_->resetBCO());
		//	std::string buffer;
		/*
		FSSRHardware_->write(FSSRFirmware_->startStream(
		        theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode("ChannelStatus0").getValue<bool>(),
		        theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode("ChannelStatus1").getValue<bool>(),
		        theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode("ChannelStatus2").getValue<bool>(),
		        theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode("ChannelStatus3").getValue<bool>(),
		        theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode("ChannelStatus4").getValue<bool>(),
		        theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode("ChannelStatus5").getValue<bool>()
		));
		 */
		// LORE THIS ONE NEEDED BY RYAN OTS FSSR
		//	((OtsUDPFirmwareCore*)FSSRFirmware_)->startBurst(buffer);
		//	FSSRHardware_->write(buffer);
	}
}

//========================================================================================================================
bool FSSRInterface::running(void)
{
	return false;
	std::string csrRegisterBuffer;
	std::string csrRegisterRead;
	uint32_t    csrRegisterValue;

	csrRegisterBuffer = FSSRFirmware_->readCSRRegister();

	csrRegisterRead.clear();
	FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
	csrRegisterValue =
	    FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer, csrRegisterRead);
	FSSRFirmware_->setCSRRegister(csrRegisterValue);

	__CFG_COUT__ << FEVInterface::interfaceUID_
	             << " -> RUN   0 STRIP CSR Register value: 0x" << std::hex
	             << csrRegisterValue << std::dec << std::endl;
	__CFG_COUT__ << FEVInterface::interfaceUID_
	             << " -> RUN   0 STRIP CSR Register value: 0x" << std::hex
	             << csrRegisterValue << std::dec << std::endl;
	__CFG_COUT__ << FEVInterface::interfaceUID_
	             << " -> RUN   0 STRIP CSR Register value: 0x" << std::hex
	             << csrRegisterValue << std::dec << std::endl;
	usleep(100000);

	csrRegisterRead.clear();
	FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
	csrRegisterValue =
	    FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer, csrRegisterRead);
	FSSRFirmware_->setCSRRegister(csrRegisterValue);
	__CFG_COUT__ << FEVInterface::interfaceUID_
	             << " -> RUN   1 STRIP CSR Register value: 0x" << std::hex
	             << csrRegisterValue << std::dec << std::endl;
	__CFG_COUT__ << FEVInterface::interfaceUID_
	             << " -> RUN   1 STRIP CSR Register value: 0x" << std::hex
	             << csrRegisterValue << std::dec << std::endl;
	__CFG_COUT__ << FEVInterface::interfaceUID_
	             << " -> RUN   1 STRIP CSR Register value: 0x" << std::hex
	             << csrRegisterValue << std::dec << std::endl;

	if(csrRegisterValue & 0x00080000)
	{
		csrRegisterRead.clear();
		FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
		csrRegisterValue =
		    FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer, csrRegisterRead);
		FSSRFirmware_->setCSRRegister(csrRegisterValue);
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> RUN   2 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> RUN   2 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		__CFG_COUT__ << FEVInterface::interfaceUID_
		             << " -> RUN   2 STRIP CSR Register value: 0x" << std::hex
		             << csrRegisterValue << std::dec << std::endl;
		usleep(1000000);
		return true;
	}

	csrRegisterRead.clear();
	FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
	csrRegisterValue =
	    FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer, csrRegisterRead);
	FSSRFirmware_->setCSRRegister(csrRegisterValue);
	__CFG_COUT__ << FEVInterface::interfaceUID_
	             << " -> RUNNING 3 STRIP CSR Register value: 0x" << std::hex
	             << csrRegisterValue << std::dec << std::endl;
	__CFG_COUT__ << FEVInterface::interfaceUID_
	             << " -> RUNNING 3 STRIP CSR Register value: 0x" << std::hex
	             << csrRegisterValue << std::dec << std::endl;
	__CFG_COUT__ << FEVInterface::interfaceUID_
	             << " -> RUNNING 3 STRIP CSR Register value: 0x" << std::hex
	             << csrRegisterValue << std::dec << std::endl;

	FSSRHardware_->write(FSSRFirmware_->resetBCO());
	//	std::string buffer;
	/*
	FSSRHardware_->write(FSSRFirmware_->startStream(
	        theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode("ChannelStatus0").getValue<bool>(),
	        theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode("ChannelStatus1").getValue<bool>(),
	        theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode("ChannelStatus2").getValue<bool>(),
	        theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode("ChannelStatus3").getValue<bool>(),
	        theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode("ChannelStatus4").getValue<bool>(),
	        theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode("ChannelStatus5").getValue<bool>()
	));
	 */
	// LORE THIS ONE NEEDED BY RYAN OTS FSSR
	//	((OtsUDPFirmwareCore*)FSSRFirmware_)->startBurst(buffer);
	//	FSSRHardware_->write(buffer);
	return false;
}

//========================================================================================================================
void FSSRInterface::stop(void)
{
	__CFG_COUT__ << "\tStop" << std::endl;
	std::string csrRegisterBuffer;
	std::string csrRegisterRead;
	uint32_t    csrRegisterValue;

	csrRegisterRead.clear();
	FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
	csrRegisterValue =
	    FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer, csrRegisterRead);
	FSSRFirmware_->setCSRRegister(csrRegisterValue);
	__CFG_COUT__ << FEVInterface::interfaceUID_
	             << " -> STOP 0 STRIP CSR Register value: 0x" << std::hex
	             << csrRegisterValue << std::dec << std::endl;
	__CFG_COUT__ << FEVInterface::interfaceUID_
	             << " -> STOP 0 STRIP CSR Register value: 0x" << std::hex
	             << csrRegisterValue << std::dec << std::endl;
	__CFG_COUT__ << FEVInterface::interfaceUID_
	             << " -> STOP 0 STRIP CSR Register value: 0x" << std::hex
	             << csrRegisterValue << std::dec << std::endl;

	FSSRHardware_->write(FSSRFirmware_->stopStream());
	FSSRHardware_->read(FSSRFirmware_->readCSRRegister(), csrRegisterRead);
	csrRegisterValue =
	    FSSRFirmware_->createRegisterFromValue(csrRegisterBuffer, csrRegisterRead);
	FSSRFirmware_->setCSRRegister(csrRegisterValue);
	__CFG_COUT__ << FEVInterface::interfaceUID_
	             << " -> STOP 1 STRIP CSR Register value: 0x" << std::hex
	             << csrRegisterValue << std::dec << std::endl;
	__CFG_COUT__ << FEVInterface::interfaceUID_
	             << " -> STOP 1 STRIP CSR Register value: 0x" << std::hex
	             << csrRegisterValue << std::dec << std::endl;
	__CFG_COUT__ << FEVInterface::interfaceUID_
	             << " -> STOP 1 STRIP CSR Register value: 0x" << std::hex
	             << csrRegisterValue << std::dec << std::endl;
	std::string buffer;
	// LORE THIS ONE NEEDED BY RYAN OTS FSSR
	((OtsUDPFirmwareCore*)FSSRFirmware_)->stopBurst(buffer);
	FSSRHardware_->write(buffer);
}

//========================================================================================================================
// NOTE: buffer for address must be at least size universalAddressSize_
// NOTE: buffer for returnValue must be max UDP size to handle return possibility
void ots::FSSRInterface::universalRead(char* address, char* returnValue)
{
	__CFG_COUT__ << "address size " << universalAddressSize_ << std::endl;

	__CFG_COUT__ << "Request: ";
	for(unsigned int i = 0; i < universalAddressSize_; ++i)
		printf("%2.2X", (unsigned char)address[i]);
	std::cout << std::endl;

	std::string readBuffer(universalDataSize_, 0);  // 0 fill to correct number of bytes
	FSSRHardware_->read(FSSRFirmware_->universalRead(address), readBuffer);  // data reply

	__CFG_COUT__ << "Result SIZE: " << readBuffer.size() << std::endl;
	memcpy(returnValue, readBuffer.substr(2).c_str(), universalDataSize_);
}

//========================================================================================================================
// NOTE: buffer for address must be at least size universalAddressSize_
// NOTE: buffer for writeValue must be at least size universalDataSize_
void ots::FSSRInterface::universalWrite(char* address, char* writeValue)
{
	__CFG_COUT__ << "address size " << universalAddressSize_ << std::endl;
	__CFG_COUT__ << "data size " << universalDataSize_ << std::endl;
	__CFG_COUT__ << "Sending: ";
	for(unsigned int i = 0; i < universalAddressSize_; ++i)
		printf("%2.2X", (unsigned char)address[i]);
	std::cout << std::endl;

	FSSRHardware_->write(
	    FSSRFirmware_->universalWrite(address, writeValue));  // data request
}

DEFINE_OTS_INTERFACE(FSSRInterface)
