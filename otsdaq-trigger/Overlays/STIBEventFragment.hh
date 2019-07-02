#ifndef otsdaq_trigger_Overlays_STIBEventFragment_hh
#define otsdaq_trigger_Overlays_STIBEventFragment_hh

#include "artdaq-core/Data/Fragment.hh"
#include "cetlib/exception.h"

#include <ostream>
#include <vector>

// Implementation of "STIBEventFragment", an artdaq::Fragment overlay class

namespace ots
{
class STIBEventFragment;
}

class ots::STIBEventFragment
{
  public:
	// The "Metadata" struct is used to store info primarily related to
	// the upstream hardware environment from where the fragment came

	// "data_t" is a typedef of the fundamental unit of data the
	// metadata structure thinks of itself as consisting of; it can give
	// its size via the static "size_words" variable (
	// STIBEventFragment::Metadata::size_words )

	struct Metadata
	{
		typedef uint32_t data_t;

		data_t address;
		data_t port;

		static size_t const size_words = 2;  // Units of Metadata::data_t
	};

	static_assert(sizeof(Metadata) == Metadata::size_words * sizeof(Metadata::data_t),
	              "STIBEventFragment::Metadata size changed");

	// The "Header" struct contains "metadata" specific to the fragment
	// which is not hardware-related

	// Header::data_t -- not to be confused with Metadata::data_t ! --
	// describes the standard size of a data type not just for the
	// header data, but ALSO the physics data beyond it; the size of the
	// header in units of Header::data_t is given by "size_words", and
	// the size of the fragment beyond the header in units of
	// Header::data_t is given by "event_size"

	// Notice only the first 28 bits of the first 32-bit unsigned
	// integer in the Header is used to hold the event_size ; this means
	// that you can't represent a fragment larger than 2**28 units of
	// data_t, or 1,073,741,824 bytes

	struct Header
	{
		typedef uint32_t data_t;

		typedef uint32_t event_size_t;
		typedef uint32_t counter_t;

		event_size_t event_size : 28;
		event_size_t unused : 4;

		counter_t bunch_counter;

		counter_t trigger_counter : 24;
		counter_t system_status : 4;
		counter_t unused3 : 4;

		static size_t const size_words = 3;  // Units of Header::data_t
	};
	static_assert(sizeof(Header) == Header::size_words * sizeof(Header::data_t),
	              "STIBEventFragment::Header size changed");

	struct Data
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
	static_assert(sizeof(Data) <= sizeof(uint32_t), "STIBEventFragment::Data too large!");
	static_assert(sizeof(Data) >= sizeof(uint32_t), "STIBEventFragment::Data too small!");

	// The constructor simply sets its const private member "artdaq_Fragment_"
	// to refer to the artdaq::Fragment object

	STIBEventFragment(artdaq::Fragment const& f) : artdaq_Fragment_(f) {}

	// const getter functions for the data in the header

	Header::event_size_t    hdr_event_size() const { return header_()->event_size; }
	static constexpr size_t hdr_size_words() { return Header::size_words; }

	Header::counter_t hdr_bunch_counter() const { return header_()->bunch_counter; }
	Header::counter_t hdr_trigger_counter() const { return header_()->trigger_counter; }
	Header::counter_t hdr_system_status() const { return header_()->system_status; }
	bool hdr_system_status_fifo_full() const { return header_()->system_status & 0x1; }
	bool hdr_system_status_clock_lock() const { return header_()->system_status & 0x2; }
	bool hdr_system_status_other_clock_lock() const
	{
		return header_()->system_status & 0x4;
	}

	// STIB Data Word Count
	size_t stib_data_words() const { return hdr_event_size() - hdr_size_words(); }

	// Start of the STIB data, returned as a pointer
	uint32_t const* dataBegin() const
	{
		return reinterpret_cast<uint32_t const*>(header_() + 1);
	}

	// End of the STIB data, returned as a pointer
	uint32_t const* dataEnd() const { return dataBegin() + stib_data_words(); }

  protected:
	// Functions to translate between byte size and the size of
	// this fragment overlay's concept of a unit of data (i.e.,
	// Header::data_t).

	static constexpr size_t bytes_per_word_()
	{
		return sizeof(Header::data_t) / sizeof(uint8_t);
	}

	// header_() simply takes the address of the start of this overlay's
	// data (i.e., where the STIBEventFragment::Header object begins) and
	// casts it as a pointer to STIBEventFragment::Header

	Header const* header_() const
	{
		return reinterpret_cast<STIBEventFragment::Header const*>(
		    artdaq_Fragment_.dataBeginBytes());
	}

  private:
	artdaq::Fragment const& artdaq_Fragment_;
};

#endif /* otsdaq_trigger_Overlays_STIBEventFragment_hh */
