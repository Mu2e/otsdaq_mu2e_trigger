//#include "otsdaq/DetectorWriter/FrontEndInterfaceTemplate.h"
//
//#include "otsdaq/DAQHardware/FrontEndHardwareTemplate.h"
//#include "otsdaq/DAQHardware/FrontEndFirmwareTemplate.h"

#include "otsdaq-core/Macros/InterfacePluginMacros.h"
#include "otsdaq-trigger/FEInterfaces/FTBFWireChamberInterface.h"
#include "otsdaq-trigger/TCPSocket/SocketException.hh"
#include "otsdaq-trigger/TCPSocket/TCPSocket.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "FE-FTBFWireChamberInterface"

//========================================================================================================================
FTBFWireChamberInterface::FTBFWireChamberInterface(
    const std::string&            interfaceUID,
    const ots::ConfigurationTree& theXDAQContextConfigTree,
    const std::string&            interfaceConfigurationPath)
    : Socket(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                 .getNode("HostIPAddress")
                 .getValue<std::string>(),
             theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                 .getNode("HostPort")
                 .getValue<unsigned int>())
    , FEVInterface(interfaceUID, theXDAQContextConfigTree, interfaceConfigurationPath)
    , UDPDataStreamerBase(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                              .getNode("HostIPAddress")
                              .getValue<std::string>(),
                          theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                              .getNode("HostPort")
                              .getValue<unsigned int>(),
                          theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                              .getNode("StreamToIPAddress")
                              .getValue<std::string>(),
                          theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                              .getNode("StreamToPort")
                              .getValue<unsigned int>())
    , host_(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                .getNode("InterfaceIPAddress")
                .getValue<std::string>())
    , port_(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                .getNode("InterfacePort")
                .getValue<unsigned int>())
{
	//	theFrontEndHardware_ = new FrontEndHardwareTemplate();
	//	theFrontEndFirmware_ = new FrontEndFirmwareTemplate();
	universalAddressSize_ = 4;
	universalDataSize_    = 4;

	tdcReadoutBusyMask_ = 0x1;
	tdcReadoutDoneMask_ = 0x2;

	readoutDoneBit_ = true;
	NUMBER_OF_TDCs  = 16;

	__COUT__ << "Finished constructor" << std::endl;

	//	{
	//		FILE *fp = fopen("/home/otsdaq/tsend.txt", "w");
	//		if (fp)fclose(fp);
	//	}
	//	{
	//		FILE *fp = fopen("/home/otsdaq/tsend0.txt", "w");
	//		if (fp)fclose(fp);
	//	}
}

//========================================================================================================================
FTBFWireChamberInterface::~FTBFWireChamberInterface(void)
{
	// delete theTCPSocket_;
}

//========================================================================================================================
void FTBFWireChamberInterface::configure(void)
{
	/// Steps to success:
	//	 1. copy the link I gave in gchat to where I told you in gchat
	//		- this becomes the hardware emulator
	//		- run the emulator
	//	 2. understand an example client code, that RAR will send you the link to right
	// now
	//		- this becomes the software emulator
	//	 3. Have Configure connect in the same way as the software emulator and "read" the
	// TDC status

	//	host_ = "ftbfwc01";
	//	port_ = 5000;

	__COUT__ << "Interface Port: "
	         << theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                .getNode("InterfacePort")
	                .getValue<unsigned int>()
	         << std::endl;
	__COUT__ << "Interface IP Address: "
	         << theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                .getNode("InterfaceIPAddress")
	                .getValue<std::string>()
	         << std::endl;

	theTCPSocket_.reset(new TCPSocket(5000));

	// Connect
	if(!theTCPSocket_->createSocket(host_, port_))
	{
		// throw SocketException ( "Could not create socket." );
		__COUT__ << "Could not create socket." << std::endl;
		// createTDCList()
	}
	else
	{
		theTCPSocket_->clearReadSocket();
	}

	//	if ( ! theTCPSocket_-> receiveOnSocket())
	//	{
	//	  throw SocketException ( "Could not bind to port." );
	//	}
	__COUT__ << "Configuration finished!" << std::endl;

	// __COUT__ << "CSR test" << std::endl;
	// setCSR(0x8);

	// __COUT__ << "CSR test" << std::endl;
	// int val = getCSR();
	// __COUT__ << "CSR test val=" << std::hex << val << std::endl;
}

