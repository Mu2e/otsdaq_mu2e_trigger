#ifndef _ots_FECAMACInterface_h_
#define _ots_FECAMACInterface_h_

#include "otsdaq-core/FECore/FEVInterface.h"
#include "otsdaq-core/NetworkUtilities/UDPDataStreamerBase.h"

class UsbHandler;
#include <fstream>
#include <iostream>
#include <string>

namespace ots
{
class FECAMACInterface : public ots::FEVInterface, public ots::UDPDataStreamerBase
{
  public:
	FECAMACInterface(const std::string&            interfaceUID,
	                 const ots::ConfigurationTree& theXDAQContextConfigTree,
	                 const std::string&            interfaceConfigurationPath);
	virtual ~FECAMACInterface(void);

	void configure(void);
	void halt(void);
	void pause(void);
	void resume(void);
	void start(std::string runNumber = "");
	void stop(void);
	bool running(void);

	void universalRead(char* address, char* returnValue) override { ; }
	void universalWrite(char* address, char* writeValue) override { ; }

  private:
	std::unique_ptr<UsbHandler> camac;
	std::ofstream               output;
	std::string                 rawOutputFile;
	std::string                 cfgHeader;
	std::string                 cardList;
	bool                        rawOutput;
	bool                        init_sent_;
};
}

#endif
