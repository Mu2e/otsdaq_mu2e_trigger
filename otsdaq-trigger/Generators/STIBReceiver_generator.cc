
#include "artdaq/DAQrate/RequestReceiver.hh"
#include "otsdaq-trigger/Overlays/FragmentType.hh"
#include "otsdaq-trigger/Overlays/STIBFragment.hh"

#include "artdaq-ots/Generators/UDPReceiver.hh"
#include "otsdaq-core/Macros/CoutMacros.h"

//#include "art/Utilities/Exception.h"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/Application/GeneratorMacros.hh"

#include "canvas/Utilities/Exception.h"
#include "cetlib_except/exception.h"
#include "fhiclcpp/ParameterSet.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>

#define VERBOSE_DEBUGGING 0

namespace ots
{
class STIBReceiver : public artdaq::CommandableFragmentGenerator
{
  public:
	explicit STIBReceiver(fhicl::ParameterSet const& ps);
	virtual ~STIBReceiver();

  private:
	struct STIBEvent
	{
		STIBFragment::EventHeader header;
		std::vector<uint32_t>     data;
	};

	bool getNext_(artdaq::FragmentPtrs& frags) override;
	void start(void) override;
	void stop(void) override { running_ = false; }
	void stopNoMutex(void) override {}
	void ProcessData_();
	void CreateFragment_(STIBFragment::EventHeader triggerData);

	size_t GetDataBufferSize() const;

	void receiveLoop_();

  private:
	ots::packetBuffer_list_t packetBuffers_;

	bool        rawOutput_;
	std::string rawPath_;

	// FHiCL-configurable variables. Note that the C++ variable names
	// are the FHiCL variable names with a "_" appended

	int         dataport_;
	std::string ip_;
	int         rcvbuf_;

	// The packet number of the next packet. Used to discover dropped packets
	uint8_t expectedPacketNumber_;

	// Socket parameters
	struct sockaddr_in           si_data_;
	int                          datasocket_;
	bool                         sendCommands_;
	bool                         running_;
	std::unique_ptr<std::thread> receiverThread_;
	std::mutex                   receiveBufferLock_;
	std::mutex                   dataBufferLock_;
	ots::packetBuffer_list_t     receiveBuffers_;

	uint64_t                                triggerOffset_;
	uint64_t                                triggerWidth_;
	bool                                    triggerOffsetSearch_;
	std::map<uint32_t, std::list<uint32_t>> events_;
	artdaq::RequestReceiver                 requests_;
	size_t                                  dataBufferMaxCount_;
	size_t                                  dataBufferMaxSizeBytes_;
	std::vector<STIBEvent>                  dataBuffer_;
};

STIBReceiver::STIBReceiver(fhicl::ParameterSet const& ps)
    : CommandableFragmentGenerator(ps)
    , rawOutput_(ps.get<bool>("raw_output_enabled", false))
    , rawPath_(ps.get<std::string>("raw_output_path", "/tmp"))
    , dataport_(ps.get<int>("port", 6343))
    , ip_(ps.get<std::string>("ip", "127.0.0.1"))
    , rcvbuf_(ps.get<int>("rcvbuf", 0x1000000))
    , expectedPacketNumber_(0)
    , sendCommands_(ps.get<bool>("send_OtsUDP_commands", false))
    , running_(false)
    , receiverThread_(nullptr)
    , triggerOffset_(ps.get<uint64_t>("trigger_clock_delay", 0))
    , triggerWidth_(ps.get<uint64_t>("trigger_width", 1))
    , triggerOffsetSearch_(ps.get<bool>("trigger_offset_search", false))
    , events_()
    , requests_(ps)
    , dataBufferMaxCount_(ps.get<size_t>("max_triggers_in_buffer", 200000))
    , dataBufferMaxSizeBytes_(ps.get<size_t>("max_buffer_size_bytes", 0x1000000))
{
	datasocket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(!datasocket_)
	{
		throw art::Exception(art::errors::Configuration)
		    << "STIBReceiver: Error creating socket!";
		exit(1);
	}

	struct sockaddr_in si_me_data;
	si_me_data.sin_family      = AF_INET;
	si_me_data.sin_port        = htons(dataport_);
	si_me_data.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(datasocket_, (struct sockaddr*)&si_me_data, sizeof(si_me_data)) == -1)
	{
		throw art::Exception(art::errors::Configuration)
		    << "STIBReceiver: Cannot bind data socket to port " << dataport_;
		exit(1);
	}
	/*if(fcntl(datasocket_, F_SETFL, O_NONBLOCK) == -1) {

	    throw art::Exception(art::errors::Configuration) <<
	    "STIBReceiver: Cannot set socket to nonblocking!" << TLOG_ENDL;
	    }*/