//========================================================================================================================
void FTBFWireChamberInterface::halt(void)
{
	/*	__COUT__ <<
	"00000000======================================================================================SUCCCAAA========================================================"
	<< std::endl; if(theTCPSocket_)
	{
	    __COUT__ <<
	"11111111======================================================================================SUCCCAAA========================================================"
	<< std::endl; delete(theTCPSocket_);
	}
	 */
	//- ** Sockets should always send 'QUIT' Cmd before closing **

	sendMessageWithoutReply("QUIT");
	theTCPSocket_.reset(nullptr);
}

//========================================================================================================================
void FTBFWireChamberInterface::pause(void) {}

//========================================================================================================================
void FTBFWireChamberInterface::resume(void)
{
	// they don't have a resume function implemented
}

//========================================================================================================================
void FTBFWireChamberInterface::start(std::string runNumber /*Run Number*/)
{
	checkTdcLinkStatus();

	// Configure controller
	// Reset FIFO, Sequencer, Spill Counter
	// Free run, test Pulser disabled, FM Tx disabled, no external PLL ref
	// Configure TDC's
	setCSR(0xe4);

	// TDC Reset All
	tdcResetAll();

	//	__COUT__ << getId() 	<< std::endl;
	//	__COUT__ << getHelp() 	<< std::endl;
	//	__COUT__ << getSock() 	<< std::endl;
	//	__COUT__ << getTime() 	<< std::endl;
	//	__COUT__ << getPower() 	<< std::endl;
	//	__COUT__ << getHeaders()<< std::endl;
	//	__COUT__ << getTdcData()<< std::endl;
	//	__COUT__ << getuCData() << std::endl;
	//	__COUT__ << getNetworkInfo() << std::endl;

	bool pulserEnabled = theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                         .getNode("UseTestPulser")
	                         .getValue<bool>();
	// turnOnTestPulser()
	if(pulserEnabled)
	{
		__COUT__ << "Turn on test pulser" << std::endl;
		int val                  = getCSR();
		int testPulserEnableMask = 0x8;
		int data                 = val & testPulserEnableMask;
		setCSR(data);
	}

	// return;// CM_SUCCESS;
}

//========================================================================================================================
void FTBFWireChamberInterface::stop(void) {}

