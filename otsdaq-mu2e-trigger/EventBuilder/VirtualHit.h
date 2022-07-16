#ifndef _ots_VirtualHit_h
#define _ots_VirtualHit_h

namespace ots
{
class VirtualHit : public TObject
{
  public:
	VirtualHit(void);
	virtual ~VirtualHit(void);

  private:
	unsigned char type_;
	unsigned char row_;
	unsigned char col_;
	unsigned char adc_;

	unsigned char chipId_;
	unsigned char channelId_;
	unsigned char fedId_;
};
}

#endif
