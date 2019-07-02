#ifndef _ots_BCOData_h
#define _ots_BCOData_h

#include <stdint.h>

namespace ots
{
class BCOData
{
  public:
	BCOData(void);
	virtual ~BCOData(void);

	bool     isBCOHigh(uint32_t data);
	bool     isBCOLow(uint32_t data);
	uint32_t decodeBCOHigh(uint32_t data);
	uint32_t decodeBCOLow(uint32_t data);
	uint64_t mergeBCOHighAndLow(uint32_t bcoHigh, uint32_t bcoLow);
	void     insertBCOHigh(uint64_t& bco, uint32_t dataBCOHigh);
	void     insertBCOLow(uint64_t& bco, uint32_t dataBCOLow);
};
}

#endif