	if(rcvbuf_ > 0 &&
	   setsockopt(datasocket_, SOL_SOCKET, SO_RCVBUF, &rcvbuf_, sizeof(rcvbuf_)))
	{
		throw art::Exception(art::errors::Configuration)
		    << "STIBReceiver: Could not set receive buffer size: " << rcvbuf_;
		exit(1);
	}

	si_data_.sin_family = AF_INET;
	si_data_.sin_port   = htons(dataport_);
	if(inet_aton(ip_.c_str(), &si_data_.sin_addr) == 0)
	{
		throw art::Exception(art::errors::Configuration)
		    << "STIBReceiver: Could not translate provided IP Address: " << ip_;
		exit(1);
	}
	TLOG_INFO("STIBReceiver") << "UDP Receiver Construction Complete!" << TLOG_ENDL;
}

STIBReceiver::~STIBReceiver()
{
	TLOG_INFO("STIBReceiver") << "UDP Receiver Destructor Started!" << TLOG_ENDL;
	running_ = false;
	if(receiverThread_ && receiverThread_->joinable())
		receiverThread_->join();

	if(datasocket_ >= 0)
	{
		close(datasocket_);
		datasocket_ = -1;
	}
	TLOG_INFO("STIBReceiver") << "UDP Receiver Destructor Complete!" << TLOG_ENDL;
}

void STIBReceiver::start()
{
	TLOG_INFO("STIBReceiver") << "Starting..." << TLOG_ENDL;

	running_ = false;
	if(receiverThread_ && receiverThread_->joinable())
	{
		receiverThread_->join();
	}
	running_ = true;
	receiverThread_.reset(new std::thread(&STIBReceiver::receiveLoop_, this));
	if(!requests_.isRunning())
		requests_.startRequestReception();
}

bool STIBReceiver::getNext_(artdaq::FragmentPtrs& frags)
{
	if(should_stop())
	{
		return false;
	}

	{
		std::unique_lock<std::mutex> lock(receiveBufferLock_);
		std::move(receiveBuffers_.begin(),
		          receiveBuffers_.end(),
		          std::inserter(packetBuffers_, packetBuffers_.end()));
		receiveBuffers_.clear();
	}

	if(packetBuffers_.size() > 0)
	{
		size_t packetBufferSize = 0;
		for(auto& buf : packetBuffers_)
		{
			packetBufferSize += buf.size();
		}
		TLOG_TRACE("STIBReceiver")
		    << "Calling ProcessData, packetBuffers_.size() == "
		    << std::to_string(packetBuffers_.size())
		    << ", sz = " << std::to_string(packetBufferSize) << TLOG_ENDL;
		ProcessData_();

		packetBuffers_.clear();
	}
	else
	{
		// Sleep 10 times per poll timeout
		usleep(100000);
	}

	{
		std::unique_lock<std::mutex> lockdb(dataBufferLock_);

		if(requests_.GetRequests().size() > 1)
		{
			TLOG_ERROR("STIBReceiver") << "More than one request received! Only replying "
			                              "to the FIRST one with data!"
			                           << TLOG_ENDL;
		}
		for(auto& req : requests_.GetRequests())
		{
			STIBFragment::Metadata metadata;
			metadata.port          = dataport_;
			metadata.address       = si_data_.sin_addr.s_addr;
			metadata.trigger_count = dataBuffer_.size();
			frags.emplace_back(artdaq::Fragment::FragmentBytes(GetDataBufferSize(),
			                                                   req.first,
			                                                   fragment_id(),
			                                                   detail::FragmentType::STIB,
			                                                   metadata,
			                                                   req.second));
			size_t offset = 0;
			for(auto& dat : dataBuffer_)
			{
				memcpy(frags.back()->dataBeginBytes() + offset,
				       &(dat.header),
				       sizeof(STIBFragment::EventHeader));
				offset += sizeof(STIBFragment::EventHeader);
				memcpy(frags.back()->dataBeginBytes() + offset,
				       &(dat.data[0]),
				       sizeof(uint32_t) * dat.data.size());
				offset += dat.data.size() * sizeof(uint32_t);
				TLOG_TRACE("STIBReceiver")
				    << "Added trigger " << std::to_string(dat.header.trigger_counter)
				    << ", BCO " << std::to_string(dat.header.bunch_counter) << ", and "
				    << std::to_string(dat.data.size()) << " hits to output Fragment"
				    << TLOG_ENDL;
			}
			dataBuffer_.clear();
			TLOG_DEBUG("STIBReceiver")
			    << "STIB Fragment created with " << std::to_string(metadata.trigger_count)
			    << " triggers." << TLOG_ENDL;
			requests_.RemoveRequest(req.first);
		}

		// Trim buffer AFTER fulfilling any requests
		while(dataBuffer_.size() > dataBufferMaxCount_)
		{
			dataBuffer_.erase(dataBuffer_.begin());
		}
		while(GetDataBufferSize() > dataBufferMaxSizeBytes_)
		{
			dataBuffer_.erase(dataBuffer_.begin());
		}
	}
	return true;
}