//========================================================================================================================
// RDB WrdCnt  Read Spill Data(16bit BINARY),(optional)WrdCnt= # wrds to rd
//            1st Sends 9 word Control Header + Spill Header(s), then Data
//            if no WrdCnt RDB returns WrdCnt= Last Spills Word Cnt
//            if WrdCnt==0, Stop any Active Req, WrdCnt= 9-0x20000000 Max
//            if Spill Gate is Open, returns only 9 word NULL Header
// RD  a       FPGA Addr(Hex), a=A16, returns=Data16
bool FTBFWireChamberInterface::running(void)
{
	std::string readBuffer;
	int         count = 0;
	while(WorkLoop::continueWorkLoop_)
	{
		std::string data = "NODATA";

		if(checkForNewData())
		{
			data = "MWPCDATA";
			// Check for spill end and read out if spill over
			int spillNumber       = getSpillCount();
			int wordCount         = getWordCount();
			int spillTriggerCount = getSpillTriggerCount();
			// std::string spillMessage = createSpillMessage(spillNumber,
			// std::to_string(spillTriggerCount) + " triggers", std::to_string(wordCount)
			// + " words");
			__MOUT__ << "Spill Number " << spillNumber << " : " << spillTriggerCount
			         << " triggers " << wordCount << " words" << std::endl;

			size_t bytesToRead = wordCount * 2;

			while(bytesToRead > 0 && WorkLoop::continueWorkLoop_)
			{
				bytesToRead -= getData(data, bytesToRead);
			}

			if(data.size() < static_cast<size_t>(wordCount) * 2)
			{
				__MOUT__ << "I only read " << std::to_string(data.size())
				         << " bytes out of an expected " << std::to_string(wordCount * 2)
				         << "!";
			}
			__MOUT__ << "Sending " << std::to_string(data.size()) << " bytes of data";
		}
		else if(WorkLoop::continueWorkLoop_)
		{
			usleep(1000000);  // sleep //wait one second before checking for end spill
			__MOUT__ << "sending no data" << std::endl;
		}

		if(0)  // binary output
		{
			std::ofstream output;
			std::string   outputPath = "/home/otsdaq/tsend.txt";
			output.open(outputPath, std::ios::out | std::ios::app | std::ios::binary);
			output << data;
		}

		if(0)  // debug forwarding
		{
			char str[5];
			for(unsigned int j = 0; j < data.length(); ++j)
			{
				sprintf(str, "%2.2x", ((unsigned int)data[j]) & ((unsigned int)(0x0FF)));

				if(j % 64 == 0)
					std::cout << "SEND " << j << "\t: 0x\t";
				std::cout << str;
				if(j % 8 == 7)
					std::cout << " ";
				if(j % 64 == 63)
					std::cout << std::endl;
			}
			std::cout << std::endl;
			std::cout << std::endl;
		}

		if(data != "NODATA")
		{  // Let's not send nulls for now
			TransmitterSocket::send(streamToSocket_, data);
		}
	}
	__MOUT__ << "Ending FTBFWireChamberInterface workloop." << std::endl;

	return false;  // WorkLoop::continueWorkLoop_;//otherwise it stops!!!!!
}

//========================================================================================================================
// universalRead
//	Must implement this function for Macro Maker to work with this interface.
//	When Macro Maker calls:
//		- address will be a [universalAddressSize_] byte long char array
//		- returnValue will be a [universalDataSize_] byte long char array
//		- expects return value of 0 on success and negative numbers on failure
void FTBFWireChamberInterface::universalRead(char* address, char* returnValue)
{
	std::string retStr;
	retStr.resize(universalDataSize_);
	retStr = std::to_string(readAddress(std::string(address)));
	memcpy(returnValue, &retStr[0], universalDataSize_);
}

//========================================================================================================================
// universalWrite
//	Must implement this function for Macro Maker to work with this interface.
//	When Macro Maker calls:
//		- address will be a [universalAddressSize_] byte long char array
//		- writeValue will be a [universalDataSize_] byte long char array
void FTBFWireChamberInterface::universalWrite(char* address, char* writeValue)
{
	writeAddress(std::string(address), std::string(writeValue));
	return;
	// TODO - implement the write for this interface
}

//========================================================================================================================
bool FTBFWireChamberInterface::checkForNewData()
{
	checkForReadoutError();
	if(!theTCPSocket_->is_valid())
	{
		__MOUT__ << "The TCP Socket Connection is invalid!" << std::endl;

		return false;
	}

	//	__COUT__ << "The TCP Socket Connection is valid! Checking for new data!" <<
	// std::endl;
	bool oldReadoutDoneBit = readoutDoneBit_;
	readoutDoneBit_        = tdcReadoutDone();
	readoutBusyCount_      = readoutBusyCount_ + tdcReadoutBusy();

	//	__COUT__ << "ReadoutDoneBit: " << readoutDoneBit_ << std::endl;
	//	__COUT__ << "ReadoutBusyCount: " << readoutBusyCount_ << std::endl;

	if(!oldReadoutDoneBit && readoutDoneBit_)
	{
		__COUT__ << "Found new data!" << std::endl;
		readoutBusyCount_ = 0;
		return true;
	}
	__COUT__ << "Didn't find new data! :(" << std::endl;
	return false;
}

