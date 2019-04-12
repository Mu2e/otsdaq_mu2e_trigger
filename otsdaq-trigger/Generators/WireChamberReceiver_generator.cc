#include "otsdaq-trigger/Generators/WireChamberReceiver.hh"

//#include "art/Utilities/Exception.h"
#include "artdaq/Application/GeneratorMacros.hh"
#include "cetlib_except/exception.h"
#include "fhiclcpp/ParameterSet.h"
#include "otsdaq-trigger/Overlays/FragmentType.hh"
#include "otsdaq-trigger/Overlays/WireChamberFragment.hh"

#include <sys/poll.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>

//========================================================================================================================
ots::WireChamberReceiver::WireChamberReceiver(fhicl::ParameterSet const& ps)
    : WorkLoop("WireChamberReceiver")
    , DataConsumer(ps.get<std::string>("SupervisorApplicationUID", "ARTDAQDataManager"),
                   ps.get<std::string>("BufferUID", "ARTDAQBuffer"),
                   ps.get<std::string>("ProcessorUID", "WireChamberReceiver"),
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
    , fragmentWindow_(ps.get<double>("fragment_time_window_ms", 1000))
    , lastFrag_(std::chrono::high_resolution_clock::now())
{
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << std::endl;
	TLOG(TLVL_INFO, "WireChamberReceiver")
	    << "MY TRIGGER MODE IS: " << ps.get<std::string>("trigger_mode", "UNDEFINED!!!");
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "registering to buffer!"
	          << std::endl;
	registerToBuffer();
}

//========================================================================================================================
bool ots::WireChamberReceiver::getNext_(artdaq::FragmentPtrs& output)
{
	// mf::LogDebug("WireChamberReceiver") << "READING DATA!" << std::endl;
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
				std::string   outputPath = rawPath_ + "/WireChamberReceiver_recv.bin";
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
				TLOG(TLVL_DEBUG, "WireChamberReceiver")
				    << "Found MWPCDATA Tag at " << tagPos;
				buffer_.erase(0, buffer_.find("MWPCDATA") + 8);

				// Total word count is first two words of data...
				auto dataPtr      = reinterpret_cast<uint16_t const*>(buffer_.c_str());
				expectedDataSize_ = 2 * ((*dataPtr << 16) + (*(dataPtr + 1)));
				TLOG(TLVL_DEBUG, "WireChamberReceiver")
				    << "Expected data size is now " << std::to_string(expectedDataSize_);
			}
			//}
			if(expectedDataSize_ > 0)  // else
			{
				TLOG(TLVL_DEBUG, "WireChamberReceiver")
				    << "Buffer received! sz=" << std::to_string(buffer_.size())
				    << ", expected=" << std::to_string(expectedDataSize_);
				if(buffer_.length() >= expectedDataSize_)
				{
					TLOG(TLVL_DEBUG, "WireChamberReceiver")
					    << "Received whole buffer, proccessing data!";
					ProcessData_(output);
					expectedDataSize_ = 0;
					TLOG(TLVL_DEBUG, "WireChamberReceiver")
					    << "Done processing data, buffer size is now "
					    << std::to_string(buffer_.size())
					    << ", expectedDataSize=" << std::to_string(expectedDataSize_);
				}
			}
		}
		else
		{
			TLOG(TLVL_DEBUG, "WireChamberReceiver") << "Received \"NODATA\" from buffer";
		}
	}
	return true;
}

//========================================================================================================================
void ots::WireChamberReceiver::ProcessData_(artdaq::FragmentPtrs& frags)
{
	ots::WireChamberFragment::WireChamberMetadata metadata;

	std::ofstream output;
	// std::cout << __COUT_HDR_FL__ << "SAVING FILE??? " << rawOutput_ << std::endl;
	if(rawOutput_)
	{
		std::string outputPath = rawPath_ + "/WireChamberReceiver_process.bin";
		// std::cout << __COUT_HDR_FL__ << "FILE DATA PATH: " << outputPath << std::endl;
		output.open(outputPath, std::ios::out | std::ios::binary);
		output.write(buffer_.c_str(), expectedDataSize_);
	}

	frags.emplace_back(
	    artdaq::Fragment::FragmentBytes(expectedDataSize_,
	                                    ev_counter(),
	                                    fragment_id(),
	                                    ots::detail::FragmentType::MWPC,
	                                    metadata));
	ev_counter_inc();

	memcpy(frags.back()->dataBegin(), &buffer_[0], expectedDataSize_);

	TLOG(TLVL_DEBUG, "WireChamberReceiver") << "Constructing WireChamberFragment";
	WireChamberFragment ff(*frags.back());
	TLOG(TLVL_DEBUG, "WireChamberReceiver") << "Generating metadata";
	frags.back()->updateMetadata<WireChamberFragment::WireChamberMetadata>(
	    ff.generateMetadata(mwpc_hostname_));
	TLOG(TLVL_DEBUG, "WireChamberReceiver") << "Setting timestamp";
	auto ts = (static_cast<uint64_t>(ff.GetControllerHeader()->YearMonth) << 32) +
	          (ff.GetControllerHeader()->DaysHours << 16) +
	          ff.GetControllerHeader()->MinutesSeconds;
	frags.back()->setTimestamp(ts);
	TLOG(TLVL_DEBUG, "WireChamberReceiver") << "Clearing buffer";
	// buffer_ = "";
	buffer_.erase(0, expectedDataSize_);
	return;
}

//========================================================================================================================
void ots::WireChamberReceiver::start() {}

//========================================================================================================================
void ots::WireChamberReceiver::stop() {}

//========================================================================================================================
void ots::WireChamberReceiver::stopNoMutex() {}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(ots::WireChamberReceiver)