void STIBReceiver::receiveLoop_()
{
	while(!should_stop())
	{
		struct pollfd ufds[1];
		ufds[0].fd     = datasocket_;
		ufds[0].events = POLLIN | POLLPRI;

		int rv = poll(ufds, 1, 1000);
		if(rv > 0)
		{
			TLOG_TRACE("STIBReceiver") << "revents: " << ufds[0].revents << ", "
			                           << TLOG_ENDL;  // ufds[1].revents << TLOG_ENDL;
			if(ufds[0].revents == POLLIN || ufds[0].revents == POLLPRI)
			{
				// FIXME -> IN THE STIB GENERATOR WE DON'T HAVE A HEADER
				// FIXME -> IN THE STIB GENERATOR WE DON'T HAVE A HEADER
				// FIXME -> IN THE STIB GENERATOR WE DON'T HAVE A HEADER
				uint8_t   peekBuffer[4];
				socklen_t dataSz = sizeof(si_data_);
				recvfrom(datasocket_,
				         peekBuffer,
				         sizeof(peekBuffer),
				         MSG_PEEK,
				         (struct sockaddr*)&si_data_,
				         &dataSz);

				TLOG_TRACE("STIBReceiver")
				    << "Received UDP Datagram with sequence number " << std::hex << "0x"
				    << static_cast<int>(peekBuffer[1]) << "!" << std::dec << TLOG_ENDL;
				TLOG_TRACE("STIBReceiver")
				    << "peekBuffer[1] == expectedPacketNumber_: " << std::hex
				    << static_cast<int>(peekBuffer[1])
				    << " =?= " << (int)expectedPacketNumber_ << TLOG_ENDL;
				TLOG_TRACE("STIBReceiver")
				    << "peekBuffer: 0: " << std::hex << static_cast<int>(peekBuffer[0])
				    << ", 1: " << std::hex << static_cast<int>(peekBuffer[1])
				    << ", 2: " << std::hex << static_cast<int>(peekBuffer[2])
				    << ", 3: " << std::hex << static_cast<int>(peekBuffer[3])
				    << TLOG_ENDL;

				uint8_t seqNum = peekBuffer[1];
				// ReturnCode dataCode = getReturnCode(peekBuffer[0]);
				if(seqNum >= expectedPacketNumber_ ||
				   (seqNum < 64 && expectedPacketNumber_ > 192))
				{
					if(seqNum != expectedPacketNumber_)
					{
						int delta = seqNum - expectedPacketNumber_;
						TLOG_WARNING("STIBReceiver")
						    << std::dec
						    << "Sequence Number different than expected! (delta: "
						    << delta << ")" << TLOG_ENDL;
						expectedPacketNumber_ = seqNum;
					}

					ots::packetBuffer_t receiveBuffer;
					receiveBuffer.resize(1500);
					int sts = recvfrom(datasocket_,
					                   &receiveBuffer[0],
					                   receiveBuffer.size(),
					                   0,
					                   (struct sockaddr*)&si_data_,
					                   &dataSz);
					if(sts == -1)
					{
						TLOG_WARNING("STIBReceiver")
						    << "Error on socket: " << strerror(errno) << TLOG_ENDL;
					}
					else
					{
						TLOG_TRACE("STIBReceiver")
						    << "Received " << sts << " bytes." << TLOG_ENDL;
					}

					std::unique_lock<std::mutex> lock(receiveBufferLock_);
					TLOG_TRACE("STIBReceiver")
					    << "Now placing UDP datagram with sequence number " << std::hex
					    << (int)seqNum << " into buffer." << std::dec << TLOG_ENDL;
					receiveBuffers_.push_back(receiveBuffer);

					++expectedPacketNumber_;
				}
				else
				{
					// Receiving out-of-order datagram, then moving on...
					TLOG_WARNING("STIBReceiver")
					    << "Received out-of-order datagram: " << seqNum
					    << " != " << expectedPacketNumber_ << " (expected)" << TLOG_ENDL;
					ots::packetBuffer_t receiveBuffer;
					receiveBuffer.resize(1500);
					int sts = recvfrom(datasocket_,
					                   &receiveBuffer[0],
					                   receiveBuffer.size(),
					                   0,
					                   (struct sockaddr*)&si_data_,
					                   &dataSz);
				}
			}
		}
	}
	TLOG_INFO("STIBReceiver") << "receive Loop exiting..." << TLOG_ENDL;
}

