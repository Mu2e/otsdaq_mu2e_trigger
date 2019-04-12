#include "artdaq/DAQdata/Globals.hh"
#include "artdaq/DAQdata/TCPConnect.hh"
#include "artdaq/DAQdata/TCP_listen_fd.hh"

#include "artdaq-core/Data/ContainerFragment.hh"
#include "artdaq-core/Data/ContainerFragmentLoader.hh"

#include "otsdaq-trigger/ArtModules/detail/DataRequestMessage.hh"
#include "otsdaq-trigger/Overlays/FragmentType.hh"
#include "otsdaq-trigger/Overlays/STIBFragment.hh"
#include "otsdaq-trigger/Overlays/WireChamberFragment.hh"

#include <art/Framework/Core/EDAnalyzer.h>
#include <art/Framework/Core/ModuleMacros.h>

#include <condition_variable>
#include <mutex>
#include <unordered_map>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <boost/thread.hpp>

namespace ots
{
class EventDataServer : public art::EDAnalyzer
{
  public:
	EventDataServer(fhicl::ParameterSet const& p);
	virtual ~EventDataServer();

	void analyze(art::Event const& e) override;
	void beginRun(art::Run const& r) override;

	struct DAQEvent
	{
		std::unique_ptr<artdaq::Fragments> mwpcFrags;
		std::unique_ptr<artdaq::Fragments> stibFrags;
		artdaq::FragmentPtrs               containers;
	};

  protected:
	size_t GetBufferSizeBytes();
	size_t GetEventSizeBytes(DAQEvent const& e,
	                         bool            wantMWPC = true,
	                         bool            wantSTIB = true);
	size_t GetBufferSizeEvents();

	void startRequestProcessingThread();
	void requestProcessingLoop();

  private:
	std::map<art::EventNumber_t, DAQEvent> eventBuffer_;
	mutable std::mutex                     event_buffer_mutex_;

	int                                  request_socket_;
	int                                  request_epoll_fd_;
	std::set<int>                        response_sockets_;
	std::unordered_map<int, std::string> receive_request_addrs_;
	std::vector<epoll_event>             receive_request_events_;
	int                                  request_port_;

	size_t max_buffer_size_events_;
	size_t max_buffer_size_bytes_;

