#ifndef _ots_FEPSI46Interface_h_
#define _ots_FEPSI46Interface_h_

#include <bitset>
#include "otsdaq-components/FEInterfaces/FEOtsUDPTemplateInterface.h"

namespace ots
{
class FEInterfaceTableBase;

class FEPSI46Interface : public FEOtsUDPTemplateInterface
{
  public:
	FEPSI46Interface(const std::string&       interfaceUID,
	                 const ConfigurationTree& theXDAQContextConfigree,
	                 const std::string&       interfaceConfigurationPath);
	virtual ~FEPSI46Interface(void);

	void configure(void) override;
	void halt(void) override;
	void pause(void) override;
	void resume(void) override;
	void start(std::string runNumber) override;
	bool running(void) override;
	void stop(void) override;

  private:
	std::string runNumber_;
};
}

#endif