size_t STIBReceiver::GetDataBufferSize() const
{
	size_t ret = 0;
	for(auto& evt : dataBuffer_)
	{
		ret += sizeof(STIBFragment::EventHeader) + sizeof(uint32_t) * evt.data.size();
	}
	return ret;
}

void STIBReceiver::CreateFragment_(STIBFragment::EventHeader triggerData)
{
	STIBEvent thisEvent;
	thisEvent.header = triggerData;

	if(triggerOffsetSearch_)
	{
		for(auto& evt : events_)
		{
			if(evt.second.size())
			{
				TLOG_DEBUG("STIBReceiver")
				    << "Event at BCO " << std::to_string(evt.first) << " has "
				    << std::to_string(evt.second.size()) << " hits. "
				    << "Trigger BCO " << std::to_string(triggerData.bunch_counter) << ", "
				    << "Start: "
				    << std::to_string(triggerData.bunch_counter - triggerOffset_)
				    << ", End: "
				    << std::to_string(triggerData.bunch_counter - triggerOffset_ +
				                      triggerWidth_ - 1)
				    << ", "
				    << "BCO Offset="
				    << std::to_string(triggerData.bunch_counter - evt.first) << TLOG_ENDL;
			}
		}
	}

	for(auto it = triggerData.bunch_counter - triggerOffset_;
	    it < triggerData.bunch_counter - triggerOffset_ + triggerWidth_;
	    ++it)
	{
		if(events_.count(it))
		{
			thisEvent.data.insert(
			    thisEvent.data.end(), events_[it].begin(), events_[it].end());
		}
	}
	events_.erase(
	    events_.begin(),
	    events_.find(triggerData.bunch_counter - triggerOffset_ + triggerWidth_));

	thisEvent.header.event_data_count = thisEvent.data.size();

	std::unique_lock<std::mutex> lockdb(dataBufferLock_);
	dataBuffer_.push_back(thisEvent);
}

void STIBReceiver::ProcessData_()
{
	std::ofstream output;
	if(rawOutput_)
	{
		std::string outputPath =
		    rawPath_ + "/STIBReceiver-" + ip_ + ":" + std::to_string(dataport_) + ".bin";
		output.open(outputPath, std::ios::out | std::ios::app | std::ios::binary);
	}

	TLOG_TRACE("STIBReceiver") << "Starting STIBReceiver Packet Processing Loop"
	                           << TLOG_ENDL;
	size_t counter = 1;
	for(auto packet = packetBuffers_.begin(); packet != packetBuffers_.end(); ++packet)
	{
		TLOG_TRACE("STIBReceiver") << "Packet " << std::to_string(counter) << " / "
		                           << std::to_string(packetBuffers_.size()) << TLOG_ENDL;
		counter++;
		for(size_t word = 2; word < packet->size() - 8; word += 8)
		{
			auto data = *reinterpret_cast<uint32_t*>(&(packet->at(word + 0)));
			auto bco  = *reinterpret_cast<uint32_t*>(&(packet->at(word + 4)));

			if(*reinterpret_cast<uint32_t*>(&(*packet)[word]) == 0)
				continue;

#if 0
				TLOG_TRACE("STIBReceiver") << "Bytes are: 0: 0x" << std::hex << static_cast<int>(byte0) << ", 1: 0x" << static_cast<int>(byte1) << ", 2: 0x" << static_cast<int>(byte2) << ", 3: 0x" << static_cast<int>(byte3) << "." << TLOG_ENDL;
#endif

			if(rawOutput_)
			{
				output.write((char*)&(packet->at(word)), sizeof(uint64_t));
			}
			if((data & 0x1) == 0x1)
			{
#if VERBOSE_DEBUGGING
				TLOG_TRACE("STIBReceiver") << "Data marker!" << TLOG_ENDL;
#endif
				events_[bco].push_back(data);
			}
			else
			{  // Trigger Counter Low
#if VERBOSE_DEBUGGING
				TLOG_TRACE("STIBReceiver") << "Trigger marker!" << TLOG_ENDL;
#endif
				STIBFragment::EventHeader triggerData;
				triggerData.bunch_counter   = bco;
				triggerData.trigger_counter = ((data & 0x01FFFFFE) >> 1);
				triggerData.fastBCO         = ((data & 0x0E000000) >> 25);
				triggerData.system_status   = ((data & 0xF0000000) >> 28);
				CreateFragment_(triggerData);
			}
		}
	}
	return;
}
}
// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(ots::STIBReceiver)
