#include <stdint.h>
#include <iostream>  // std::cout, std::dec, std::hex, std::oct
#include <set>
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-trigger/FEInterfaces/FEPSI46Interface.h"

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "FE-FEPSI46Interface"

//========================================================================================================================
FEPSI46Interface::FEPSI46Interface(const std::string&       interfaceUID,
                                   const ConfigurationTree& theXDAQContextConfigTree,
                                   const std::string&       interfaceConfigurationPath)
    : Socket(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                 .getNode("HostIPAddress")
                 .getValue<std::string>(),
             theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                 .getNode("HostPort")
                 .getValue<unsigned int>())
    , FEOtsUDPTemplateInterface(
          interfaceUID, theXDAQContextConfigTree, interfaceConfigurationPath)
{
}

//========================================================================================================================
FEPSI46Interface::~FEPSI46Interface(void) {}

//========================================================================================================================
void FEPSI46Interface::configure(void)
{
	__CFG_COUT__ << "configure" << std::endl;

	std::string writeBuffer;
	std::string readBuffer;
	uint64_t    readQuadWord;

	ConfigurationTree optionalLink =
	    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	        .getNode("LinkToOptionalParameters");
	bool usingOptionalParams = !optionalLink.isDisconnected();

	////////////////////////////////////////////////////////////////////////////////
	// if clock reset is enabled reset clock
	// MUST BE FIXED ADDING SOFT RESET
	{
		try
		{
			if((usingOptionalParams &&
			    optionalLink.getNode("EnableClockResetDuringConfigure").getValue<bool>()))
			{
				__CFG_COUT__ << "\"Soft\" Resetting NIM PLUS Ethernet!" << std::endl;

				OtsUDPFirmwareCore::ethernetReset(writeBuffer);
				OtsUDPHardware::write(writeBuffer);

				OtsUDPFirmwareCore::ethernetUnreset(writeBuffer);
				OtsUDPHardware::write(writeBuffer);

				// usleep(100000); //micro seconds
				sleep(5);  // seconds
			}
		}
		catch(...)
		{
			__CFG_COUT__ << "Could not find reset clock flag, so not resetting... "
			             << std::endl;
		}
	}

	FEOtsUDPTemplateInterface::configure();  // sets up destination IP/port

	// choose external or internal clock
	__CFG_COUT__ << "Choosing external or internal clock..." << std::endl;
	OtsUDPFirmwareCore::writeAdvanced(
	    writeBuffer,
	    0x3,
	    (usingOptionalParams
	         ? (optionalLink.getNode("UseExternalClock").getValue<bool>() ? 1 : 0)
	         : 0) |
	        (((usingOptionalParams
	               ? (optionalLink.getNode("ExternalClockSource").getValue<unsigned int>()
	                      ? (optionalLink.getNode("ExternalClockSource")
	                             .getValue<unsigned int>() -
	                         1) /*subtract 1 to normal index*/
	                      : 0 /*default to NW-FMC-PTA*/)
	               : 0) &
	          0x7)
	         << 4)  // Choosing external clock source := 1-4 (front panel NIM-input A-D),
	                // 0 (NW-FMC-PTA clk source)

	);  // Choosing external := 1, internal := 0
	OtsUDPHardware::write(writeBuffer);
	usleep(100000);  // micro seconds

	{  // debug read
		// read NIM+ version (for debugging)
		OtsUDPFirmwareCore::readAdvanced(writeBuffer,
		                                 0x5);  // This can be removed when you want
		OtsUDPHardware::read(writeBuffer,
		                     readBuffer);  // This can be removed when you want

		// read back clock lock loss status
		OtsUDPFirmwareCore::readAdvanced(writeBuffer, 0x10);
		OtsUDPHardware::read(writeBuffer, readQuadWord);
		__CFG_COUT__ << "Clocks lock loss " << ((readQuadWord >> 24) & 0xF) << __E__;

		// if clock lock was lost at some point, reset DCMs here
		//	and check clock lock again
		if(!((readQuadWord >> 24) & 0xF))
		{
			__CFG_COUT__ << "Re-locking clocks..." << std::endl;
			// reset clock PLLs
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer,
			    /*address*/ 0x999,
			    /*data*/ 0x7);  // reset wiz0, wiz1, and nimDacClk
			OtsUDPHardware::write(writeBuffer);
			usleep(100000);  // micro seconds
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer, /*address*/ 0x999, /*data*/ 0);  // unreset
			OtsUDPHardware::write(writeBuffer);
			usleep(100000);  // micro seconds
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer,
			    /*address*/ 0x999,
			    /*data*/ 0x8);  // reset phase shift output clock
			OtsUDPHardware::write(writeBuffer);
			usleep(100000);  // micro seconds
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer, /*address*/ 0x999, /*data*/ 0);  // unreset
			OtsUDPHardware::write(writeBuffer);
			usleep(100000);  // micro seconds

			// read back clock lock loss status
			OtsUDPFirmwareCore::readAdvanced(writeBuffer, 0x10);
			OtsUDPHardware::read(writeBuffer, readQuadWord);
			__CFG_COUT__ << "Clocks lock loss " << ((readQuadWord >> 24) & 0xF) << __E__;
		}

		// read NIM+ version (for debugging)
		OtsUDPFirmwareCore::readAdvanced(writeBuffer,
		                                 0x5);  // This can be removed when you want
		OtsUDPHardware::read(writeBuffer,
		                     readBuffer);  // This can be removed when you want
	}

	// Run Configure Sequence Commands
	// runSequenceOfCommands("LinkToConfigureSequence");

	// disable all 3 muxable trigger channels
	// always do this to avoid stray output triggers during configure
	if(1)
	{
		// addresses sig_norm and 2 veto channeling
		// write 3 to disable and reset

		writeBuffer.resize(0);
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, /*address*/ 0x4, /*data*/ 0x3);  // disable sig norm
		OtsUDPHardware::write(writeBuffer);
		writeBuffer.resize(0);
		OtsUDPFirmwareCore::writeAdvanced(writeBuffer, 0x18016, 0x3);  // disable cms1
		OtsUDPHardware::write(writeBuffer);
		writeBuffer.resize(0);
		OtsUDPFirmwareCore::writeAdvanced(writeBuffer, 0x18017, 0x3);  // disable cms2
		OtsUDPHardware::write(writeBuffer);
		writeBuffer.resize(0);
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x5, 0x0);  // chA output mux to sig_norm
		OtsUDPHardware::write(writeBuffer);
		writeBuffer.resize(0);
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x18013, 0x0);  // chB output mux to sig_norm
		OtsUDPHardware::write(writeBuffer);
		writeBuffer.resize(0);
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x18014, 0x0);  // chC output mux to sig_norm
		OtsUDPHardware::write(writeBuffer);
		writeBuffer.resize(0);
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x18015, 0x0);  // chD output mux to sig_norm
		OtsUDPHardware::write(writeBuffer);

		writeBuffer.resize(0);
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x1800C, 0xF);  // setup output polarity
		OtsUDPHardware::write(writeBuffer);

		writeBuffer.resize(0);
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x6, 0x0);  // disable AND gate selection logic
		OtsUDPHardware::write(writeBuffer);
	}

	// Reset everything for configure
	nimEnables_.reset();  // set all bits to 0
	nimResets_.set();     // set all bits to 1
	writeBuffer.resize(0);
	OtsUDPFirmwareCore::writeAdvanced(
	    writeBuffer,
	    0x18000,
	    nimResets_.to_ulong());  // reset everything (counters and vetos/ps)
	OtsUDPHardware::write(writeBuffer);
	nimResets_.reset();  // set all bits to 0
	writeBuffer.resize(0);
	OtsUDPFirmwareCore::writeAdvanced(
	    writeBuffer, 0x18000, nimResets_.to_ulong());  // unreset everything
	OtsUDPHardware::write(writeBuffer);

	std::array<std::string, 4> channelNames(
	    {"ChannelA", "ChannelB", "ChannelC", "ChannelD"});
	// set up DACs
	bool doWriteDACs = false;
	try
	{
		doWriteDACs =
		    usingOptionalParams &&
		    optionalLink.getNode("EnableDACSetupDuringConfigure").getValue<bool>();
	}
	catch(...)
	{
		__CFG_COUT__ << "Skipping DAC writing because enable field was not found in tree."
		             << std::endl;
	}

	if(doWriteDACs)  // works, but temporarily disabling
	{
		__CFG_COUT__ << "Setting up DACs" << std::endl;

		// 16 bit DAC value
		//   - 15:12 channel := 2=A, 6=B, A=C, E=D
		//   - 11:0 value  := 0-3.3V

		std::string   dacValueField     = "DACValue";
		unsigned char dacChannelAddress = 2;
		for(const auto& channelName : channelNames)
		{
			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(writeBuffer, /*address*/ 0x1, /*data*/ 0x0);
			OtsUDPHardware::write(writeBuffer);
			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(writeBuffer, /*address*/ 0x1, /*data*/ 0x0);
			OtsUDPHardware::write(writeBuffer);

			unsigned short dacValue = optionalLink.getNode(dacValueField + channelName)
			                              .getValue<unsigned short>();
			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer,
			    /*address*/ 0x0,
			    /*data*/ (dacChannelAddress << 12) | (dacValue & 0xFFF));
			OtsUDPHardware::write(writeBuffer);
			dacChannelAddress += 4;

			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(writeBuffer, /*address*/ 0x1, /*data*/ 0x2);
			OtsUDPHardware::write(writeBuffer);
			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(writeBuffer, /*address*/ 0x1, /*data*/ 0x0);
			OtsUDPHardware::write(writeBuffer);
			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(writeBuffer, /*address*/ 0x1, /*data*/ 0x4);
			OtsUDPHardware::write(writeBuffer);
			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(writeBuffer, /*address*/ 0x1, /*data*/ 0x0);
			OtsUDPHardware::write(writeBuffer);
			// sleep(1); // give a little time to the slow DAC ASIC write
		}
	}

	// setup sig_mod channels, the input delay and width stages and sig_log, and other
	// pre-sig_norm things
	try
	{
		unsigned char channelCount = 0;
		bool          enableInput, invertPolarity;
		uint64_t      inputModMask;
		unsigned int  inputDelay;
		unsigned int  inputWidth;

		unsigned char selectionOnOffMask = 0, inputPolarityMask = 0;

		__CFG_COUT__ << "Setting up input channels..." << std::endl;
		for(const auto& channelName : channelNames)  // setup sig mod for each channel
		{
			// if(channelName != "ChannelD") { ++channelCount; continue;} //For Debugging
			// just one channel

			enableInput = theXDAQContextConfigTree_.getNode(theConfigurationPath_)
			                  .getNode("EnableInput" + channelName)
			                  .getValue<bool>();

			if(usingOptionalParams)
			{
				inputDelay = optionalLink.getNode("DelayInput" + channelName)
				                 .getValue<unsigned int>();
				inputWidth = optionalLink.getNode("WidthInput" + channelName)
				                 .getValue<unsigned int>();
				inputModMask = (0xFFFFFFFFFFFFFFFF >> (64 - inputWidth)) << inputDelay;
			}
			else
			{
				inputModMask = 0x7;  // b111 default value
			}

			selectionOnOffMask |= ((enableInput ? 1 : 0) << channelCount);

			invertPolarity = usingOptionalParams &&
			                 optionalLink.getNode("InvertPolarityInput" + channelName)
			                     .getValue<bool>();

			inputPolarityMask |= ((invertPolarity ? 1 : 0) << channelCount);

			__CFG_COUT__ << "Output word for " << channelName << " is "
			             << std::bitset<64>(inputModMask) << std::endl
			             << " with a delay of " << inputDelay << " and a width of "
			             << inputWidth << std::endl;

			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(writeBuffer,
			                                  /*address*/ ((channelCount + 1) << 8),
			                                  /*data*/ 0x3);  // reset channel
			OtsUDPHardware::write(writeBuffer);

			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer,
			    ((channelCount + 1) << 8) | 0x2,
			    inputModMask);  // set sig_mod width and delay
			OtsUDPHardware::write(writeBuffer);

			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer,
			    ((channelCount + 1) << 8) | 0x4,
			    enableInput ? 0 : 1);  // enable/disable channel
			OtsUDPHardware::write(writeBuffer);

			++channelCount;
		}
		__CFG_COUT__ << "Input polariy mask is " << std::bitset<8>(inputPolarityMask)
		             << std::endl;
		writeBuffer.resize(0);
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer,
		    /*address*/ 0x1800B,
		    /*data*/ inputPolarityMask);  // setup input polarity
		OtsUDPHardware::write(writeBuffer);

		__CFG_COUT__ << "Writing accelerator clock mask." << __E__;
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer,
		    0x107,
		    (usingOptionalParams
		         ? optionalLink.getNode("AcceleratorClockMask").getValue<unsigned int>()
		         : 0));  // chooses a 40MHz clock phase relative to the accelerator clock
		                 // (in increments of ~3ns)
		OtsUDPHardware::write(writeBuffer);

		__CFG_COUT__ << "Writing trigger clock mask." << __E__;
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer,
		    0x18008,
		    (1 << 8) |  // disable sig_log masking with 40MHz clock block
		        (usingOptionalParams
		             ? optionalLink.getNode("TriggerClockMask").getValue<unsigned int>()
		             : 0));  // chooses a section of 40MHz clock
		OtsUDPHardware::write(writeBuffer);

		// 40Mhz Clock Stuff - Update to reflect firmware changes
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x18000, 0x40);  // resets section of 40MHz clock block
		OtsUDPHardware::write(writeBuffer);

		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x18000, 0x0);  // enables a section of 40MHz clock block
		OtsUDPHardware::write(writeBuffer);
	}
	catch(const std::runtime_error& e)
	{
		__CFG_COUT__ << "Failed input stage setup!\n" << e.what() << std::endl;
		throw;
	}

	// setup output stage
	try
	{
		unsigned char channelCount = 0;
		bool          enableOutput;
		unsigned int  outputDelay;
		unsigned int  outputWidthMask;  // max is 64 bit mask
		uint64_t      outputWidth;
		uint64_t      outputModMask;

		unsigned int  outputMuxSelect;
		unsigned int  outputChannelSourceSelect;
		unsigned int  outputTimeVetoDuration, outputPrescaleCount;
		bool          outputBackpressureSelect;
		unsigned char backpressureMask      = 0;
		unsigned int  gateChannelVetoSel[3] = {0, 0, 0};

		__CFG_COUT__ << "Setting up output channels..." << std::endl;
		// there are 3 output channels (alias: signorm, sigcms1, sigcms2)
		std::array<std::string, 3> outChannelNames = {"Channel0", "Channel1", "Channel2"};
		for(const auto& channelName : outChannelNames)
		{
			outputChannelSourceSelect =
			    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
			        .getNode("OutputSourceSelect" + channelName)
			        .getValue<unsigned int>();  // 0: sig_log   or    1: sig_norm/ch0
			__CFG_COUT__ << "OutputSourceSelect for " << channelName << " is "
			             << outputChannelSourceSelect << std::endl;
			if(outputChannelSourceSelect)  // if non-default, subtract 1 so choice 1
			                               // evaluates to 0, and so on..
				--outputChannelSourceSelect;

			if(usingOptionalParams)
			{
				outputDelay = optionalLink.getNode("DelayOutput" + channelName)
				                  .getValue<unsigned int>();
				outputWidth = optionalLink.getNode("WidthOutput" + channelName)
				                  .getValue<uint64_t>();
				if(outputWidth > 64)
					outputWidthMask = 64;
				else
				{
					outputWidthMask = outputWidth;
					outputWidth     = 0;
				}
				outputModMask = (0xFFFFFFFFFFFFFFFF >> (64 - outputWidthMask))
				                << outputDelay;

				outputTimeVetoDuration =
				    optionalLink.getNode("OutputTimeVetoDuration" + channelName)
				        .getValue<unsigned int>();  // 0 ignores time veto, units of 3ns
				                                    // (NIM+X) or 6ns (NIM+)
				outputPrescaleCount =
				    optionalLink.getNode("OutputPrescaleCount" + channelName)
				        .getValue<unsigned int>();
				outputBackpressureSelect =
				    optionalLink.getNode("OutputBackpressureSelect" + channelName)
				        .getValue<bool>();
				gateChannelVetoSel[channelCount] =
				    optionalLink.getNode("InputChannelVetoSourceForOutput" + channelName)
				        .getValue<int>();
				__CFG_COUT__ << "Raw gateChannelVetoSelect for " << channelName << " is "
				             << gateChannelVetoSel[channelCount] << std::endl;
				// 0/1 := No Veto, 2-5 := Input_A-D
			}
			else  // defaults
			{
				outputModMask            = 0xFFF;
				outputWidth              = 0;
				outputTimeVetoDuration   = 0;
				outputPrescaleCount      = 0;
				outputBackpressureSelect = false;
			}

			if(gateChannelVetoSel[channelCount] <= 1)
				gateChannelVetoSel[channelCount] = 4;  // Ground on mux
			else
				gateChannelVetoSel[channelCount] -= 2;  // 0-3 on mux is chA-D

			backpressureMask |= (outputBackpressureSelect ? 1 : 0) << channelCount;

			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer,
			    channelCount == 0 ? 0x4 : (0x18016 + channelCount - 1),
			    0x33);  // reset output channel block (bits 4/5 are reseting ch1/2)
			OtsUDPHardware::write(writeBuffer);

			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer,
			    channelCount == 0 ? 0x2 : (0x18002 + channelCount - 1),
			    outputModMask);  // set output channel width/delay mask
			OtsUDPHardware::write(writeBuffer);

			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer,
			    0x106 + channelCount * 0x100,
			    outputWidth | (uint64_t(outputWidth ? 1 : 0 /*enable*/)
			                   << 63));  // set output channel long width count
			OtsUDPHardware::write(writeBuffer);

			__CFG_COUT__ << "Output word for " << channelName << " is "
			             << std::bitset<64>(outputModMask) << std::endl;
			__CFG_COUT__ << "Output delay of " << outputDelay
			             << " and an extended width of " << outputWidth << std::endl;

			if(channelCount)
			{
				writeBuffer.resize(0);
				OtsUDPFirmwareCore::writeAdvanced(
				    writeBuffer,
				    (0x18018 + channelCount - 1),
				    outputChannelSourceSelect);  // select source (1 := signorm, 0 :=
				                                 // siglog)
				OtsUDPHardware::write(writeBuffer);
				__CFG_COUT__ << "Output src select is " << outputChannelSourceSelect
				             << " for " << channelName << std::endl;
			}

			// time veto setup
			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer,
			    channelCount == 0 ? 0x1801B : (0x18011 + channelCount - 1),
			    outputTimeVetoDuration);
			OtsUDPHardware::write(writeBuffer);

			__CFG_COUT__ << "Veto count for " << channelName << " is "
			             << outputTimeVetoDuration << " writing to ch "
			             << (channelCount == 0 ? 0x1801B : (0x18011 + channelCount - 1))
			             << std::endl;

			// prescale veto setup
			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer, 0x1801C + channelCount, 0);  // Set to 0, then set to value
			OtsUDPHardware::write(writeBuffer);
			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer, 0x1801C + channelCount, outputPrescaleCount);
			OtsUDPHardware::write(writeBuffer);

			__CFG_COUT__ << "Prescaler count for " << channelName << " is "
			             << outputPrescaleCount << " writing to ch "
			             << 0x1801C + channelCount << std::endl;

			++channelCount;
		}

		// backpressure select
		// bit 0, 1, 2 mean apply backpressure to that output channel
		//  2 external backpressure inputs
		//  bit 3 is 0/1 disable/enable for first input
		//  bit 4 is 0/1 disable/enable for second input
		__CFG_COUT__ << "Backpressure Selecting..." << std::endl;
		outputBackpressureSelect =
		    usingOptionalParams &&
		    optionalLink.getNode("EnableBackPressureInputA").getValue<bool>();
		backpressureMask |= outputBackpressureSelect << 3;
		outputBackpressureSelect =
		    usingOptionalParams &&
		    optionalLink.getNode("EnableBackPressureInputB").getValue<bool>();
		backpressureMask |= outputBackpressureSelect << 4;
		writeBuffer.resize(0);
		OtsUDPFirmwareCore::writeAdvanced(writeBuffer, 0x1801A, backpressureMask);
		OtsUDPHardware::write(writeBuffer);

		// force output clk/trig bank to D by default
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x200, (uint64_t)-1);  // setup burst output mux select
		OtsUDPHardware::write(writeBuffer);

		// and 4 output muxes (first is special)
		__CFG_COUT__ << "Setting output muxes..." << std::endl;
		unsigned int outputPolarityMask = 0;
		bool         outputInvertPolarity;
		channelCount = 0;
		for(const auto& channelName : channelNames)
		{
			outputMuxSelect = theXDAQContextConfigTree_.getNode(theConfigurationPath_)
			                      .getNode("OutputMuxSelect" + channelName)
			                      .getValue<unsigned int>();
			if(outputMuxSelect)
				--outputMuxSelect;  // default is 0, the actual selection address for all
				                    // other choices is 1 higher

			if(outputMuxSelect > 31)
			{
				__SS__;
				throw std::runtime_error(ss.str() + "Invalid output mux select!");
			}

			if(usingOptionalParams)
			{
				outputInvertPolarity =
				    usingOptionalParams &&
				    optionalLink.getNode("InvertPolarityOutput" + channelName)
				        .getValue<bool>();
				outputPolarityMask |= ((outputInvertPolarity ? 1 : 0) << channelCount);
			}
			else
			{
				outputPolarityMask = 0xF;
			}

			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer,
			    channelCount == 0 ? 0x5 : (0x18013 + channelCount - 1),
			    outputMuxSelect);  // setup mux select
			OtsUDPHardware::write(writeBuffer);
			__CFG_COUT__ << "Mux value for output channel " << channelName << " is "
			             << outputMuxSelect << ", written to 0x" << std::hex
			             << (channelCount == 0 ? 0x5 : (0x18013 + channelCount - 1))
			             << std::dec << std::endl;

			++channelCount;
		}

		writeBuffer.resize(0);
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer,
		    /*address*/ 0x1800C,
		    /*data*/ outputPolarityMask);  // setup output polarity
		OtsUDPHardware::write(writeBuffer);
		__CFG_COUT__ << "Output polarity mask is " << std::bitset<8>(outputPolarityMask)
		             << std::endl;

		// setting up trigger/clk output banks A and B
		if(usingOptionalParams)
		{
			// trigger/clock output banks are controlled by a single
			//	64-bit register at address 0x200
			//
			//	there are 12x 53MHz trigger channels
			//	and 6x 40MHz trigger channels
			//
			//	..each controlled by 2 bits to choose mux output channels 0-3
			//
			//	this is a total of 18*2=36 control bits.. so 0 to 35 are used
			//
			//	In the configuration, we organize by how the panel was assuembled
			//	which is two banks of 9 like so:
			//		* 0-5 are 53MHz
			//		* 6-8 are 40MHz

			uint64_t     trigClkOutBankRegister = 0;
			unsigned int bank                   = 0;

			std::array<std::string, 2> trigClkOutBankNames = {"TrigerMuxSelectionsBankA",
			                                                  "TrigerMuxSelectionsBankB"};
			for(auto& trigClkOutBankName : trigClkOutBankNames)
			{
				__COUTV__(trigClkOutBankName);

				ConfigurationTree::BitMap bankSelectionMap =
				    optionalLink.getNode(trigClkOutBankName).getValueAsBitMap();

				// 0-5 are 53MHz
				// 6-8 are 40MHz

				unsigned int col = 0;
				for(; col < 6; ++col)
					trigClkOutBankRegister |=
					    (bankSelectionMap.get(0, col) & 0x3)
					    << ((bank * 6 + col) *
					        2);  // put the two bit mux selection in the right place

				for(; col < 9; ++col)
					trigClkOutBankRegister |=
					    (bankSelectionMap.get(0, col) & 0x3)
					    << ((12 + bank * 3 + col) *
					        2);  // put the two bit mux selection in the right place

				++bank;
			}

			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer,
			    0x200,
			    trigClkOutBankRegister);  // setup burst output mux select
			OtsUDPHardware::write(writeBuffer);
		}

		// setup burst data blocks
		{
			__CFG_COUT__ << "Setting up Burst Data Blocks" << std::endl;

			unsigned int logicSampleDelay = 0;
			unsigned int gateChannel      = 0;
			unsigned int gateChannelReg =
			    (gateChannelVetoSel[2] << 8) | (gateChannelVetoSel[1] << 4) |
			    (gateChannelVetoSel[0] << 0);  // nibbles ... 3:= delta-time, 2:= out-ch2,
			                                   // 1:= out-ch1, 0 := out-ch0
			__CFG_COUT__ << "Gate Ch Veto Selections - 2: " << gateChannelVetoSel[2]
			             << std::endl;
			__CFG_COUT__ << "Gate Ch Veto Selections - 1: " << gateChannelVetoSel[1]
			             << std::endl;
			__CFG_COUT__ << "Gate Ch Veto Selections - 0: " << gateChannelVetoSel[0]
			             << std::endl;
			// value of 4 is no-gate
			// 0-3 are input channels A-D depending on polarity

			if(usingOptionalParams)
			{
				outputMuxSelect =
				    optionalLink.getNode("BurstDataMuxSelect").getValue<unsigned int>();
				logicSampleDelay = optionalLink.getNode("BurstDataLogicSampleDelay")
				                       .getValue<unsigned int>();
				gateChannel = optionalLink.getNode("BurstDataGateInputChannel")
				                  .getValue<unsigned int>();
			}
			else
				outputMuxSelect = 0;

			if(outputMuxSelect)  // if non-default, subtract 1 so choice 1 evaluates to 0,
			                     // and so on..
				--outputMuxSelect;

			if(gateChannel >
			   1)  // if non-default, subtract 1 so choice 1 evaluates to 0, and so on..
			{
				gateChannel -= 2;  // to give range 0 to 3
				gateChannelReg |= gateChannel << 12;
			}
			else  // default is no gate
			{
				gateChannelReg |= 4 << 12;
			}
			__CFG_COUT__ << "Gate Ch Veto Selections - Burst: " << gateChannel
			             << std::endl;
			std::bitset<16> gateChannelRegBitset(gateChannelReg);
			__CFG_COUT__ << "Gate Ch Veto Register: " << gateChannelReg << std::endl;
			__CFG_COUT__ << "Gate Ch Veto Register: " << gateChannelRegBitset.to_string()
			             << std::endl;

			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer, 0x1800E, outputMuxSelect);  // setup burst output mux select
			OtsUDPHardware::write(writeBuffer);

			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer,
			    0x18010,
			    logicSampleDelay);  // setup burst logic sample delay
			OtsUDPHardware::write(writeBuffer);

			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer,
			    0x18004,
			    gateChannelReg);  // setup burst block gate signal choice
			OtsUDPHardware::write(writeBuffer);
		}

		// selection logic setup
		unsigned int coincidenceLogicWord =
		    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
		        .getNode("CoincidenceLogicWord")
		        .getValue<unsigned int>();

		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x06, 0x01);  // reset selection logic
		OtsUDPHardware::write(writeBuffer);

		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x06, 0x00);  // disable selection logic, take out of reset
		OtsUDPHardware::write(writeBuffer);

		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x07, coincidenceLogicWord);  // setup selection logic
		OtsUDPHardware::write(writeBuffer);

		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x06, 0x02);  // renable selection  logic
		OtsUDPHardware::write(writeBuffer);
		__CFG_COUT__ << "Selection Logic word is  "
		             << std::bitset<16>(coincidenceLogicWord) << std::endl;

		unsigned int sigGenCount =
		    optionalLink.getNode("SignalGeneratorPulseCount").getValue<unsigned int>();
		unsigned int sigGenHighPer =
		    optionalLink.getNode("SignalGeneratorHighPeriod").getValue<unsigned int>();
		unsigned int sigGenLowPer =
		    optionalLink.getNode("SignalGeneratorLowPeriod").getValue<unsigned int>();
		bool sigGenPolarity =
		    optionalLink.getNode("SignalGeneratorInvertPolarity").getValue<bool>();
		unsigned int sigGenPolarityMask = (sigGenPolarity ? 1 : 0);

		if(optionalLink.getNode("SignalGeneratorEnable").getValue<bool>())
		{
			nimResets_.reset(5);  // set bit 5 in resets to 0 to take sig gen out of reset
			nimEnables_.set(5);   // set bit 5 in enables to 1 to enable sig gen

			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer, 0x18000, 0xFFFF);  // reset all (is this safe to do?)
			OtsUDPHardware::write(writeBuffer);
			__CFG_COUT__ << "Resets all for sig gen!" << std::endl;

			// signal generator setup
			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer, 0x18005, sigGenCount);  // sig gen pulse count
			OtsUDPHardware::write(writeBuffer);
			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer, 0x18006, sigGenHighPer);  // sig gen high per
			OtsUDPHardware::write(writeBuffer);
			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer, 0x18007, sigGenLowPer);  // sig gen low per
			OtsUDPHardware::write(writeBuffer);
			writeBuffer.resize(0);
			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer, 0x1800D, sigGenPolarityMask);  // sig gen polarity
			OtsUDPHardware::write(writeBuffer);

			__CFG_COUT__ << "Configured signal generator with a count of " << sigGenCount
			             << " (0 is continuous output), a high period of "
			             << sigGenHighPer << ", a low period of " << sigGenLowPer
			             << ", and output inversion set to " << sigGenPolarity
			             << std::endl;
		}
		else
		{
			nimResets_.set(5);  // set bit 5 in resets to 1 to take sig gen out of reset
			nimEnables_.reset(5);  // set bit 5 in enables to 0 to disable sig gen
			__CFG_COUT__ << "Signal Generator disabled" << std::endl;
		}

		writeBuffer.resize(0);
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer,
		    0x18000,
		    nimResets_.to_ulong());  // set sig gen in or out of reset
		OtsUDPHardware::write(writeBuffer);
		__CFG_COUT__ << "Nim Resets set to " << nimResets_ << std::endl;

		writeBuffer.resize(0);
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x18001, nimEnables_.to_ulong());  // enable or disable sig gen
		OtsUDPHardware::write(writeBuffer);
		__CFG_COUT__ << "Nim Enables set to " << nimEnables_ << std::endl;
	}
	catch(const std::runtime_error& e)
	{
		__CFG_COUT__ << "Failed output stage setup!\n" << e.what() << std::endl;
		throw;
	}

	// now that configure done, save sel_ctl_register_ for later
	OtsUDPFirmwareCore::readAdvanced(writeBuffer, 10 /*address*/);  // read back sig log
	OtsUDPHardware::read(writeBuffer, sel_ctl_register_);

	__CFG_COUT__ << "receiveQuadWord all = 0x" << std::hex << sel_ctl_register_
	             << std::dec << std::endl;

	sel_ctl_register_ = ((sel_ctl_register_ >> (5 * 8)) & 0x0FF);

	__CFG_COUT__ << "sel_ctl_register_ = 0x" << std::hex << sel_ctl_register_ << std::dec
	             << std::endl;

	// at this point sig_log should be active (for chipscope, and recognizing of trigger
	// input active)

	__CFG_COUT__ << "Done with configuring." << std::endl;
}

