#include "art/Framework/Core/InputSourceMacros.h"
#include "art/Framework/IO/Sources/Source.h"
#include "art/Framework/IO/Sources/SourceTraits.h"
#include "artdaq/ArtModules/detail/SharedMemoryReader.hh"
#include "otsdaq-trigger/Overlays/FragmentType.hh"

#include <string>
using std::string;

namespace ots
{
std::map<artdaq::Fragment::type_t, std::string> makeFragmentTypeMap()
{
	auto output = artdaq::Fragment::MakeSystemTypeMap();
	for(auto name : names)
	{
		output[toFragmentType(name)] = name;
	}
	return output;
}
}

namespace art
{
/**
 * \brief  Specialize an art source trait to tell art that we don't care about
 * source.fileNames and don't want the files services to be used.
 */
template<>
struct Source_generator<artdaq::detail::SharedMemoryReader<ots::makeFragmentTypeMap>>
{
	static constexpr bool value =
	    true;  ///< Used to suppress use of file services on art Source
};
}

namespace ots
{
/**
 * \brief DemoInput is an art::Source using the detail::RawEventQueueReader class
 */
typedef art::Source<artdaq::detail::SharedMemoryReader<ots::makeFragmentTypeMap>>
    FTBFInput;
}

DEFINE_ART_INPUT_SOURCE(ots::FTBFInput)