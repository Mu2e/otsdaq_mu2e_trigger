#ifndef _ots_TriggerData_h
#define _ots_TriggerData_h

#include <stdint.h>

namespace ots
{
class TriggerData
{
  public:
	TriggerData(void);
	virtual ~TriggerData(void);

	bool     isTrigger(uint32_t data);
	uint32_t decodeTrigger(uint32_t data);
};
}

#endif