//========================================================================================================================
void FEPSI46Interface::halt(void)
{
	__CFG_COUT__ << "\tHalt" << std::endl;
	stop();
}

//========================================================================================================================
void FEPSI46Interface::pause(void)
{
	__CFG_COUT__ << "\tPause" << std::endl;
	stop();
}

//========================================================================================================================
void FEPSI46Interface::resume(void)
{
	__CFG_COUT__ << "\tResume" << std::endl;
	start("");
}

//========================================================================================================================
void FEPSI46Interface::start(std::string runNumber)
{
	runNumber_ = runNumber;
	__CFG_COUT__ << "\tStart " << runNumber_ << std::endl;
	std::string writeBuffer;

	// Run Start Sequence Commands
	// runSequenceOfCommands("LinkToStartSequence");

	ConfigurationTree optionalLink =
	    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	        .getNode("LinkToOptionalParameters");
	bool usingOptionalParams = !optionalLink.isDisconnected();

	if(usingOptionalParams && optionalLink.getNode("EnableBurstData").getValue<bool>())
	{
		__CFG_COUT__ << "Enabling burst mode!" << __E__;
		OtsUDPFirmwareCore::startBurst(writeBuffer);
		OtsUDPHardware::write(writeBuffer);
	}

	// enable nim plus burst data
	OtsUDPFirmwareCore::writeAdvanced(writeBuffer, /*address*/ 0x1801F, /*data*/ 0x6);
	OtsUDPHardware::write(writeBuffer);
}

