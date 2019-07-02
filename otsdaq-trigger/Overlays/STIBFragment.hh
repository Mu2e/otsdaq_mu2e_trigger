#ifndef otsdaq_trigger_Overlays_STIBFragment_hh
#define otsdaq_trigger_Overlays_STIBFragment_hh

#include "artdaq-core/Data/Fragment.hh"
#include "cetlib_except/exception.h"

#include <ostream>
#include <vector>

// Implementation of "STIBFragment", an artdaq::Fragment overlay class

namespace ots
{
class STIBFragment;
}

class ots::STIBFragment
{
  public:
	// The "Metadata" struct is used to store info primarily related to
	// the upstream hardware environment from where the fragment came

	// "data_t" is a typedef of the fundamental unit of data the
	// metadata structure thinks of itself as consisting of; it can give
	// its size via the static "size_words" variable (
	// STIBFragment::Metadata::size_words )

	struct Metadata
	{
		typedef uint32_t data_t;

		data_t address;
		data_t port;
		data_t trigger_count;

		static size_t const size_words = 3;  // Units of Metadata::data_t
	};

	static_assert(sizeof(Metadata) == Metadata::size_words * sizeof(Metadata::data_t),
	              "STIBFragment::Metadata size changed");

	struct EventHeader
	{
		typedef uint32_t data_t;

		data_t event_data_count;

		data_t bunch_counter;

		data_t trigger_counter;

		data_t system_status : 4;
		data_t fastBCO : 8;
		data_t unused : 20;

		static size_t const size_words = 4;  // Units of Header::data_t
	};
	static_assert(sizeof(EventHeader) ==
	                  EventHeader::size_words * sizeof(EventHeader::data_t),
	              "STIBFragment::EventHeader size changed");

	struct EventData
	{
		uint32_t marker : 1;
		uint32_t adc : 3;
		uint32_t bco : 8;
		uint32_t set : 5;
		uint32_t strip : 4;
		uint32_t unused : 3;
		uint32_t chip_id : 3;
		uint32_t channel : 3;
		uint32_t stib_id : 2;
	};
	static_assert(sizeof(EventData) <= sizeof(uint32_t),
	              "STIBFragment::EventData too large!");
	static_assert(sizeof(EventData) >= sizeof(uint32_t),
	              "STIBFragment::EventData too small!");

	// The constructor simply sets its const private member "artdaq_Fragment_"
	// to refer to the artdaq::Fragment object

	STIBFragment(artdaq::Fragment const& f) : artdaq_Fragment_(f) {}

	// Start of the STIB data, returned as a pointer
	EventHeader const* dataBegin() const
	{
		return reinterpret_cast<EventHeader const*>(artdaq_Fragment_.dataBeginBytes());
	}

	// End of the STIB data, returned as a pointer
	EventHeader const* dataEnd() const
	{
		return reinterpret_cast<EventHeader const*>(artdaq_Fragment_.dataBeginBytes() +
		                                            artdaq_Fragment_.dataSizeBytes());
	}

	EventHeader const* nextEvent(EventHeader const* prev) const
	{
		if(prev == nullptr || prev == dataEnd())
			return nullptr;
		return reinterpret_cast<EventHeader const*>(
		    reinterpret_cast<uint32_t const*>(prev) + prev->event_data_count);
	}

	EventData const* eventData(EventHeader const* hdr) const
	{
		return reinterpret_cast<EventData const*>(hdr + 1);
	}

	uint32_t const* eventDataUint(EventHeader const* hdr) const
	{
		return reinterpret_cast<uint32_t const*>(hdr + 1);
	}

  private:
	artdaq::Fragment const& artdaq_Fragment_;
};

#endif /* otsdaq_trigger_Overlays_STIBFragment_hh */
