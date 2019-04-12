#include "otsdaq-trigger/Generators/CAMACReceiver.hh"
#define TRACE_NAME "CAMACReceiver"
#include "artdaq/DAQdata/Globals.hh"

//#include "art/Utilities/Exception.h"
#include "artdaq/Application/GeneratorMacros.hh"
#include "cetlib_except/exception.h"
#include "fhiclcpp/ParameterSet.h"
#include "otsdaq-trigger/Overlays/CAMACFragment.hh"
#include "otsdaq-trigger/Overlays/FragmentType.hh"

#include <sys/poll.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>

//========================================================================================================================
ots::CAMACReceiver::CAMACReceiver(fhicl::ParameterSet const& ps)
    : WorkLoop("CAMACReceiver")
    , DataConsumer(ps.get<std::string>("SupervisorApplicationUID", "ARTDAQDataManager"),
                   ps.get<std::string>("BufferUID", "ARTDAQBuffer"),
                   ps.get<std::string>("ProcessorUID"),
                   HighConsumerPriority)
    , CommandableFragmentGenerator(ps)
    , initialized_(false)
    , rawOutput_(ps.get<bool>("rawOutput", false))
    , rawPath_(ps.get<std::string>("rawOutputPath", "."))
    , ts_(0)
    , lines_()
{
	TLOG(TLVL_INFO) << "CAMACReceiver CONSTRUCTOR";
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << std::endl;
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "registering to buffer!"
	          << std::endl;
	registerToBuffer();
}

//========================================================================================================================
bool ots::CAMACReceiver::getNext_(artdaq::FragmentPtrs& output)
{
	// mf::LogDebug("CAMACReceiver") << "READING DATA!" << std::endl;
	if(should_stop())
		return false;

	// unsigned long block;
	std::string tmpBuffer;
	if(read<std::string, std::map<std::string, std::string>>(tmpBuffer) < 0)
	{
		usleep(10000);
	}
	else
	{
		// TLOG(TLVL_DEBUG) << "Received \"" << tmpBuffer << "\" from FE";
		// std::cout << "CAMACReceiver: Received \"" << tmpBuffer << "\" from FE" <<
		// std::endl;
		if(tmpBuffer != "NODATA")
		{
			if(!initialized_)
			{
				TLOG(TLVL_DEBUG) << "Initializing CAMAC Receiver";
				tmpBuffer = initializeCAMAC_(tmpBuffer);
			}

			if(tmpBuffer.size())
			{
				ProcessInput_(tmpBuffer);
				// std::cout << __COUT_HDR_FL__ << "SAVING FILE??? " << rawOutput_ <<
				// std::endl;
				if(rawOutput_)
				{
					TLOG(TLVL_DEBUG) << "Writing received data to raw file";
					std::ofstream outputStream;
					std::string   outputPath = rawPath_ + "/CAMACReceiver_recv.bin";
					// std::cout << __COUT_HDR_FL__ << "FILE DATA PATH: " << outputPath <<
					// std::endl;
					outputStream.open(outputPath, std::ios::out | std::ios::binary);
					outputStream.write(tmpBuffer.c_str(), tmpBuffer.size());
				}

				TLOG(TLVL_DEBUG) << "Received " << lines_.size() << " events, processing";
				ProcessData_(output);
			}
		}
		else
		{
			TLOG(TLVL_DEBUG) << "Received \"NODATA\" from buffer";
		}
	}
	return true;
}

void ots::CAMACReceiver::ProcessInput_(std::string buffer)
{
	std::vector<uint16_t> line;

	size_t ii = 0;
	for(; ii < buffer.size(); ii += 2)
	{
		auto word = reinterpret_cast<const uint16_t*>(&buffer[ii]);
		if(*word == 65535)
		{
			// if (line.size() == CAMACFragment::LECROY2249A_CHANNELS || line.size() ==
			// CAMACFragment::LECROY2228A_CHANNELS || line.size() ==
			// CAMACFragment::JORWAY85A_CHANNELS) {
			TLOG(TLVL_TRACE) << "Adding line of length " << static_cast<int>(line.size())
			                 << " words to lines list";
			lines_.push_back(line);
			//}
			line.clear();
		}
		else
		{
			line.emplace_back(*word);
		}
	}
}

