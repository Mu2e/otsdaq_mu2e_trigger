#ifndef OTSDAQ_FERMILABTESTBEAM_Overlays_FragmentType_hh
#define OTSDAQ_FERMILABTESTBEAM_Overlays_FragmentType_hh
#include "artdaq-core/Data/Fragment.hh"

namespace ots
{
std::vector<std::string> const names{
    "MISSED", "UDP", "STIB", "MWPC", "MWPCEVT", "STIBEVT", "CAMAC", "UNKNOWN"};

namespace detail
{
enum FragmentType : artdaq::Fragment::type_t
{
	MISSED = artdaq::Fragment::FirstUserFragmentType,
	UDP,
	STIB,
	MWPC,
	MWPCEVT,
	STIBEVT,
	CAMAC,
	INVALID  // Should always be last.
};

// Safety check.
static_assert(artdaq::Fragment::isUserFragmentType(FragmentType::INVALID - 1),
              "Too many user-defined fragments!");
}

using detail::FragmentType;

FragmentType toFragmentType(std::string t_string);
std::string  fragmentTypeToString(FragmentType val);
}
#endif /*  OTSDAQ_FERMILABTESTBEAM_Overlays_FragmentType_hh */