//========================================================================================================================
void FTBFWireChamberInterface::checkForReadoutError()
{
	if(readoutBusyCount_ > 40)
	{
		readoutBusyCount_ = 0;
		__MOUT__ << "\n\n\n\n\n\nRuntime Error: Wire chamber readout has stalled!"
		         << std::endl;
		__MOUT__ << "Runtime Error: Wire chamber readout has stalled! \n\n\n\n\n\n"
		         << std::endl;
		// Raise exception
		return;
	}
	return;
}

//========================================================================================================================
bool FTBFWireChamberInterface::tdcReadoutBusy()
{
	if(getSpillState() & tdcReadoutBusyMask_)
		return 1;
	else
		return 0;
}

//========================================================================================================================
bool FTBFWireChamberInterface::tdcReadoutDone()
{
	if(getSpillState() & tdcReadoutDoneMask_)
		return 1;
	else
		return 0;
}

//========================================================================================================================
// Data comes back in binary format
size_t FTBFWireChamberInterface::getData(std::string& physicsData, size_t totalByteCount)
{
	int maxByteCount = 4096;

	// Start the data transfer from the controller
	std::string message = "rdb";
	sendMessageWithoutReply(message);

	__COUT__ << "totalByteCount: " << totalByteCount << std::endl;

	// Read in the data
	int  remainingBytes = totalByteCount;
	int  sts            = 1;
	char str[5];
	while(remainingBytes > 0 && sts > 0)  // Retrieve data in maxByteCount chunk
	{
		std::string binaryData = "";
		//__COUT__ << "maxByteCount=" << maxByteCount << " remainingBytes=" <<
		// remainingBytes << std::endl;

		sts = theTCPSocket_->read(
		    remainingBytes > maxByteCount ? maxByteCount : remainingBytes, binaryData);

		//__COUT__ << "binaryData.length(): " << binaryData.length() << " " << sts <<
		// std::endl;

		//		{
		//			std::ofstream output;
		//			std::string outputPath = "/home/otsdaq/tsend0.txt";
		//			output.open(outputPath, std::ios::out | std::ios::app |
		// std::ios::binary); 			output << binaryData;
		//		}
		//
		//		for(unsigned int j=0;j<binaryData.length();++j)
		//		{
		//			sprintf(str,"%2.2x",((unsigned int)binaryData[j]) & ((unsigned
		// int)(0x0FF)));
		//
		//			if(j%64 == 0) std::cout << "0x\t";
		//			std::cout << str;
		//			if(j%8 == 7) std::cout << " ";
		//			if(j%64 == 63) std::cout << std::endl;
		//		}
		//		std::cout << std::endl;
		//		std::cout << std::endl;
		//		__COUT__ << "binaryData: " << binaryData <<  std::endl;

		remainingBytes = remainingBytes - binaryData.length();

		//__COUT__ << "Byte-swapping string so it will be correct!" << std::endl;
		for(size_t ii = 0; ii < binaryData.size(); ii += 2)
		{
			auto ptr = reinterpret_cast<uint16_t*>(&binaryData[0] + ii);
			*ptr     = ntohs(*ptr);
		}

		physicsData += binaryData;
	}

	if(remainingBytes > 0)
	{
		__COUT__ << "Error reading data!" << std::endl;
	}
	else
	{
		__COUT__ << "SUCCCA" << std::endl;
	}

	__MOUT__ << "getData Returning " << std::to_string(totalByteCount - remainingBytes)
	         << " bytes read of " << std::to_string(totalByteCount) << " total ("
	         << std::to_string(remainingBytes) << " remain)." << std::endl;
	return totalByteCount - remainingBytes;
}