//========================================================================================================================
void FEPSI46Interface::stop(void)
{
	std::string writeBuffer;
	// immediately stop triggers (by disabling sig log)
	OtsUDPFirmwareCore::writeAdvanced(
	    writeBuffer,
	    0x6 /*address*/,
	    (sel_ctl_register_) & (~(1 << 1)));  // disable sig mod block
	OtsUDPHardware::write(writeBuffer);

	__CFG_COUT__ << "\tStop" << std::endl;

	// Run Stop Sequence Commands
	runSequenceOfCommands("LinkToStopSequence");

	// attempt to stop burst always
	OtsUDPFirmwareCore::stopBurst(writeBuffer);
	OtsUDPHardware::write(writeBuffer);

	uint64_t readQuadWord;

	ConfigurationTree optionalLink =
	    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	        .getNode("LinkToOptionalParameters");
	if(!optionalLink.isDisconnected())
	{
		std::string
		    filename =  // theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode(
		    optionalLink.getNode("TriggerCountAtRunStopFilename").getValue<std::string>();

		if(filename != "DEFAULT" && filename != "")
		{
			std::string filename = optionalLink.getNode("TriggerCountAtRunStopFilename")
			                           .getValue<std::string>();

			if(filename != "DEFAULT" && filename != "")
			{
				filename += "_" + runNumber_ + ".cnt";

				__CFG_COUT__ << "Attempting to save counts to " << filename << __E__;
				FILE* fp =
				    fopen(  //("/data/TestBeam/2017_12_December/NimPlus/TriggerCount_" +
				            // runNumber_ + ".cnt").c_str()
				        filename.c_str(),
				        "w");
				if(fp)
				{
					__CFG_COUT__ << "Saving counts to " << filename << __E__;
					// std::string readBuffer;

					uint32_t count;
					uint8_t  tag;

					OtsUDPFirmwareCore::readAdvanced(writeBuffer, 0x102);
					OtsUDPHardware::read(writeBuffer, readQuadWord);

					count = (readQuadWord >> 32);
					tag   = count >> 28;           // top 4 bits
					count = (count & 0x0FFFFFFF);  // only 28 bits
					__CFG_COUT__ << "sig_log count = " << count << __E__;
					fprintf(fp, "sig_log   \t [tag=%d] %d 0x%4.4X\n", tag, count, count);

					count = (readQuadWord & 0x0FFFFFFFF);
					tag   = count >> 28;           // top 4 bits
					count = (count & 0x0FFFFFFF);  // only 28 bits
					__CFG_COUT__ << "sig_norm(out0) count = " << count << __E__;
					fprintf(
					    fp, "sig_norm(out0) \t [tag=%d] %d 0x%4.4X\n", tag, count, count);

					OtsUDPFirmwareCore::readAdvanced(writeBuffer, 0x100);
					OtsUDPHardware::read(writeBuffer, readQuadWord);

					count = (readQuadWord & 0x0FFFFFFFF);
					tag   = count >> 28;           // top 4 bits
					count = (count & 0x0FFFFFFF);  // only 28 bits
					__CFG_COUT__ << "sig_cms1(out1) count = " << count << __E__;
					fprintf(
					    fp, "sig_cms1(out1) \t [tag=%d] %d 0x%4.4X\n", tag, count, count);

					count = (readQuadWord >> 32);
					tag   = count >> 28;           // top 4 bits
					count = (count & 0x0FFFFFFF);  // only 28 bits
					__CFG_COUT__ << "sig_cms2(out2) count = " << count << __E__;
					fprintf(
					    fp, "sig_cms2(out2) \t [tag=%d] %d 0x%4.4X\n", tag, count, count);

					OtsUDPFirmwareCore::readAdvanced(writeBuffer, 0x105);
					OtsUDPHardware::read(writeBuffer, readQuadWord);

					count = (readQuadWord & 0x0FFFFFFFF);
					tag   = count >> 28;           // top 4 bits
					count = (count & 0x0FFFFFFF);  // only 28 bits
					__CFG_COUT__ << "muxout-A count = " << count << __E__;
					fprintf(fp, "muxout-A \t [tag=%d] %d 0x%4.4X\n", tag, count, count);

					count = (readQuadWord >> 32);
					tag   = count >> 28;           // top 4 bits
					count = (count & 0x0FFFFFFF);  // only 28 bits
					__CFG_COUT__ << "muxout-B count = " << count << __E__;
					fprintf(fp, "muxout-B \t [tag=%d] %d 0x%4.4X\n", tag, count, count);

					OtsUDPFirmwareCore::readAdvanced(writeBuffer, 0x106);
					OtsUDPHardware::read(writeBuffer, readQuadWord);

					count = (readQuadWord & 0x0FFFFFFFF);
					tag   = count >> 28;           // top 4 bits
					count = (count & 0x0FFFFFFF);  // only 28 bits
					__CFG_COUT__ << "muxout-C count = " << count << __E__;
					fprintf(fp, "muxout-C \t [tag=%d] %d 0x%4.4X\n", tag, count, count);

					count = (readQuadWord >> 32);
					tag   = count >> 28;           // top 4 bits
					count = (count & 0x0FFFFFFF);  // only 28 bits
					__CFG_COUT__ << "muxout-D count = " << count << __E__;
					fprintf(fp, "muxout-D \t [tag=%d] %d 0x%4.4X\n", tag, count, count);

					// check for clock loss
					OtsUDPFirmwareCore::readAdvanced(writeBuffer, 0x10);
					OtsUDPHardware::read(writeBuffer, readQuadWord);

					__CFG_COUT__ << "Clocks lock loss " << count << __E__;

					count = (((readQuadWord >> 24) & 0xF) >> 0) & 1;
					fprintf(
					    fp, "DAC       Clock-Lock-Loss \t %d 0x%4.4X\n", count, count);
					count = (((readQuadWord >> 24) & 0xF) >> 1) & 1;
					fprintf(
					    fp, "Main      Clock-Lock-Loss \t %d 0x%4.4X\n", count, count);
					count = (((readQuadWord >> 24) & 0xF) >> 2) & 1;
					fprintf(
					    fp, "External  Clock-Lock-Loss \t %d 0x%4.4X\n", count, count);
					count = (((readQuadWord >> 24) & 0xF) >> 3) & 1;
					fprintf(
					    fp, "OutPhase  Clock-Lock-Loss \t %d 0x%4.4X\n", count, count);

					fclose(fp);
				}
			}
		}
	}

	// there are 3 output channels (alias: signorm, sigcms1, sigcms2)

	writeBuffer.resize(0);
	OtsUDPFirmwareCore::writeAdvanced(
	    writeBuffer, /*address*/ 0x4, /*data*/ 0x33);  // only reset signorm  0x33);
	                                                   // //reset output channel blocks
	                                                   // synchronously
	OtsUDPHardware::write(writeBuffer);
}

