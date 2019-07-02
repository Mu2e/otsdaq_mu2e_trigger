#ifndef _ots_FTBFWireChamberInterface_h_
#define _ots_FTBFWireChamberInterface_h_

#include "otsdaq-core/FECore/FEVInterface.h"
#include "otsdaq-core/NetworkUtilities/UDPDataStreamerBase.h"
//#include "otsdaq-demo/MidasDependencies/ClientSocket.hh"
//#include "otsdaq-demo/MidasDependencies/SocketException.hh"

#include <chrono>
#include "otsdaq-trigger/TCPSocket/TCPSocket.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ots
{
struct TDCHeaderStruct
{
	unsigned int tdcSpillWordCount;
	unsigned int tdcNumber;
	unsigned int tdcSpillTriggerCount;
	unsigned int tdcSpillStatus;
	unsigned int tdcWords;
	unsigned int tdcHeaderIndex;
};

struct TDCEvent
{
	unsigned int              wordCount;
	unsigned int              tdcNumber;
	unsigned int              eventStatus;
	unsigned int              triggerNumber;  // 32 bit number
	unsigned int              triggerType;
	unsigned int              controllerEventTimeStamp;
	unsigned int              tdcEventTimeStamp;  // 32 bit number
	unsigned int              dataWords;
	unsigned int              words;
	std::vector<unsigned int> tdcData;
};

class FTBFWireChamberInterface : public ots::FEVInterface, public ots::UDPDataStreamerBase
{
  public:
	FTBFWireChamberInterface(const std::string&            interfaceUID,
	                         const ots::ConfigurationTree& theXDAQContextConfigTree,
	                         const std::string&            interfaceConfigurationPath);
	virtual ~FTBFWireChamberInterface(void);

	void configure(void);
	void halt(void);
	void pause(void);
	void resume(void);
	void start(std::string runNumber);
	void stop(void);
	bool running(void);

	void universalRead(char* address, char* readValue) override;
	void universalWrite(char* address, char* writeValue) override;

  private:
	bool checkForNewData();
	void checkForReadoutError();
	bool tdcReadoutBusy();
	bool tdcReadoutDone();
	//
	size_t getData(std::string& data, size_t byteCount);
	void   readOut();

	int getSpillCount();
	int getSpillTriggerCount();
	int getWordCount();
	int getSpillState();

	std::string createSpillMessage(int spillNum, std::string text1);
	std::string createSpillMessage(int spillNum, std::string text1, std::string text2);

	int                      readAddress(std::string address);
	void                     writeAddress(std::string address, std::string data);
	std::vector<std::string> readMultiAddress(std::string address, int count);
	int                      readDoubleRegister(std::string hiAddr, std::string loAddr);

	void        sendMessageWithoutReply(std::string message);
	std::string sendMessageWithReply(std::string message, int bufferSize);

	void                     setCSR(unsigned int data);
	int                      getCSR();
	int                      checkTdcLinkStatus();
	std::vector<std::string> getTdcLinkStatus();
	void                     tdcResetAll();
	int                      readTdc(std::string port, std::string address);
	void writeTdc(std::string port, std::string address, std::string data);

	// Controller Communication
	std::string getId() { return sendMessageWithReply("id", 4096); }
	std::string getHelp() { return sendMessageWithReply("he", 4096); }
	std::string getSock() { return sendMessageWithReply("sock", 4096); }
	std::string getTime() { return sendMessageWithReply("time", 4096); }
	std::string getPower() { return sendMessageWithReply("pwr", 4096); }
	std::string getHeaders() { return sendMessageWithReply("P0", 4096); }
	std::string getTdcData() { return sendMessageWithReply("P1", 4096); }
	std::string getuCData() { return sendMessageWithReply("P2", 4096); }
	std::string getNetworkInfo() { return sendMessageWithReply("set", 4096); }

	//    		void 											reset_run (
	//    ); static  int  											rpc_MT_bor
	//    (int runNumber	);
	//            int  											connectToTriggerServer (
	//            );
	//            int disconnectFromTriggerServer
	//            (
	//            ); int	 											sendMessage
	//            (std::string msg); std::chrono::high_resolution_clock::time_point
	//            getTime                     (               );

  private:
	bool                       runStarted_ = false;
	std::vector<std::string>   linkStatus_;
	std::string                host_;
	unsigned int               port_;
	std::unique_ptr<TCPSocket> theTCPSocket_;

	unsigned int tdcReadoutBusyMask_;  // 0x1
	unsigned int tdcReadoutDoneMask_;  // 0x2
	bool         readoutDoneBit_;
	int          readoutBusyCount_;
	bool         waitingForResponse_;

	std::vector<TDCHeaderStruct> vectorOfTDCHeaders_;
	std::vector<TDCEvent>        vectorOfTDCEvents_;
	// Constants
	unsigned int NUMBER_OF_TDCs;

	// std::vector<Controller*>	controllers_			;

	// TCPSocket* 					TCPSocket_				;
  protected:
	// FrontEndHardwareTemplate* theFrontEndHardware_;
	// FrontEndFirmwareTemplate* theFrontEndFirmware_;
};
}

#endif