//========================================================================================================================
//================================================== WireChamberDaq
//======================================================
//========================================================================================================================
int FTBFWireChamberInterface::getSpillCount()
{
	std::string addr = "7";
	return readAddress(addr);
}

//========================================================================================================================
int FTBFWireChamberInterface::getSpillTriggerCount()
{
	return readDoubleRegister("4", "5");
}

//========================================================================================================================
int FTBFWireChamberInterface::getWordCount() { return readDoubleRegister("e", "f"); }

//========================================================================================================================
int FTBFWireChamberInterface::getSpillState()
{
	std::string address  = "b";
	int         response = readAddress(address);
	__COUT__ << response << std::endl;
	return response;
}

//========================================================================================================================
std::string FTBFWireChamberInterface::createSpillMessage(int spillNum, std::string text1)
{
	std::string spillMessage =
	    "spill " + std::to_string(spillNum) + " -- " + text1 + " -- ";
	return spillMessage;
}
std::string FTBFWireChamberInterface::createSpillMessage(int         spillNum,
                                                         std::string text1,
                                                         std::string text2)
{
	std::string spillMessage =
	    ("spill " + std::to_string(spillNum) + " -- " + text1 + " -- " + text2);
	return spillMessage;
}

//========================================================================================================================
//=========================================== Controller Register I/O
//===================================================
//========================================================================================================================
// readaddress (std::string address)
// writeaddress(std::string address, std::string data)

//========================================================================================================================
//   Read register on controller.
//
//	readAddress actually gets ascii response (From uP)
//		for example, a 16 bit value of 10 will be returned as "000A" = 0x30 0x30 0x30 0x61
//   address = register address in hex
//   RD  a       FPGA Addr(Hex), a=A16, returns=Data16
int FTBFWireChamberInterface::readAddress(std::string address)
{
	std::string message = "rd " + address;
	std::string rawVal  = sendMessageWithReply(message, 4096);
	__COUT__ << "ASCII readAddress " << address << " response " << rawVal << std::endl;
	int retVal = -1;
	try
	{
		retVal = std::stol(rawVal.c_str(), nullptr, 16);
	}
	catch(...)
	{
		__COUT_WARN__ << "Illegal response." << std::endl;
	}
	return retVal;
}

//========================================================================================================================
//    WR  a d     Write Addr16(H) Data16(H)
void FTBFWireChamberInterface::writeAddress(std::string address, std::string data)
{
	__COUT__ << "ASCII writeAddress " << address << " data " << data << std::endl;
	std::string message = "wr " + address + " " + data;
	sendMessageWithoutReply(message);
	return;
}

//========================================================================================================================
//	Read multiple consecutive addresses.
//	RDI adr c   Read/and Incr FPGA Addr(H), c=WrdCnt(D)
std::vector<std::string> FTBFWireChamberInterface::readMultiAddress(std::string address,
                                                                    int         count)
{
	std::string              message = "rdi " + address + " " + std::to_string(count);
	std::string              rawVal  = sendMessageWithReply(message, 4096);
	std::vector<std::string> listVal;
	// std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<string>(),
	// std::back_inserter(tokens));
	std::stringstream                  ss(rawVal);
	std::istream_iterator<std::string> begin(ss);
	std::istream_iterator<std::string> end;
	std::vector<std::string>           vstrings(begin, end);
	std::copy(vstrings.begin(),
	          vstrings.end(),
	          std::ostream_iterator<std::string>(std::cout, ","));

	return listVal;
}

