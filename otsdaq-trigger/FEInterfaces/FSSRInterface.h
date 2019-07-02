#ifndef _ots_FEOtsUDPFSSRInterface_h_
#define _ots_FEOtsUDPFSSRInterface_h_

#include "otsdaq-core/FECore/FEVInterface.h"

#include <string>

namespace ots
{
class FrontEndHardwareBase;
class FSSRFirmwareBase;

class FSSRInterface : public FEVInterface
{
  public:
	FSSRInterface(const std::string&       interfaceUID,
	              const ConfigurationTree& theXDAQContextConfigTree,
	              const std::string&       interfaceConfigurationPath);
	virtual ~FSSRInterface(void);

	void configure(void);
	void halt(void);
	void pause(void);
	void resume(void);
	void start(std::string runNumber) override;
	void stop(void);

	bool running(void);

	void universalRead(char* address, char* readValue) override;
	void universalWrite(char* address, char* writeValue) override;

	// void configureFEW     		(void);
	void configureDetector(void);

  private:
	// void destroy          		(void);

	// use firmware to describe the data payload and hardware to send/receive it
	FSSRFirmwareBase*     FSSRFirmware_;
	FrontEndHardwareBase* FSSRHardware_;
	std::string           hardwareType_;
	std::string           firmwareType_;
	unsigned              firmwareVersion_;
};
}

#endif
