#ifndef mu2e_artdaq_core_Overlays_Offline_OfflineFragmentReader_hh
#define mu2e_artdaq_core_Overlays_Offline_OfflineFragmentReader_hh

#include "art/Framework/Core/FileBlock.h"
#include "art/Framework/Core/ProductRegistryHelper.h"
#include "art/Framework/IO/Sources/SourceHelper.h"
#include "art/Framework/IO/Sources/SourceTraits.h"
#include "art/Framework/Principal/EventPrincipal.h"
#include "art/Framework/Principal/RunPrincipal.h"
#include "art/Framework/Principal/SubRunPrincipal.h"
#include "artdaq-core/Core/SharedMemoryEventReceiver.hh"
#include "artdaq-core/Data/RawEvent.hh"
#include "artdaq-core/Utilities/TimeUtils.hh"
#include "fhiclcpp/types/Atom.h"
#include "mu2e-artdaq-core/Overlays/mu2eFragment.hh"
#include "otsdaq-trigger/ArtModules/detail/CurrentFragment.hh"
#include "otsdaq-trigger/ArtModules/detail/EventIDHandler.hh"

#include <set>
#include <string>

namespace mu2e {

class OfflineFragmentReader
{
public:
	OfflineFragmentReader(OfflineFragmentReader const&) = delete;
	OfflineFragmentReader& operator=(OfflineFragmentReader const&) = delete;
#if 0
	struct Config {
	  fhicl::Atom<std::string> module_type { fhicl::Name("module_type") };
	  fhicl::Atom<int64_t> maxSubRuns { fhicl::Name("maxSubRuns"), -1 };
	  fhicl::Atom<int64_t> maxEvents { fhicl::Name("maxEvents"), -1 };
	  fhicl::Atom<double> waiting_time { fhicl::Name("waiting_time"),
		  fhicl::Comment("The next parameter is the allowed wait time (in seconds) for fetching\n"
						 "an artdaq event off of the queue. If the wait time is reached, then the\n"
						 "behavior is dictated by the value of the 'resume_after_timeout' parameter."), 86400. };
	  fhicl::Atom<bool> resume_after_timeout { fhicl::Name("resume_after_timeout"), true };
	  struct KeysToIgnore {
		std::set<std::string> operator()() { return {"module_label"}; }
	  };
	};

	using Parameters = art::ConfigTable<Config, Config::KeysToIgnore>;
#endif
	explicit OfflineFragmentReader(fhicl::ParameterSet const& ps, art::ProductRegistryHelper& help,
								   art::SourceHelper const& pm);

	virtual ~OfflineFragmentReader();
	void closeCurrentFile() {}
	void readFile(std::string const& name, art::FileBlock*& fb);

	bool hasMoreData() const { return !shutdownMsgReceived_; }

	bool readNext(art::RunPrincipal* const& inR, art::SubRunPrincipal* const& inSR, art::RunPrincipal*& outR,
				  art::SubRunPrincipal*& outSR, art::EventPrincipal*& outE);

private:
	art::SourceHelper const& pMaker_;
	double waitingTime_;
	std::unique_ptr<artdaq::SharedMemoryEventReceiver> incoming_events;  ///< The events from the EventStore
	bool resumeAfterTimeout_;
	bool shutdownMsgReceived_{false};
	bool outputFileCloseNeeded_{false};
	detail::EventIDHandler idHandler_{};
	detail::CurrentFragment currentFragment_{};

	artdaq::detail::RawEventHeader evtHeader_;
};

}  // namespace mu2e

// Specialize an art source trait to tell art that we don't care about
// source.fileNames and don't want the files services to be used.
namespace art {
template<>
struct Source_generator<mu2e::OfflineFragmentReader> : std::true_type
{
};
}  // namespace art

#endif /* mu2e_artdaq_core_Overlays_Offline_OfflineFragmentReader_hh */