//========================================================================================================================
// Controller has 16-bit words.  Use two registers for a 32-bit value.
int FTBFWireChamberInterface::readDoubleRegister(std::string hiAddr, std::string loAddr)
{
	int hiVal = readAddress(hiAddr);
	int loVal = readAddress(loAddr);

	__COUT__ << "hiVal = 0x" << std::hex << hiVal << " " << std::dec << hiVal
	         << std::endl;
	__COUT__ << "loVal = 0x" << std::hex << loVal << " " << std::dec << loVal
	         << std::endl;

	return (hiVal << 16) + loVal;
	return 1;
}
//========================================================================================================================
//============================================ Socket Communication
//=====================================================
//========================================================================================================================
//- Test Beam Controller Command List (Help= HE,H1,H2,H3) -
//- ** Sockets should always send 'QUIT' Cmd before closing **
// Read
// RDB WrdCnt  Read Spill Data(16bit BINARY),(optional)WrdCnt= # wrds to rd
// 1st Sends 9 word Control Header + Spill Header(s), then Data
// if no WrdCnt RDB returns WrdCnt= Last Spills Word Cnt
// if WrdCnt==0, Stop any Active Req, WrdCnt= 9-0x20000000 Max
// if Spill Gate is Open, returns only 9 word NULL Header
// RD  a       FPGA Addr(Hex), a=A16, returns=Data16
// Write
// WR  a d     Write Addr16(H) Data16(H)
// HC  p s     Write ASCII Command String to TDC Port, p=1-16, s=TDC64 Cmd
// Read Pool
// P0          Display current Controller and TDC Spill Headers, P3=Totals
// P1 t        Pooled TDC Data(ASC) active ports, t=1=noTextHdrs, Grps of 16
// P2 t        Pooled  uC Data(ASC) active ports, t=1=noTextHdrs, Grps of 16
// PWR         Display POE Ports Power
// PWRRST      PWRRST= Resets/Power Cycles all TDC POE ports
// Misc
// RDB1 ad cnt Read Binary Data FIFO, Addr(H), WordCount(H)
// RDM adr c   Read/NoIncr FPGA Addr(H), c=WrdCnt(D)
// RDI adr c   Read/and Incr FPGA Addr(H), c=WrdCnt(D)
// RDML adr c  32bit ver of RDM, addr= Upper 16bit Port
// RDD c n     Read DDR Data Port(clrs RdPtrs 74,75),c=Cnt(D) n=1(8wrd/line)
// ADC n       Display uC temperature and ADC channels, n=1=noText
// FAN n       Chassis Fans Full Speed, n=1 Enable, n=0 Disable
// DSAV p      Save Device Setup fpga regs to Flash, page 1-10, 1=BootPage
// FL          USB Flash load with Altera binary file (*.rbf) (USB only)
// FF          Re-Load FPGA from Atmel Flash memory
// FD          USB FPGA direct load of Altera binary file (*.rbf) (USB only)
// FMR page    Flash Memory reads (DSAV page 1..10), (11 Holds Network setup
// FMR page 1  Read 256 byte page of Flashed fpga file data, page(d)=0-~1000
// FR s        Check sum of fpga file in Flash,(optional s read siz in bytes)
//========================================================================================================================
void FTBFWireChamberInterface::sendMessageWithoutReply(std::string message)
{
	__COUT__ << "sendMessageWithoutReply: " << message << std::endl;
	theTCPSocket_->send(message);
	return;
}

//========================================================================================================================
std::string FTBFWireChamberInterface::sendMessageWithReply(std::string message,
                                                           int         bufferSize)
{
	__COUT__ << "sendMessageWithReply: " << message << std::endl;
	std::string response = "";
	theTCPSocket_->send(message);
	usleep(100000);  // Fixme only wait as long as we don't have a response + add a
	                 // timeout
	theTCPSocket_->read(bufferSize, response);

	char msg[100];

	__SS__ << "Response: ";
	for(unsigned int i = 0; i < response.size(); ++i)
	{
		sprintf(msg, "0x%2.2x", (unsigned int)response[i]);
		ss << msg << " ";
	}
	__COUT__ << "\n" << ss.str() << std::endl;
	return response;
}

//========================================================================================================================
// Write to the control and status register
void FTBFWireChamberInterface::setCSR(unsigned int data)
{
	std::string address = "1";

	// Always set bit 2 - Enable the Send 2 FM transmitters 1: enabled, disabled
	// If this bit is not set there will be no readout.

	char msg[100];
	sprintf(msg, "%X", data | 4);
	writeAddress(address, msg);
	return;
}

