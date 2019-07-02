#ifndef _ots_FEPixelTelescopeInterface_h_
#define _ots_FEPixelTelescopeInterface_h_

#include "otsdaq-core/FECore/FEVInterface.h"
#include "otsdaq-core/NetworkUtilities/TransceiverSocket.h"

namespace ots
{
class FEPixelTelescopeInterface : public FEVInterface, public TransceiverSocket
{
  public:
	FEPixelTelescopeInterface(const std::string&       interfaceUID,
	                          const ConfigurationTree& theXDAQContextConfigTree,
	                          const std::string&       interfaceConfigurationPath);
	virtual ~FEPixelTelescopeInterface(void);

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
