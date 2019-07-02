#ifndef _ots_VIPICData_h
#define _ots_VIPICData_h

#include <stdint.h>
#include <string>
#include "otsdaq-trigger/DataDecoders/DetectorDataBase.h"

namespace ots
{
class VIPICData
{
  public:
	VIPICData(void);
	virtual ~VIPICData(void);

	bool       isVIPIC(uint32_t data);
	VIPICData& decode(uint32_t data);

	// Getters
	unsigned int getStibId(void);
	unsigned int getChannelNumber(void);
	unsigned int getChipId(void);
	unsigned int getStripNumber(void);
	unsigned int getBco(void);
	unsigned int getCol(void);
	unsigned int getRow(void);

  protected:
	unsigned char stibId_;
	unsigned char channelNumber_;
	unsigned char chipId_;
	unsigned char stripNumber_;
	unsigned char bco_;
	unsigned char set_;
	unsigned int  pixel_;
};
}

#endif
