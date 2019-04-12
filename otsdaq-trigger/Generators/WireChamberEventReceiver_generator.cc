#include "otsdaq-trigger/Generators/WireChamberEventReceiver.hh"

//#include "art/Utilities/Exception.h"
#include "artdaq/Application/GeneratorMacros.hh"
#include "cetlib_except/exception.h"
#include "fhiclcpp/ParameterSet.h"
#include "otsdaq-trigger/Overlays/FragmentType.hh"
#include "otsdaq-trigger/Overlays/WireChamberEventFragment.hh"

#include <sys/poll.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>

//========================================================================================================================
ots::WireChamberEventReceiver::WireChamberEventReceiver(fhicl::ParameterSet const& ps)
    : WorkLoop("WireChamberEventReceiver")
    , DataConsumer(ps.get<std::string>("SupervisorApplicationUID", "ARTDAQDataManager"),
                   ps.get<std::string>("BufferUID", "ARTDAQBuffer"),
                   ps.get<std::string>("ProcessorUID", "WireChamberEventReceiver"),
                   HighConsumerPriority)
    , CommandableFragmentGenerator(ps)
    , rawOutput_(ps.get<bool>("raw_output_enabled", true))
    , rawPath_(ps.get<std::string>("raw_output_path", "/data/otsdaq/OutputData"))
    , dataport_(ps.get<int>("port", 6343))
    , ip_(ps.get<std::string>("ip", "127.0.0.1"))
    , mwpc_hostname_(ps.get<std::string>("wire_chamber_hostname", "UNKNOWN MWPC"))
    , expectedPacketNumber_(0)
    , expectedDataSize_(0)
    , sendCommands_(ps.get<bool>("send_OtsUDP_commands", false))
    , sendEndOfSubrunFrag_(ps.get<bool>("send_endofsubrun_frag", false))
    , fragmentWindow_(ps.get<double>("fragment_time_window_ms", 1000))
    , lastFrag_(std::chrono::high_resolution_clock::now())
{
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << std::endl;
	TLOG(TLVL_INFO, "WireChamberEventReceiver")
	    << "MY TRIGGER MODE IS: " << ps.get<std::string>("trigger_mode", "UNDEFINED!!!");
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "registering to buffer!"
	          << std::endl;
	registerToBuffer();
}

//========================================================================================================================
bool ots::WireChamberEventReceiver::getNext_(artdaq::FragmentPtrs& output)
{
	// mf::LogDebug("WireChamberEventReceiver") << "READING DATA!" << std::endl;
	if(should_stop())
		return false;

	// unsigned long block;
	std::string tmpBuffer;
	if(read<std::string, std::map<std::string, std::string>>(tmpBuffer) < 0)
		usleep(10000);
	else
	{
		if(tmpBuffer != "NODATA")
		{
			buffer_.append(tmpBuffer.c_str(), tmpBuffer.length());

			// std::cout << __COUT_HDR_FL__ << "SAVING FILE??? " << rawOutput_ <<
			// std::endl;
			if(rawOutput_)
			{
				std::ofstream outputStream;
				std::string outputPath = rawPath_ + "/WireChamberEventReceiver_recv.bin";
				// std::cout << __COUT_HDR_FL__ << "FILE DATA PATH: " << outputPath <<
				// std::endl;
				outputStream.open(outputPath, std::ios::out | std::ios::binary);
				outputStream.write(tmpBuffer.c_str(), tmpBuffer.length());
			}

			//			if (expectedDataSize_ == 0)
			//{
			// Look for 'MWPCDATA' tag
			auto tagPos = buffer_.find("MWPCDATA");
			if(tagPos != std::string::npos && tagPos <= buffer_.size() - 12)
			{
				mf::LogDebug("WireChamberEventReceiver")
				    << "Found MWPCDATA Tag at " << tagPos;
				buffer_.erase(0, buffer_.find("MWPCDATA") + 8);

				// Total word count is first two words of data...
				auto dataPtr      = reinterpret_cast<uint16_t const*>(buffer_.c_str());
				expectedDataSize_ = 2 * ((*dataPtr << 16) + (*(dataPtr + 1)));
				TLOG(TLVL_DEBUG, "WireChamberEventReceiver")
				    << "Expected data size is now " << std::to_string(expectedDataSize_);
			}
			//}
			if(expectedDataSize_ > 0)  // else
			{
				TLOG(TLVL_DEBUG, "WireChamberEventReceiver")
				    << "Buffer received! sz=" << std::to_string(buffer_.size())
				    << ", expected=" << std::to_string(expectedDataSize_);
				if(buffer_.length() >= expectedDataSize_)
				{
					TLOG(TLVL_DEBUG, "WireChamberEventReceiver")
					    << "Received whole buffer, proccessing data!";
					ProcessSpillData_(output);
					expectedDataSize_ = 0;
					TLOG(TLVL_DEBUG, "WireChamberEventReceiver")
					    << "Done processing data, buffer size is now "
					    << std::to_string(buffer_.size())
					    << ", expectedDataSize=" << std::to_string(expectedDataSize_);
				}
			}
		}
		else
		{
			TLOG(TLVL_DEBUG, "WireChamberEventReceiver")
			    << "Received \"NODATA\" from buffer";
		}
	}
	return true;
}