//========================================================================================================================
// Read from the control and status register
int FTBFWireChamberInterface::getCSR()
{
	std::string address = "1";
	return readAddress(address);
}

/////////////////// Controller CSR (0x1) /////////////////////
// 7       6       5       4       3       2       1       0
// FIFO    SEQ     Spill   Free    Test    FM      PLL     DMA
// Reset   Reset   Counter Run     Pulser  Tx      Ref     Busy
//                 Reset           Enable  Enable  Source
///////////////////////////////////////////////////////////////
/////////////////// TDC CSR (0x0) /////////////////////////////
// 7       6       5       4       3       2       1       0
// FIFO    SEQ     Spill   Free    Test    FM      PLL     DMA
// Reset   Reset   Counter Run     Pulser  Tx      Ref     Busy
//                 Reset           Enable  Enable  Source
///////////////////////////////////////////////////////////////

//========================================================================================================================
int FTBFWireChamberInterface::checkTdcLinkStatus()
{
	int                      errors = 0;
	std::vector<std::string> status = getTdcLinkStatus();
	for(unsigned int it = 0; it < status.size(); it++)
		if(status[it] != "4c")
			errors++;

	linkStatus_ = status;

	__COUT__ << "Errors: " << std::to_string(errors) << std::endl;

	return errors;
}

//========================================================================================================================
std::vector<std::string> FTBFWireChamberInterface::getTdcLinkStatus()
{
	return readMultiAddress("20", 16);
}

//========================================================================================================================
//============================================ TDC Registers
//============================================================
//========================================================================================================================

//========================================================================================================================
// HC  p s     Write ASCII Command String to TDC Port, p=1-16, s=TDC64 Cmd
int FTBFWireChamberInterface::readTdc(std::string port, std::string address)
{
	std::string message;
	message                  = "hc " + port + " rd " + address;
	std::string       rawVal = sendMessageWithReply(message, 4096);
	unsigned int      value;
	std::stringstream ss;
	ss << std::hex << rawVal;
	ss >> value;

	return value;
}

//========================================================================================================================
// HC  p s     Write ASCII Command String to TDC Port, p=1-16, s=TDC64 Cmd
void FTBFWireChamberInterface::writeTdc(std::string port,
                                        std::string address,
                                        std::string data)
{
	std::string message = "hc " + port + " wr " + address + " " + data;
	sendMessageWithoutReply(message);
	usleep(100000);
	return;
}

//========================================================================================================================
// Reading all TDC values does not work over the network.
// When a TDC is not attached the port returns no value and
// the program hangs.
// The "hc 0" commands (send to all TDCs) can't be used here.
void FTBFWireChamberInterface::tdcResetAll()
{
	writeTdc("0", "0", "27");
	return;
}

////========================================================================================================================
////Function to reset variables between runs
// void FTBFWireChamberInterface::reset_run()
//{
//
//}
////========================================================================================================================
////Function to reset variables between runs
// int FTBFWireChamberInterface::rpc_MT_bor(int runNumber )
//{
//
//}
//
////========================================================================================================================
////Make socket connection to spill trigger server
// int FTBFWireChamberInterface::connectToTriggerServer() {
//
//}
//
////========================================================================================================================
////Disconnect for the trigger server socket
// int FTBFWireChamberInterface::disconnectFromTriggerServer() {
//
//}
//
////========================================================================================================================
////Function to send messages over socket
// int FTBFWireChamberInterface::sendMessage(std::string msg) {
//
//}
//
////========================================================================================================================
////Function to get time from system (nanosec precision - if supported by OS/HW)
// std::chrono::high_resolution_clock::time_point FTBFWireChamberInterface::getTime() {
//
//}

DEFINE_OTS_INTERFACE(FTBFWireChamberInterface)