//========================================================================================================================
bool FEPSI46Interface::running(void)
{
	std::string writeBuffer;

	__CFG_COUT__ << "Disabling sig_log" << std::endl;

	OtsUDPFirmwareCore::writeAdvanced(
	    writeBuffer,
	    0x6 /*address*/,
	    (sel_ctl_register_) & (~(1 << 1)));  // disable sig mod block
	OtsUDPHardware::write(writeBuffer);

	__CFG_COUT__ << "Resetting all counters (including sig log)" << std::endl;
	// 0x18000 ==> counter resets

	OtsUDPFirmwareCore::writeAdvanced(
	    writeBuffer,
	    0x18000 /*address*/,
	    (-1) & (~(1 << 6)) /*data*/);  // reset sig_log and sig_norm/cms1/cms2 counters
	                                   // (just not bit 6 which is acc_sync block reset)
	OtsUDPHardware::write(writeBuffer);

	OtsUDPFirmwareCore::writeAdvanced(
	    writeBuffer, 0x18000 /*address*/, 0 /*data*/);  // unreset sig_log and
	                                                    // sig_norm/cms1/cms2 counters
	                                                    // (just not bit 6 which is
	                                                    // acc_sync block reset)
	OtsUDPHardware::write(writeBuffer);

	////////////////////////////////
	////////////////////////////////
	// long sleep so trigger numbers match
	// sleep(22);
	// sleep(1);

	__CFG_COUT__ << "Running" << std::endl;

	//		//example!
	//		//play with array of 8 LEDs at address 0x1003

	ConfigurationTree optionalLink =
	    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	        .getNode("LinkToOptionalParameters");
	bool usingOptionalParams = !optionalLink.isDisconnected();

	// 0x0/0x8 to 0x4 to use edge detection
	try
	{
		unsigned int sleepSeconds = 0;
		if(usingOptionalParams)
		{
			try
			{
				sleepSeconds = optionalLink.getNode("SecondsDelayBeforeStartingTriggers")
				                   .getValue<unsigned int>();
			}
			catch(...)
			{
				__CFG_COUT__
				    << "Ingore missing SecondsDelayBeforeStartingTriggers field..."
				    << __E__;
			}
		}

		// if(!sleepSeconds) sleepSeconds = 22;
		__CFG_COUT__ << "Sleeping for " << sleepSeconds << " seconds..." << __E__;
		sleep(sleepSeconds);

		unsigned char channelCount = 0;
		bool          enable40MHzMask;
		unsigned int  gateChannelVetoSel;

		// there are 3 output channels (alias: signorm, sigcms1, sigcms2)
		std::array<std::string, 3> outChannelNames = {"Channel0", "Channel1", "Channel2"};

		__CFG_COUT__ << "Enabling output trigger channels!" << std::endl;

		// must do channel 0 last!! (synchronously enables all 3 channels)
		for(channelCount = 2; channelCount <= 2; --channelCount)
		{
			enable40MHzMask =
			    usingOptionalParams &&
			    optionalLink.getNode("EnableClockMask" + outChannelNames[channelCount])
			        .getValue<bool>();
			gateChannelVetoSel = usingOptionalParams
			                         ? optionalLink
			                               .getNode("InputChannelVetoSourceForOutput" +
			                                        outChannelNames[channelCount])
			                               .getValue<int>()
			                         : 0;

			OtsUDPFirmwareCore::writeAdvanced(
			    writeBuffer,
			    channelCount == 0 ? 0x4 : (0x18016 + channelCount - 1) /*address*/,
			    (enable40MHzMask ? 0x0 : 0x8) |
			        (gateChannelVetoSel <= 1
			             ? 0
			             : (1 << 2)) /*data*/);  // unreset output channel block
			OtsUDPHardware::write(writeBuffer);
		}

		__CFG_COUT__ << "Enabling sig mod block!" << __E__;
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer,
		    0x6 /*address*/,
		    sel_ctl_register_);  // enable sig mod block and
		                         // restore original register
		                         // value
		OtsUDPHardware::write(writeBuffer);
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Failed start setup!\n" << e.what() << std::endl;
		__CFG_COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}

	return false;
}

