#ifndef OTSDAQ_FERMILABTESTBEAM_OVERLAYS_CAMACFRAGMENT_HH
#define OTSDAQ_FERMILABTESTBEAM_OVERLAYS_CAMACFRAGMENT_HH 1

#include <vector>
#include "artdaq-core/Data/Fragment.hh"

namespace ots
{
class CAMACFragment
{
  public:
	const static int LECROY2249A_CHANNELS = 12;
	const static int LECROY2228A_CHANNELS = 8;
	const static int LECROY2323A_CHANNELS = 2;

	const static int JORWAY85A_CHANNELS = 4;

	const static int CAMAC_N_SLOTS = 25;

	const static int ADC_WORD_COUNT    = 12;
	const static int TDC_WORD_COUNT    = 8;
	const static int SCALAR_WORD_COUNT = 8;

	enum class CAMAC_TYPES
	{
		ADC,
		TDC,
		SCALAR,
		INVALID
	};

	struct CAMACMetadata
	{
		uint8_t nADCs;
		uint8_t nTDCs;
		uint8_t nScalars;

		uint8_t slotToIndex[CAMAC_N_SLOTS];

		CAMACMetadata()
		{
			nADCs    = 0;
			nTDCs    = 0;
			nScalars = 0;
			memset(slotToIndex, 0xFF, CAMAC_N_SLOTS);
		}
	};

	struct CCUsbShortData
	{
		uint16_t data;
	};

	struct CCUsbLongData
	{
		uint16_t dataLow;
		uint16_t dataHigh : 8;
		uint16_t dataQX : 8;
	};

	struct ADCDataPoint
	{
		CCUsbShortData data[LECROY2249A_CHANNELS];
	};

	struct TDCDataPoint
	{
		CCUsbShortData data[LECROY2228A_CHANNELS];
	};

	struct ScalarDataPoint
	{
		CCUsbLongData data[JORWAY85A_CHANNELS];
	};

	explicit CAMACFragment(artdaq::Fragment const& f) : artdaq_fragment_(f) {}

	static CAMACMetadata GenerateMetadata(std::vector<int> adcs,
	                                      std::vector<int> tdcs,
	                                      std::vector<int> scalars)
	{
		CAMACMetadata md;
		if(adcs.size() + tdcs.size() + scalars.size() > CAMAC_N_SLOTS)
			return md;
		md.nADCs    = adcs.size();
		md.nTDCs    = tdcs.size();
		md.nScalars = scalars.size();

		int index = 0;
		for(auto& adc : adcs)
		{
			if(adc > CAMAC_N_SLOTS)
				return CAMACMetadata();
			md.slotToIndex[adc] = index;
			index++;
		}
		for(auto& tdc : tdcs)
		{
			if(tdc > CAMAC_N_SLOTS)
				return CAMACMetadata();
			md.slotToIndex[tdc] = index;
			index++;
		}
		for(auto& scalar : scalars)
		{
			if(scalar > CAMAC_N_SLOTS)
				return CAMACMetadata();
			md.slotToIndex[scalar] = index;
			index++;
		}

		return md;
	}

	CAMACMetadata const* metadata() const
	{
		if(!artdaq_fragment_.hasMetadata())
		{
			CAMACMetadata md;
			const_cast<artdaq::Fragment&>(artdaq_fragment_)
			    .setMetadata<CAMACMetadata>(md);
		}
		return artdaq_fragment_.metadata<CAMACMetadata>();
	}

	CAMAC_TYPES SlotToIndex(uint8_t slot, uint8_t& index_out) const
	{
		auto res = metadata()->slotToIndex[slot];
		if(res == 0xFF)
			return CAMAC_TYPES::INVALID;

		if(res < metadata()->nADCs)
		{
			index_out = res;
			return CAMAC_TYPES::ADC;
		}
		res -= metadata()->nADCs;
		if(res < metadata()->nTDCs)
		{
			index_out = res;
			return CAMAC_TYPES::TDC;
		}
		res -= metadata()->nTDCs;
		if(res < metadata()->nScalars)
		{
			index_out = res;
			return CAMAC_TYPES::SCALAR;
		}

		return CAMAC_TYPES::INVALID;
	}

	ADCDataPoint const* getAdcDataByIndex(uint8_t index) const
	{
		if(index >= metadata()->nADCs)
			return nullptr;
		return reinterpret_cast<ADCDataPoint const*>(artdaq_fragment_.dataBegin()) +
		       index;
	}

	TDCDataPoint const* getTdcDataByIndex(uint8_t index) const
	{
		if(index >= metadata()->nTDCs)
			return nullptr;
		auto start = getAdcDataByIndex(metadata()->nADCs - 1) + 1;

		return reinterpret_cast<TDCDataPoint const*>(start) + index;
	}

	ScalarDataPoint const* getScalarDataByIndex(uint8_t index) const
	{
		if(index >= metadata()->nScalars)
			return nullptr;
		auto start = getTdcDataByIndex(metadata()->nTDCs - 1) + 1;
		return reinterpret_cast<ScalarDataPoint const*>(start) + index;
	}

	ADCDataPoint const* getAdcDataBySlot(uint8_t slot) const
	{
		uint8_t index;
		if(SlotToIndex(slot, index) != CAMAC_TYPES::ADC)
			return nullptr;

		return getAdcDataByIndex(index);
	}

	TDCDataPoint const* getTdcDataBySlot(uint8_t slot) const
	{
		uint8_t index;
		if(SlotToIndex(slot, index) != CAMAC_TYPES::TDC)
			return nullptr;

		return getTdcDataByIndex(index);
	}

	ScalarDataPoint const* getScalarDataBySlot(uint8_t slot) const
	{
		uint8_t index;
		if(SlotToIndex(slot, index) != CAMAC_TYPES::SCALAR)
			return nullptr;

		return getScalarDataByIndex(index);
	}

	size_t sizeBytes() const { return artdaq_fragment_.sizeBytes(); }

  private:
	artdaq::Fragment const& artdaq_fragment_;
};
}

#endif  // OTSDAQ_FERMILABTESTBEAM_OVERLAYS_CAMACFRAGMENT_HH
