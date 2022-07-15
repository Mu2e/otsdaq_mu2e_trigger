#ifndef _ots_Event_h_
#define _ots_Event_h_

#include <TObject.h>
#include <stdint.h>
#include <vector>

namespace ots
{
class Event : public TObject
{
  public:
	Event(void);
	Event(int64_t bcoNumber);
	virtual ~Event(void);

	// Getters
	int32_t                      getTriggerNumber(void);
	int64_t                      getBCONumber(void);
	uint32_t                     getRawHit(unsigned int position);
	unsigned int                 getNumberOfRawHits(void);
	const std::vector<uint32_t>& getRawHits(void);

	// Setters
	void setTriggerNumber(int32_t triggerNumber);
	void setBCONumber(int64_t bcoNumber);
	void setRawHit(uint32_t rawHit);
	void setRawHits(const std::vector<uint32_t>& rawHits);

  private:
	int32_t triggerNumber_;  //-1 means untriggered event
	int64_t bcoNumber_;      //-1 means no bco information

	std::vector<uint32_t> rawHits_;

	ClassDef(Event, 1);
};
}

#endif