////========================================================================================================================
// void FEPSI46Interface::FEMacroGenerateTriggers(FEVInterface::frontEndMacroInArgs_t
// argsIn, 		FEVInterface::frontEndMacroOutArgs_t argsOut)
//{
//	__CFG_COUT__ << "FEMacroGenerateTriggers" << __E__;
//
//	unsigned int numberOfTriggers =
//			FEVInterface::getFEMacroInputArgument<unsigned
// int>(argsIn,"numberOfTriggers"); 	unsigned int clocksOfDelayBetweenTriggers =
//			FEVInterface::getFEMacroInputArgument<unsigned
// int>(argsIn,"clocksOfDelayBetweenTriggers"); 	std::string& triggersWereLaunched =
//			FEVInterface::getFEMacroOutputArgument(argsOut,"triggersWereLaunched");
//
//	unsigned int numberOfTriggersStr =
//			FEVInterface::getFEMacroInputArgument<std::string>(argsIn,"numberOfTriggers");
//
//	__CFG_COUT__ << "numberOfTriggers " << numberOfTriggers << __E__;
//	__CFG_COUT__ << "numberOfTriggersStr " << numberOfTriggersStr << __E__;
//	__CFG_COUT__ << "clocksOfDelayBetweenTriggers " << clocksOfDelayBetweenTriggers <<
//__E__;
//
//
//	//TODO launch triggers based on parameters...
//
//
//	__CFG_COUT__ << "triggersWereLaunched " << triggersWereLaunched << __E__;
//	triggersWereLaunched = "Done!";
//	__CFG_COUT__ << "triggersWereLaunched " << triggersWereLaunched << __E__;
//	FEVInterface::setFEMacroOutputArgument<unsigned int>(argsOut,"triggersWereLaunched",
//			42.2f);
//	__CFG_COUT__ << "triggersWereLaunched " << triggersWereLaunched << __E__;
//}

