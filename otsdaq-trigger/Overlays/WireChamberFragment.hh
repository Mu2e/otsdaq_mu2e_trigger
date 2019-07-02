#ifndef OTSDAQ_FERMILABTESTBEAM_OVERLAYS_WIRECHAMBERFRAGMENT_HH
#define OTSDAQ_FERMILABTESTBEAM_OVERLAYS_WIRECHAMBERFRAGMENT_HH 1

#include <vector>
#include "artdaq-core/Data/Fragment.hh"

#define MAX_TDC_COUNT 16

namespace ots
{
class WireChamberFragment
{
  public:
	struct WireChamberMetadata
	{
		typedef uint16_t tdcDataWord;

		uint8_t  TDCCount;
		uint32_t EventCount;
		char     WireChamberHostname[256];
	};

	struct ControllerHeader
	{
		uint16_t WordCountHigh;
		uint16_t WordCountLow;
		uint16_t SpillCounter;
		uint16_t YearMonth;
		uint16_t DaysHours;
		uint16_t MinutesSeconds;
		uint16_t TriggerCountHigh;
		uint16_t TriggerCountLow;
		uint16_t TDCStatus;
		uint16_t LinkStatus;
	};
	static int ConvertBCD(uint8_t bcd)
	{
		return static_cast<int>(bcd);
	}  //(bcd & 0xF) + 10 * ((bcd & 0xF0) >> 4)); }
	uint32_t ControllerWordCount(ControllerHeader const* hdr) const
	{
		return (hdr->WordCountHigh << 16) + hdr->WordCountLow;
	}
	uint32_t ControllerTriggerCount(ControllerHeader const* hdr) const
	{
		return (hdr->TriggerCountHigh << 16) + hdr->TriggerCountLow;
	}

	struct TDCHeader
	{
		uint16_t WordCountHigh;
		uint16_t WordCountLow;
		uint16_t TDCNumber : 5;  // 5 bits, LSB
		uint16_t tdcNumberPad : 11;
		uint16_t TriggerCountHigh;
		uint16_t TriggerCountLow;
		uint16_t Status : 8;  // 8 bits, LSB
		uint16_t statusPad : 8;
	};
	uint32_t TDCHeaderWordCount(TDCHeader const* hdr) const
	{
		return (hdr->WordCountHigh << 16) + hdr->WordCountLow;
	}
	uint32_t TDCHeaderTriggerCount(TDCHeader const* hdr) const
	{
		return (hdr->TriggerCountHigh << 16) + hdr->TriggerCountLow;
	}

	struct TDCEvent
	{
		uint16_t WordCount : 8;  // 8 bits, LSB
		uint16_t wordCountPad : 8;
		uint16_t TDCNumber : 5;  // 5 bits, LSB
		uint16_t tdcNumberPad : 11;
		uint16_t EventStatus : 8;  // 8 bits, LSB
		uint16_t eventStatusPad : 8;
		uint16_t TriggerCountHigh;
		uint16_t TriggerCountLow;
		uint16_t TriggerType : 4;  // 4 bits
		uint16_t triggerTypePad : 12;
		uint16_t ControllerEventTimeStamp : 12;  // 12 bits
		uint16_t controllerEventTimestampPad : 4;
		uint16_t LocalEventTimeStampHigh;
		uint16_t LocalEventTimeStampLow;
	};
	uint32_t TDCEventTriggerCounter(TDCEvent const* hdr) const
	{
		return (hdr->TriggerCountHigh << 16) + hdr->TriggerCountLow;
	}
	uint32_t TDCEventTimestamp(TDCEvent const* hdr) const
	{
		return (hdr->LocalEventTimeStampHigh << 16) + hdr->LocalEventTimeStampLow;
	}

	struct TDCDataWord
	{
		uint16_t TimeBits : 10;      // LSB
		uint16_t ChannelNumber : 6;  // MSB
	};

	static constexpr size_t WireChamberFragmentMinSize =
	    sizeof(ControllerHeader) + sizeof(TDCHeader) + sizeof(TDCEvent);

	explicit WireChamberFragment(artdaq::Fragment const& f) : artdaq_fragment_(f) {}

