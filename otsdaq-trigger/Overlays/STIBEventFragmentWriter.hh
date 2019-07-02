#ifndef otsdaq_trigger_Overlays_STIBEventFragmentWriter_hh
#define otsdaq_trigger_Overlays_STIBEventFragmentWriter_hh

////////////////////////////////////////////////////////////////////////
// STIBEventFragmentWriter
//
// Class derived from STIBEventFragment which allows writes to the data (for
// simulation purposes). Note that for this reason it contains
// non-const members which hide the const members in its parent class,
// STIBEventFragment, including its reference to the artdaq::Fragment
// object, artdaq_Fragment_, as well as its functions pointing to the
// beginning and end of ADC values in the fragment, dataBegin() and
// dataEnd()
//
////////////////////////////////////////////////////////////////////////

#include "artdaq-core/Data/Fragment.hh"
#include "otsdaq-trigger/Overlays/STIBEventFragment.hh"

#include <iostream>

namespace ots
{
class STIBEventFragmentWriter;
}

class ots::STIBEventFragmentWriter : public ots::STIBEventFragment
{
  public:
	STIBEventFragmentWriter(artdaq::Fragment& f);

	// These functions form overload sets with const functions from
	// ots::STIBEventFragment

	uint32_t* dataBegin();
	uint32_t* dataEnd();

	// We'll need to hide the const version of header in STIBEventFragment in
	// order to be able to perform writes

	Header* header_()
	{
		assert(artdaq_Fragment_.dataSizeBytes() >= sizeof(Header));
		return reinterpret_cast<Header*>(artdaq_Fragment_.dataBeginBytes());
	}

	void set_hdr_bunch_counter(Header::counter_t counter)
	{
		header_()->bunch_counter = counter;
	}

	void set_hdr_trigger_counter(Header::counter_t counter)
	{
		header_()->trigger_counter = counter;
	}

	void set_hdr_system_status(Header::counter_t counter)
	{
		header_()->system_status = counter;
	}

	void resize(size_t nBytes);

  private:
	size_t calc_event_size_words_(size_t nBytes);

	static size_t bytes_to_words_(size_t nBytes);

	// Note that this non-const reference hides the const reference in the base class
	artdaq::Fragment& artdaq_Fragment_;
};

// The constructor will expect the artdaq::Fragment object it's been
// passed to contain the artdaq::Fragment header + the
// STIBEventFragment::Metadata object, otherwise it throws

ots::STIBEventFragmentWriter::STIBEventFragmentWriter(artdaq::Fragment& f)
    : STIBEventFragment(f), artdaq_Fragment_(f)
{
	if(!f.hasMetadata() && f.dataSizeBytes() > 0)
	{
		throw cet::exception(
		    "Error in STIBEventFragmentWriter: Raw artdaq::Fragment object does not "
		    "appear to consist of its own header + the STIBEventFragment::Metadata "
		    "object");
	}

	if(f.dataSizeBytes() == 0)
	{
		// Allocate space for the header
		artdaq_Fragment_.resizeBytes(sizeof(Header));
	}
}

inline uint32_t* ots::STIBEventFragmentWriter::dataBegin()
{
	// Make sure there's data past the STIBEventFragment header
	assert(artdaq_Fragment_.dataSizeBytes() >=
	       sizeof(Header) + sizeof(artdaq::Fragment::value_type));
	return reinterpret_cast<uint32_t*>(header_() + 1);
}

inline uint32_t* ots::STIBEventFragmentWriter::dataEnd()
{
	return dataBegin() + stib_data_words();
}

inline void ots::STIBEventFragmentWriter::resize(size_t nBytes)
{
	artdaq_Fragment_.resizeBytes(sizeof(Header::data_t) * calc_event_size_words_(nBytes));
	header_()->event_size = calc_event_size_words_(nBytes);
}

inline size_t ots::STIBEventFragmentWriter::calc_event_size_words_(size_t nBytes)
{
	return bytes_to_words_(nBytes) + hdr_size_words();
}

inline size_t ots::STIBEventFragmentWriter::bytes_to_words_(size_t nBytes)
{
	auto mod(nBytes % bytes_per_word_());
	return (mod == 0) ? nBytes / bytes_per_word_() : nBytes / bytes_per_word_() + 1;
}

#endif /* otsdaq_trigger_Overlays_STIBEventFragmentWriter_hh */
