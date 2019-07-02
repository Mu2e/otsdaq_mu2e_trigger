#ifndef _ots_FEPurdueNetworkMasterInterface_h_
#define _ots_FEPurdueNetworkMasterInterface_h_

#include "otsdaq-core/FECore/FEVInterface.h"
//#include "otsdaq-components/DAQHardware/OtsUDPHardware.h"
//#include "otsdaq-components/DAQHardware/FSSRFirmware.h"

namespace ots
{
class FSSRFirmwareBase;
class FrontEndHardwareBase;

class FEPurdueNetworkMasterInterface : public FEVInterface
{
  public:
	FEPurdueNetworkMasterInterface(const std::string&       interfaceUID,
	                               const ConfigurationTree& theXDAQContextConfigTree,
	                               const std::string&       interfaceConfigurationPath);
	virtual ~FEPurdueNetworkMasterInterface(void);
	// void destroy          (void);
	// void configureFEW     (void);
	// void configureDetector(const DACStream& theDACStream);
	void configure(void);
	void halt(void);
	void pause(void);
	void resume(void);
	void start(std::string runNumber);
	void stop(void);
	bool running(void);

	void universalRead(char* address, char* returnValue) override { ; }
	void universalWrite(char* address, char* writeValue) override { ; }

  private:
	FSSRFirmwareBase*     FSSRFirmware_;
	FrontEndHardwareBase* FSSRHardware_;
};
}

#endif