//========================================================================================================================
void ots::WireChamberEventReceiver::ProcessSpillData_(artdaq::FragmentPtrs& frags)
{
	ots::WireChamberFragment::WireChamberMetadata metadata;

	std::ofstream output;
	// std::cout << __COUT_HDR_FL__ << "SAVING FILE??? " << rawOutput_ << std::endl;
	if(rawOutput_)
	{
		std::string outputPath = rawPath_ + "/WireChamberEventReceiver_process.bin";
		// std::cout << __COUT_HDR_FL__ << "FILE DATA PATH: " << outputPath << std::endl;
		output.open(outputPath, std::ios::out | std::ios::binary);
		output.write(buffer_.c_str(), expectedDataSize_);
	}

	auto wcfrag = artdaq::Fragment::FragmentBytes(expectedDataSize_,
	                                              ev_counter(),
	                                              fragment_id(),
	                                              ots::detail::FragmentType::MWPC,
	                                              metadata);
	ev_counter_inc();

	memcpy(wcfrag->dataBegin(), &buffer_[0], expectedDataSize_);

	TLOG(TLVL_DEBUG, "WireChamberEventReceiver") << "Constructing WireChamberFragment";
	WireChamberFragment ff(*wcfrag);
	TLOG(TLVL_DEBUG, "WireChamberEventReceiver") << "Generating metadata";
	wcfrag->updateMetadata<WireChamberFragment::WireChamberMetadata>(
	    ff.generateMetadata(mwpc_hostname_));
	TLOG(TLVL_DEBUG, "WireChamberEventReceiver") << "Setting timestamp";
	auto ts = (static_cast<uint64_t>(ff.GetControllerHeader()->YearMonth) << 32) +
	          (ff.GetControllerHeader()->DaysHours << 16) +
	          ff.GetControllerHeader()->MinutesSeconds;
	wcfrag->setTimestamp(ts);
	TLOG(TLVL_DEBUG, "WireChamberEventReceiver") << "Clearing buffer";
	// buffer_ = "";

	TLOG(TLVL_DEBUG, "WireChamberEventReceiver")
	    << "Processing "
	    << std::to_string(ff.ControllerTriggerCount(ff.GetControllerHeader()))
	    << " triggers...";
	ProcessTriggerData_(frags, ff);

	buffer_.erase(0, expectedDataSize_);

	// End the SubRun (Spill)
	if(sendEndOfSubrunFrag_)
	{
		std::unique_ptr<artdaq::Fragment> endOfSubrunFrag(
		    new artdaq::Fragment(static_cast<size_t>(
		        ceil(sizeof(my_rank) /
		             static_cast<double>(sizeof(artdaq::Fragment::value_type))))));

		endOfSubrunFrag->setSystemType(artdaq::Fragment::EndOfSubrunFragmentType);
		*endOfSubrunFrag->dataBegin() = my_rank;
		frags.push_back(std::move(endOfSubrunFrag));
	}

	TLOG(TLVL_DEBUG, "WireChamberEventReceiver") << "Done with spill";
	return;
}

void ots::WireChamberEventReceiver::ProcessTriggerData_(
    artdaq::FragmentPtrs& frags, WireChamberFragment& spillData)
{
	// Increment through the data blocks of the current super block.
	ots::WireChamberFragment::TDCEvent const* tdcEvent = spillData.dataBegin();
	while(tdcEvent != nullptr)
	{
		auto beginCounter = spillData.TDCEventTriggerCounter(tdcEvent);
		auto begin        = tdcEvent;

		TLOG(TLVL_DEBUG, "WireChamberEventReceiver")
		    << "Looking for end of event " << std::to_string(beginCounter) << " in data";
		size_t byteCount = tdcEvent->WordCount * 2;
		bool   endFound  = false;
		while(!endFound)
		{
			tdcEvent = spillData.nextEvent(tdcEvent);
			if(!tdcEvent)
				break;
			if(spillData.TDCEventTriggerCounter(tdcEvent) != beginCounter)
			{
				endFound = true;
			}
			else
			{
				byteCount += tdcEvent->WordCount * 2;
			}
		}

		// Use byte count to calculate how many words the current data
		// block should occupy in the new fragment.
		auto const wordCount  = byteCount / sizeof(artdaq::RawDataType);
		auto const packetSize = (byteCount % 8 == 0) ? wordCount : wordCount + 1;

		TLOG(TLVL_DEBUG, "WireChamberEventReceiver")
		    << "Creating event fragment with wordCount=" << std::to_string(wordCount);

		frags.push_back(artdaq::Fragment::dataFrag(
		    beginCounter,
		    1,
		    reinterpret_cast<artdaq::RawDataType const*>(begin),
		    packetSize,
		    spillData.TDCEventTimestamp(begin)));

		frags.back()->setUserType(ots::FragmentType::MWPCEVT);
		frags.back()
		    ->setMetadata<ots::WireChamberEventFragment::WireChamberEventMetadata>(
		        WireChamberEventFragment::generateMetadata(
		            spillData.metadata()->WireChamberHostname,
		            *spillData.GetControllerHeader()));
	}
}

//========================================================================================================================
void ots::WireChamberEventReceiver::start() {}

//========================================================================================================================
void ots::WireChamberEventReceiver::stop() {}

//========================================================================================================================
void ots::WireChamberEventReceiver::stopNoMutex() {}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(ots::WireChamberEventReceiver)
