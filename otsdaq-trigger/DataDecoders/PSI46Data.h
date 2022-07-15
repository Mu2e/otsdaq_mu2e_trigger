#ifndef _ots_PSI46Data_h
#define _ots_PSI46Data_h

#include <stdint.h>
#include <string>
#include "otsdaq-trigger/DataDecoders/DetectorDataBase.h"

namespace ots
{
class PSI46Data
{
  public:
	PSI46Data(void);
	virtual ~PSI46Data(void);

	bool       isPSI46(uint32_t data);
	PSI46Data& decode(uint32_t data);

	// Getters
	unsigned int getStibId(void);
	unsigned int getChannelNumber(void);
	unsigned int getChipId(void);
	unsigned int getStripNumber(void);
	unsigned int getBco(void);
	unsigned int getAdc(void);
	unsigned int getSensorStrip(void);

  protected:
	unsigned char stibId_;
	unsigned char channelNumber_;
	unsigned char chipId_;
	unsigned char stripNumber_;
	unsigned char bco_;
	unsigned char adc_;
	unsigned char set_;
};
}

#endif
