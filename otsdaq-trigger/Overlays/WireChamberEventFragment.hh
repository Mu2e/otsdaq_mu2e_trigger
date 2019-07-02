#ifndef OTSDAQ_FERMILABTESTBEAM_OVERLAYS_WIRECHAMBEREVENTFRAGMENT_HH
#define OTSDAQ_FERMILABTESTBEAM_OVERLAYS_WIRECHAMBEREVENTFRAGMENT_HH 1

#include <vector>
#include "artdaq-core/Data/Fragment.hh"
#include "otsdaq-trigger/Overlays/WireChamberFragment.hh"

namespace ots
{
class WireChamberEventFragment
{
  public:
	struct WireChamberEventMetadata
	{
		typedef uint16_t                      tdcDataWord;
		char                                  WireChamberHostname[256];
		WireChamberFragment::ControllerHeader SpillHeader;
	};

	typedef WireChamberFragment::TDCEvent    TDCEvent;
	typedef WireChamberFragment::TDCDataWord TDCDataWord;

	explicit WireChamberEventFragment(artdaq::Fragment const& f) : artdaq_fragment_(f) {}

	static WireChamberEventMetadata generateMetadata(
	    std::string hostname, WireChamberFragment::ControllerHeader header)
	{
		WireChamberEventMetadata output;

		output.SpillHeader = header;
		snprintf(output.WireChamberHostname, 256, hostname.c_str());

		return output;
	}

	TDCEvent const* dataBegin() const
	{
		return reinterpret_cast<TDCEvent const*>(artdaq_fragment_.dataBegin());
	}

	bool validateEvent(TDCEvent const* evt) const
	{
		if(evt == nullptr)
			return true;

		bool validEvent = true;
		if(evt->controllerEventTimestampPad != 0 || evt->eventStatusPad != 0 ||
		   evt->tdcNumberPad != 0 || evt->triggerTypePad != 0 || evt->wordCountPad != 0)
		{
			validEvent = false;
		}
		if(evt->WordCount < sizeof(TDCEvent) / 2)
		{
			validEvent = false;
		}

		return validEvent;
	}

	TDCEvent const* getValidEvent(TDCEvent const* evt) const
	{
		auto end     = reinterpret_cast<uint16_t const*>(artdaq_fragment_.dataEndBytes());
		auto wordPtr = reinterpret_cast<uint16_t const*>(evt);
		while(!validateEvent(evt) && wordPtr < end)
		{
			wordPtr += 1;
			evt = reinterpret_cast<TDCEvent const*>(wordPtr);
		}
		return evt;
	}

	TDCEvent const* nextEvent(TDCEvent const* evt) const
	{
		auto end  = reinterpret_cast<uint16_t const*>(artdaq_fragment_.dataEndBytes());
		auto next = reinterpret_cast<uint16_t const*>(evt) + evt->WordCount;
		if(next >= end)
			return nullptr;
		return getValidEvent(reinterpret_cast<TDCEvent const*>(next));
	}

	TDCDataWord const* eventData(TDCEvent const* evt, size_t& dataSize) const
	{
		auto data          = reinterpret_cast<TDCDataWord const*>(evt + 1);
		auto dataWordCount = evt->WordCount - (sizeof(TDCEvent) / 2);
		dataSize           = dataWordCount;
		return data;
	}

	WireChamberEventMetadata const* metadata() const
	{
		if(!artdaq_fragment_.hasMetadata())
		{
			auto md =
			    generateMetadata("UNKNOWN MWPC", WireChamberFragment::ControllerHeader());
			const_cast<artdaq::Fragment&>(artdaq_fragment_)
			    .setMetadata<WireChamberEventMetadata>(md);
		}
		return artdaq_fragment_.metadata<WireChamberEventMetadata>();
	}

	size_t trigger_number() const { return artdaq_fragment_.sequenceID(); }

	size_t sizeBytes() const { return artdaq_fragment_.sizeBytes(); }

  private:
	artdaq::Fragment const& artdaq_fragment_;
};
}

#endif  // OTSDAQ_FERMILABTESTBEAM_OVERLAYS_WIRECHAMBERFRAGMENT_HH