//========================================================================================================================
void FEPSI46Interface::FEMacroGenerateTriggers(__ARGS__)
{
	__CFG_COUT__ << "FEMacroGenerateTriggers" << __E__;

	unsigned int numberOfTriggers     = __GET_ARG_IN__("numberOfTriggers", unsigned int);
	unsigned int signalHiDuration     = __GET_ARG_IN__("signalHiDuration", unsigned int);
	unsigned int signalLoDuration     = __GET_ARG_IN__("signalLoDuration", unsigned int);
	std::string& triggersWereLaunched = __GET_ARG_OUT__("triggersWereLaunched");
	std::string  numberOfTriggersStr  = __GET_ARG_IN__("numberOfTriggers", std::string);

	__CFG_COUTV__(numberOfTriggers);
	__CFG_COUTV__(signalHiDuration);
	__CFG_COUTV__(signalLoDuration);
	__CFG_COUTV__(numberOfTriggersStr);

	// launch triggers based on parameters...
	{
		std::string writeBuffer;

		// clear bit 5 in 0x18001
		nimEnables_.reset(5);
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x18001, nimEnables_.to_ulong());  // enable or disable sig gen
		OtsUDPHardware::write(writeBuffer);
		__CFG_COUT__ << "Nim Enables set to 0x" << std::hex << nimEnables_ << std::dec
		             << std::endl;

		// 3 registers:
		//	count 		0x18005 (27:0)
		//	hi_duration 0x18006 (27:0)
		//	lo_duration 0x18007 (31:0)

		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x18005, nimEnables_.to_ulong());  // enable or disable sig gen
		OtsUDPHardware::write(writeBuffer);
		__CFG_COUT__ << "Nim Enables set to 0x" << std::hex << nimEnables_ << std::dec
		             << std::endl;

		// Note: select input mux to sig gen
		//	muxInputA 0x108 => 1
		//		B, C, D are on same register shifted up by 3 bits.

		// set bit 5
		nimEnables_.set(5);
		OtsUDPFirmwareCore::writeAdvanced(
		    writeBuffer, 0x18001, numberOfTriggers);  // enable or disable sig gen
		OtsUDPHardware::write(writeBuffer);
		__CFG_COUT__ << "Nim Enables set to 0x" << std::hex << nimEnables_ << std::dec
		             << std::endl;
	}

	__CFG_COUT__ << "triggersWereLaunched " << triggersWereLaunched << __E__;
	triggersWereLaunched = "Done!";
	__CFG_COUT__ << "triggersWereLaunched " << triggersWereLaunched << __E__;
	__SET_ARG_OUT__("triggersWereLaunched", 42.2f);  //,unsigned int);
	__CFG_COUT__ << "triggersWereLaunched " << triggersWereLaunched << __E__;
	__SET_ARG_OUT__("triggersWereLaunched", 42.2f);  //,float);
	__CFG_COUT__ << "triggersWereLaunched " << triggersWereLaunched << __E__;
}

DEFINE_OTS_INTERFACE(FEPSI46Interface)
