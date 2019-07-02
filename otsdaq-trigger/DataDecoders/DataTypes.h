#ifndef _ots_DataTypes_h
#define _ots_DataTypes_h

#include <stdint.h>

namespace ots
{
class DataTypes
{
  public:
	DataTypes(void);
	virtual ~DataTypes(void);
	bool isBCOHigh(uint32_t data);
	bool isBCOLow(uint32_t data);
	bool isTrigger(uint32_t data);
	bool isFSSRData(uint32_t data);
	bool isVIPICData(uint32_t data);
	bool isPSI46Data(uint32_t data);
	bool isPSI46DigData(uint32_t data);
};
}

#endif