//========================================================================================================================
void ots::CAMACReceiver::ProcessData_(artdaq::FragmentPtrs& frags)
{
	while(lines_.size())
	{
		// Check for null
		auto line = *lines_.begin();
		if(line.size() == nullEvent_.size())
		{
			bool isNull = true;
			for(size_t ii = 0; ii < line.size(); ++ii)
			{
				if(line[ii] != nullEvent_[ii])
				{
					isNull = false;
					break;
				}
			}

			if(isNull)
			{
				lines_.erase(lines_.begin());
				continue;
			}
		}

		auto expectedDataSize =
		    metadata_.nADCs * sizeof(CAMACFragment::ADCDataPoint) +
		    metadata_.nTDCs * sizeof(CAMACFragment::TDCDataPoint) +
		    metadata_.nScalars * sizeof(CAMACFragment::ScalarDataPoint);
		frags.emplace_back(
		    artdaq::Fragment::FragmentBytes(expectedDataSize,
		                                    ev_counter(),
		                                    fragment_id(),
		                                    ots::detail::FragmentType::CAMAC,
		                                    metadata_));
		ev_counter_inc();
		size_t fragment_offset = 0;
		size_t line_offset     = 1;  // Event word count word

		// It seems as though scalar data is always present. As of 1/22/19, we have not
		// tested with TDCs to see how the system behaves with three different boards...
		if(sizeof(uint16_t) * (line.size() - 2) ==
		   expectedDataSize)  // First word is event word count, last word is terminator
		{
			memcpy(frags.back()->dataBeginBytes(), &(line[1]), expectedDataSize);
		}
		else
		{
			// Special case handling here!
		}

		lines_.erase(lines_.begin());

		TLOG(TLVL_DEBUG) << "Constructing CAMACFragment";
		CAMACFragment ff(*frags.back());
		TLOG(TLVL_DEBUG) << "Generating metadata";
		frags.back()->updateMetadata<CAMACFragment::CAMACMetadata>(metadata_);
		TLOG(TLVL_DEBUG) << "Setting timestamp";
		frags.back()->setTimestamp(++ts_);
	}
	return;
}

//========================================================================================================================
std::string ots::CAMACReceiver::initializeCAMAC_(std::string buffer)
{
	TLOG(TLVL_DEBUG) << "InitializeCAMAC BEGIN";
	std::vector<int> adcs;
	std::vector<int> tdcs;
	std::vector<int> scalars;
	nullEvent_.push_back(0x000a);

	if(buffer.find("END") == std::string::npos)
	{
		TLOG(TLVL_DEBUG)
		    << "Buffer does not contain complete initialization message, returning";
		return buffer;
	}

	std::istringstream is(buffer);

	std::string tmpString;
	is >> tmpString;
	assert(tmpString == "ADC");

	is >> tmpString;
	while(tmpString != "TDC")
	{
		adcs.push_back(std::stoi(tmpString));
		nullEvent_.push_back(0);
		is >> tmpString;
	}
	is >> tmpString;
	while(tmpString != "SCAL")
	{
		tdcs.push_back(std::stoi(tmpString));
		nullEvent_.push_back(0);
		is >> tmpString;
	}
	is >> tmpString;
	while(tmpString != "GATE")
	{
		scalars.push_back(std::stoi(tmpString));
		nullEvent_.push_back(0);
		nullEvent_.push_back(0x0300);
		is >> tmpString;
	}
	is >> tmpString;
	while(tmpString.find("END") == std::string::npos)
	{
		is >> tmpString;
	}
	if(!is.eof())
	{
		size_t pos = is.tellg();
		auto   str = is.str();
		if(pos < str.size())
		{
			buffer = str.substr(pos);
		}
		else
		{
			buffer = "";
		}
	}
	else
	{
		buffer = "";
	}

	metadata_ = CAMACFragment::GenerateMetadata(adcs, tdcs, scalars);

	TLOG(TLVL_DEBUG) << "initializeCAMAC_ END. Configured "
	                 << static_cast<int>(metadata_.nADCs) << " ADCs, "
	                 << static_cast<int>(metadata_.nTDCs) << " TDCs, and "
	                 << static_cast<int>(metadata_.nScalars) << " Scalars";
	initialized_ = true;
	return buffer;
}

void ots::CAMACReceiver::start() {}

//========================================================================================================================
void ots::CAMACReceiver::stop() {}

//========================================================================================================================
void ots::CAMACReceiver::stopNoMutex() {}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(ots::CAMACReceiver)