	WireChamberMetadata generateMetadata(std::string hostname) const
	{
		WireChamberMetadata output;

		snprintf(output.WireChamberHostname, 256, hostname.c_str());

		output.EventCount = ControllerTriggerCount(GetControllerHeader());
		output.TDCCount   = 0;
		// For validation
		size_t expectedWordCount =
		    ControllerWordCount(GetControllerHeader()) - sizeof(ControllerHeader) / 2;

		bool   done               = false;
		auto   tdcPtr             = GetTDCHeaders();
		size_t tdcWordCount       = 0;
		size_t processedWordCount = 0;

		while(!done)
		{
			auto thisCount = TDCHeaderWordCount(tdcPtr);

			// Stop when we encounter the first event header
			if((thisCount & 0x00FF001F) == thisCount && (thisCount & 0x1F) == 1)
			{
				done = true;
				break;
			}

			output.TDCCount++;
			tdcWordCount += thisCount;
			if(TDCHeaderTriggerCount(tdcPtr) != output.EventCount)
			{
				std::cerr << "ERROR: Different number of triggers in Controller Header "
				             "and TDC Header!"
				          << std::endl;
			}
			tdcPtr += 1;

			processedWordCount += sizeof(TDCHeader) / 2;
			// Stop if we are out of data
			if(processedWordCount >= expectedWordCount)
			{
				done = true;
			}
		}

		if(expectedWordCount != tdcWordCount)
		{
			std::cerr
			    << "ERROR: TDC Headers report different size than the controller header!"
			    << std::endl
			    << "Controller Header: " << std::to_string(expectedWordCount) << std::endl
			    << "TDC Headers: " << std::to_string(tdcWordCount) << std::endl
			    << "Fragment Size: "
			    << std::to_string(artdaq_fragment_.dataSizeBytes() / 2) << std::endl;
		}

		return output;
	}

	ControllerHeader const* GetControllerHeader() const
	{
		return reinterpret_cast<ControllerHeader const*>(artdaq_fragment_.dataBegin());
	}

	TDCHeader const* GetTDCHeaders() const
	{
		return reinterpret_cast<TDCHeader const*>(GetControllerHeader() + 1);
	}

	TDCEvent const* dataBegin() const
	{
		return metadata()->EventCount > 0
		           ? reinterpret_cast<TDCEvent const*>(GetTDCHeaders() + tdcHeaderCount())
		           : nullptr;
	}

	bool validateEvent(TDCEvent const* evt) const
	{
		if(!evt || evt == nullptr)
			return true;

		bool validEvent = true;
		if(evt->controllerEventTimestampPad != 0 || evt->eventStatusPad != 0 ||
		   evt->tdcNumberPad != 0 || evt->triggerTypePad != 0 || evt->wordCountPad != 0)
		{
			validEvent = false;
			auto ptr   = reinterpret_cast<uint16_t const*>(evt);
			auto word1 = *ptr;
			auto word2 = *(ptr + 1);
			auto word3 = *(ptr + 2);
			auto word4 = *(ptr + 3);
			auto word5 = *(ptr + 4);
			auto word6 = *(ptr + 5);
			auto word7 = *(ptr + 6);
			TRACE(4,
			      "WCF::validateEvent: Data detected in padding! Mask: 00FF 001F 00FF "
			      "FFFF FFFF 000F 0FFF");
			TRACE(4,
			      "WCF::validateEvent: Data: %x %x %x %x %x %x %x",
			      word1,
			      word2,
			      word3,
			      word4,
			      word5,
			      word6,
			      word7);
		}
		if(evt->WordCount < sizeof(TDCEvent) / 2)
		{
			TRACE(4, "WireChamberFragment::validateEvent: WordCount too small");
			validEvent = false;
		}

		return validEvent;
	}

	TDCEvent const* getValidEvent(TDCEvent const* evt) const
	{
		if(!evt || evt == nullptr)
			return nullptr;
		auto end     = artdaq_fragment_.dataEndBytes();
		auto bytePtr = reinterpret_cast<uint8_t const*>(evt);
		TRACE(4,
		      "getValidEvent: end=%p, bytePtr=%p, evt=%p",
		      (void*)end,
		      (void*)bytePtr,
		      (void*)evt);
		while(!validateEvent(evt))
		{
			bytePtr += 1;
			if(bytePtr >= end)
				return nullptr;
			evt = reinterpret_cast<TDCEvent const*>(bytePtr);
		}

		return evt;
	}

	TDCEvent const* nextEvent(TDCEvent const* evt) const
	{
		auto end  = reinterpret_cast<uint16_t const*>(artdaq_fragment_.dataEndBytes());
		auto next = reinterpret_cast<uint16_t const*>(evt) + evt->WordCount;
		if(next >= end)
			return nullptr;
		return getValidEvent(reinterpret_cast<TDCEvent const*>(next));
	}

	TDCDataWord const* eventData(TDCEvent const* evt, size_t& dataSize) const
	{
		auto data          = reinterpret_cast<TDCDataWord const*>(evt + 1);
		auto dataWordCount = evt->WordCount - (sizeof(TDCEvent) / 2);
		dataSize           = dataWordCount;
		return data;
	}

	WireChamberMetadata const* metadata() const
	{
		if(!artdaq_fragment_.hasMetadata())
		{
			auto md = generateMetadata("UNKNOWN MWPC");
			const_cast<artdaq::Fragment&>(artdaq_fragment_)
			    .setMetadata<WireChamberMetadata>(md);
		}
		return artdaq_fragment_.metadata<WireChamberMetadata>();
	}

	size_t tdcHeaderCount() const { return metadata()->TDCCount; }

	size_t sizeBytes() const { return artdaq_fragment_.sizeBytes(); }

  private:
	artdaq::Fragment const& artdaq_fragment_;
};
}

#endif  // OTSDAQ_FERMILABTESTBEAM_OVERLAYS_WIRECHAMBERFRAGMENT_HH