	std::atomic<bool> should_stop_;
	boost::thread     requestProcessingThread_;
};

//========================================================================================================================
EventDataServer::EventDataServer(fhicl::ParameterSet const& ps)
    : art::EDAnalyzer(ps)
    , eventBuffer_()
    , request_socket_(-1)
    , request_epoll_fd_(-1)
    , response_sockets_()
    , receive_request_addrs_()
    , receive_request_events_(10)
    , request_port_(ps.get<int>("request_port", 44555))
    , max_buffer_size_events_(ps.get<size_t>("max_buffer_size_events", 10))
    , max_buffer_size_bytes_(ps.get<size_t>("max_buffer_size_bytes",
                                            5ULL * 1024ULL * 1024ULL * 1024ULL))  // 5 GB
    , should_stop_(false)
{
}

EventDataServer::~EventDataServer()
{
	should_stop_ = true;
	if(requestProcessingThread_.joinable())
		requestProcessingThread_.join();
}

void EventDataServer::beginRun(art::Run const&) { startRequestProcessingThread(); }

void EventDataServer::analyze(art::Event const& e)
{
	TLOG_INFO("EventDataServer")
	    << "EventDataServer Analyzing event " << e.event() << TLOG_ENDL;

	// John F., 1/22/14 -- there's probably a more elegant way of
	// collecting fragments of various types using ART interface code;
	// will investigate. Right now, we're actually re-creating the
	// fragments locally

	DAQEvent                 evt;
	std::vector<std::string> fragment_type_labels{
	    "MWPC", "ContainerMWPC", "STIB", "ContainerSTIB", "Container"};

	TLOG_INFO("EventDataServer") << "EventDataServer Extracting Fragments" << TLOG_ENDL;
	for(auto label : fragment_type_labels)
	{
		art::Handle<artdaq::Fragments> fragments_with_label;
		e.getByLabel("daq", label, fragments_with_label);

		if(!fragments_with_label.isValid())
			continue;
		//    for (int i_l = 0; i_l < static_cast<int>(fragments_with_label->size());
		//    ++i_l) {
		//      fragments.emplace_back( (*fragments_with_label)[i_l] );
		//    }

		if(label == "ContainerMWPC" || label == "ContainerSTIB" || label == "Container")
		{
			for(auto cont : *fragments_with_label)
			{
				artdaq::ContainerFragment contf(cont);
				for(size_t ii = 0; ii < contf.block_count(); ++ii)
				{
					evt.containers.push_back(contf[ii]);
					switch(contf[ii]->type())
					{
					case FragmentType::MWPC:
						evt.mwpcFrags->emplace_back(*evt.containers.back());
						break;
					case FragmentType::STIB:
						evt.stibFrags->emplace_back(*evt.containers.back());
						break;
					default:
						TLOG_WARNING("EventDataReceiver")
						    << "Unknown Fragment Type supplied under known label!"
						    << TLOG_ENDL;
						break;
					}
				}
			}
		}
		else
		{
			for(auto frag : *fragments_with_label)
			{
				switch(frag.type())
				{
				case FragmentType::MWPC:
					evt.mwpcFrags->emplace_back(frag);
					break;
				case FragmentType::STIB:
					evt.stibFrags->emplace_back(frag);
					break;
				default:
					TLOG_WARNING("EventDataReceiver")
					    << "Unknown Fragment Type supplied under known label!"
					    << TLOG_ENDL;
					break;
				}
			}
		}
	}
	TLOG_INFO("EventDataServer")
	    << "EventDataServer: This event has " << evt.mwpcFrags->size()
	    << " MWPC Fragments"
	    << ", and " << evt.stibFrags->size() << " STIB Fragments" << TLOG_ENDL;

	std::unique_lock<std::mutex> lk(event_buffer_mutex_);
	eventBuffer_[e.event()] = std::move(evt);

	while(GetBufferSizeEvents() > max_buffer_size_events_)
	{
		eventBuffer_.erase(eventBuffer_.begin());
	}

	while(GetBufferSizeBytes() > max_buffer_size_bytes_)
	{
		eventBuffer_.erase(eventBuffer_.begin());
	}

	TLOG_DEBUG("EventDataServer")
	    << "Data buffer stats: " << GetBufferSizeEvents() << "/"
	    << max_buffer_size_events_ << " events, " << GetBufferSizeBytes() << "/"
	    << max_buffer_size_bytes_ << " bytes." << TLOG_ENDL;
}

size_t EventDataServer::GetEventSizeBytes(DAQEvent const& e, bool wantMWPC, bool wantSTIB)
{
	size_t size = 0;
	if(wantMWPC)
	{
		for(auto f = e.mwpcFrags->begin(); f != e.mwpcFrags->end(); ++f)
		{
			size += f->sizeBytes();
		}
	}
	if(wantSTIB)
	{
		for(auto f = e.stibFrags->begin(); f != e.stibFrags->end(); ++f)
		{
			size += f->sizeBytes();
		}
	}
	return size;
}

size_t EventDataServer::GetBufferSizeBytes()
{
	size_t size = 0;

	for(auto& ev : eventBuffer_)
	{
		GetEventSizeBytes(ev.second);
	}

	return size;
}

size_t EventDataServer::GetBufferSizeEvents() { return eventBuffer_.size(); }

void EventDataServer::startRequestProcessingThread()
{
	if(requestProcessingThread_.joinable())
		requestProcessingThread_.join();
	TLOG_INFO("EventDataServer") << "Starting Request Processing Thread" << TLOG_ENDL;
	requestProcessingThread_ =
	    boost::thread(&EventDataServer::requestProcessingLoop, this);
}

void EventDataServer::requestProcessingLoop()
{
	while(!should_stop_)
	{
		TLOG_DEBUG("EventDataServer") << "Request Processing loop start" << TLOG_ENDL;
		if(request_socket_ == -1)
		{
			TLOG_DEBUG("EventDataServer")
			    << "Opening request listener socket" << TLOG_ENDL;
			request_socket_ =
			    TCP_listen_fd(request_port_, 3 * sizeof(DataRequestMessage));

			if(request_epoll_fd_ != -1)
				close(request_epoll_fd_);
			struct epoll_event ev;
			request_epoll_fd_ = epoll_create1(0);
			ev.events         = EPOLLIN | EPOLLPRI;
			ev.data.fd        = request_socket_;
			if(epoll_ctl(request_epoll_fd_, EPOLL_CTL_ADD, request_socket_, &ev) == -1)
			{
				TLOG_ERROR("EventDataServer")
				    << "Could not register listen socket to epoll fd" << TLOG_ENDL;
				exit(3);
			}
		}
		if(request_socket_ == -1 || request_epoll_fd_ == -1)
		{
			TLOG_DEBUG("EventDataServer")
			    << "One of the listen sockets was not opened successfully." << TLOG_ENDL;
			return;
		}

		auto nfds = epoll_wait(request_epoll_fd_,
		                       &receive_request_events_[0],
		                       receive_request_events_.size(),
		                       1000);
		if(nfds == -1)
		{
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}

		TLOG_DEBUG("EventDataServer")
		    << "Received " << std::to_string(nfds) << " events" << TLOG_ENDL;
		for(auto n = 0; n < nfds; ++n)
		{
			if(receive_request_events_[n].data.fd == request_socket_)
			{
				TLOG_DEBUG("EventDataServer")
				    << "Accepting new connection on request_socket" << TLOG_ENDL;
				sockaddr_in addr;
				socklen_t   arglen = sizeof(addr);
				auto        conn_sock =
				    accept(request_socket_, (struct sockaddr*)&addr, &arglen);

				if(conn_sock == -1)
				{
					perror("accept");
					exit(EXIT_FAILURE);
				}

				receive_request_addrs_[conn_sock] = std::string(inet_ntoa(addr.sin_addr));
				struct epoll_event ev;
				ev.events  = EPOLLIN | EPOLLET;
				ev.data.fd = conn_sock;
				if(epoll_ctl(request_epoll_fd_, EPOLL_CTL_ADD, conn_sock, &ev) == -1)
				{
					perror("epoll_ctl: conn_sock");
					exit(EXIT_FAILURE);
				}
			}
			else
			{
				auto               fd = receive_request_events_[n].data.fd;
				DataRequestMessage req;
				auto               sts = read(fd, &req, sizeof(DataRequestMessage));

				if(!req.isValid())
				{
					TLOG_WARNING("EventDataServer")
					    << "Received bad request from " << receive_request_addrs_[fd]
					    << "!" << TLOG_ENDL;
					continue;
				}

				std::unique_lock<std::mutex> dl(event_buffer_mutex_);
				if(eventBuffer_.count(req.eventNumber))
				{
					DataResponseHeader hdr(
					    (req.wantMWPC ? 1 : 0) + (req.wantSTIB ? 1 : 0),
					    eventBuffer_.begin()->first,
					    eventBuffer_.end()->first);
					write(fd, &hdr, sizeof(hdr));
					if(req.wantMWPC)
					{
						artdaq::Fragment                mwpc_container;
						artdaq::ContainerFragmentLoader mwpc_loader(mwpc_container);

						for(auto& frag : *eventBuffer_[req.eventNumber].mwpcFrags.get())
						{
							mwpc_loader.addFragment(frag);
						}

						write(fd,
						      mwpc_container.headerAddress(),
						      mwpc_container.sizeBytes());
					}
					if(req.wantSTIB)
					{
						artdaq::Fragment                stib_container;
						artdaq::ContainerFragmentLoader stib_loader(stib_container);

						for(auto& frag : *eventBuffer_[req.eventNumber].stibFrags.get())
						{
							stib_loader.addFragment(frag);
						}

						write(fd,
						      stib_container.headerAddress(),
						      stib_container.sizeBytes());
					}
				}
				else
				{
					DataResponseHeader hdr(
					    0, eventBuffer_.begin()->first, eventBuffer_.end()->first);
					write(fd, &hdr, sizeof(hdr));
				}
			}
		}
	}
}
}
DEFINE_ART_MODULE(ots::EventDataServer)
