#ifndef _ots_FECommanderInterface_h_
#define _ots_FECommanderInterface_h_

#include "otsdaq-core/FECore/FEVInterface.h"
#include "otsdaq-core/NetworkUtilities/TransceiverSocket.h"

namespace ots
{
class FECommanderInterface : public FEVInterface, public TransceiverSocket
{
  public:
	FECommanderInterface(const std::string&       interfaceUID,
	                     const ConfigurationTree& theXDAQContextConfigTree,
	                     const std::string&       interfaceConfigurationPath);
	virtual ~FECommanderInterface(void);

	void configure(void);
	void halt(void);
	void pause(void);
	void resume(void);
	void start(std::string runNumber = "");
	void stop(void);
	//	bool running          (void);

	void universalRead(char* address, char* returnValue) override { ; }
	void universalWrite(char* address, char* writeValue) override { ; }

  private:
	void   send(std::string buffer) throw(std::runtime_error);
	Socket interfaceSocket_;
};
}

#endif
