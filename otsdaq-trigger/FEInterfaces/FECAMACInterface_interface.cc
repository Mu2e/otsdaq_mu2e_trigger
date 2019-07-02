/*********************************************************************************************************************\
Front end interface written for OTSDAQ to communicate with the Wiener, Plein, & Baus
CC-USB CAMAC controller. Primary purpose is data acquisition from CAMAC detectors at the
Fermilab Test Beam. Ben Roberts, Summer 2018
\*********************************************************************************************************************/

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/Macros/InterfacePluginMacros.h"
#include "otsdaq-trigger/CAMAC_XXUSB/UsbHandler.cpp"
#include "otsdaq-trigger/CAMAC_XXUSB/UsbHandler.h"
#include "otsdaq-trigger/FEInterfaces/FECAMACInterface.h"

using namespace ots;

//========================================================================================================================
ots::FECAMACInterface::FECAMACInterface(const std::string&       interfaceUID,
                                        const ConfigurationTree& theXDAQContextConfigTree,
                                        const std::string& interfaceConfigurationPath)
    : Socket(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                 .getNode("HostIPAddress")
                 .getValue<std::string>(),
             theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                 .getNode("HostPort")
                 .getValue<unsigned int>())
    , FEVInterface(interfaceUID, theXDAQContextConfigTree, interfaceConfigurationPath)
    , UDPDataStreamerBase(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                              .getNode("HostIPAddress")
                              .getValue<std::string>(),
                          theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                              .getNode("HostPort")
                              .getValue<unsigned int>(),
                          theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                              .getNode("StreamToIPAddress")
                              .getValue<std::string>(),
                          theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                              .getNode("StreamToPort")
                              .getValue<unsigned int>())
    , camac(nullptr)  //< This object contains the code that interacts with the CC-USB
    , init_sent_(false)
{
	universalAddressSize_ = 8;
	universalDataSize_    = 8;
}

//========================================================================================================================
ots::FECAMACInterface::~FECAMACInterface(void)
{
	if(rawOutput && output.is_open())
		output.close();
}

//========================================================================================================================
void ots::FECAMACInterface::halt(void)
{
	__CFG_MOUT__ << "\tHalt" << std::endl;
	camac.reset(nullptr);
	if(rawOutput && output.is_open())
		output.close();
}

//========================================================================================================================
void ots::FECAMACInterface::pause(void) { __CFG_MOUT__ << "\tPause" << std::endl; }

//========================================================================================================================
void ots::FECAMACInterface::resume(void) { __CFG_MOUT__ << "Resuming" << std::endl; }
//========================================================================================================================
void ots::FECAMACInterface::start(std::string runNumber)
{
	__CFG_MOUT__ << "Starting CAMAC Interface" << std::endl;
	if(rawOutput)
	{
		// Setup stream to output file.
		output.open(rawOutputFile + "_" + runNumber + ".raw",
		            std::ios::out | std::ios::trunc);
		if(!output.is_open())
		{
			__CFG_MOUT__ << "Could not open file.";
			return;
		}
		__CFG_MOUT__ << "Output file created at " << rawOutputFile << "_" << runNumber
		             << ".raw" << std::endl;
		std::time_t now =
		    std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		char buf[100];
		std::strftime(buf, sizeof(buf), "%F %T", std::localtime(&now));
		std::string header = buf;
		header += "\n";
		output << header << cfgHeader;
	}

	// Send configuration downstream
	camac->configureJorway();
}

//========================================================================================================================
void ots::FECAMACInterface::stop(void)
{
	__CFG_MOUT__ << "Stopping CAMAC Interface" << std::endl;
	if(rawOutput && output.is_open())
		output.close();  // Close the output file.
}

//========================================================================================================================
void ots::FECAMACInterface::configure(void)
{
	__CFG_MOUT__ << "Configuring CAMAC crate." << std::endl;
	camac.reset(new UsbHandler());
	std::string ADC = theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                      .getNode("ADCList")
	                      .getValue<std::string>();
	std::string TDC = theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                      .getNode("TDCList")
	                      .getValue<std::string>();
	std::string Scal = theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                       .getNode("ScalerList")
	                       .getValue<std::string>();
	std::string Gate = theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                       .getNode("GateList")
	                       .getValue<std::string>();
	rawOutput = theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                .getNode("RawOutput")
	                .getValue<bool>();
	std::string header = theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                         .getNode("RawFileHeader")
	                         .getValue<std::string>();
	std::string filePath = theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                           .getNode("RawFilePath")
	                           .getValue<std::string>();
	std::string filePrefix = theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                             .getNode("RawFilenamePrefix")
	                             .getValue<std::string>();
	rawOutputFile = filePath + "/" + filePrefix;
	cfgHeader     = header + "\n" + camac->setConfig(ADC, TDC, Scal, Gate);
	cardList = "ADC " + ADC + " TDC " + TDC + " SCAL " + Scal + " GATE " + Gate + " END";
	__CFG_MOUT__ << "Selected devices: " << cardList << std::endl;

	int retries = 5;
	int ret     = -1;
	while(retries > 0 && ret < 0)
	{
		ret = camac->configureCrate();
		if(ret < 0)
		{
			__CFG_MOUT_ERR__ << "Failed CAMAC configure, retries=" << retries
			                 << ", ret=" << ret;
		}
		retries--;
	}
	__CFG_MOUT__ << "Finished configuring FECAMACInterface" << std::endl;
}

//========================================================================================================================
bool ots::FECAMACInterface::running(void)
{
	auto                  last_send_time = std::chrono::steady_clock::now();
	std::vector<uint16_t> data_vec;

	while(WorkLoop::continueWorkLoop_)
	{
		ccusb_buf_vec pdata    = camac->getData();
		int           numReads = pdata.size();
		if(numReads < 0)
		{
			__CFG_MOUT_ERR__ << "Error reading data, getData returned " << numReads
			                 << std::endl;
			return false;
		}
		else if(numReads == 0)
		{
			__CFG_MOUT__ << "No data received, continuing..." << std::endl;
			// TransmitterSocket::send(streamToSocket_, "NODATA");
			usleep(10000);
			continue;
		}

		for(int read = 0; read < numReads; read++)
		{
			int events = pdata[read][0] & 0xfff;  // Read number of events from first
			                                      // word.

			if(events > 0)
			{
				__CFG_MOUT__ << "Read " << read << ", " << events << " events."
				             << std::endl;
				if(rawOutput)
					output << std::hex << std::showbase << pdata[read][0] << std::dec
					       << std::noshowbase << std::endl;

				int idx = 1;
				// Print each event on a separate line using the event terminator 0xfeed.
				// Skip the terminator and the event size header.
				for(int i = 0; i < events; ++i)
				{
					int eventLength = pdata[read][idx];
					data_vec.push_back(pdata[read][idx]);
					idx++;

					__CFG_MOUT__ << "Event length is " << eventLength << std::endl;

					for(int j = 0; j < eventLength; ++j)
					{
						data_vec.push_back(pdata[read][idx]);
						if(rawOutput)
							output << std::setw(10) << pdata[read][idx];
						++idx;
					}
					if(rawOutput)
						output << std::endl;
				}
			}
		}

		if(!init_sent_)
		{
			__CFG_MOUT__ << "Sending init message" << std::endl;
			TransmitterSocket::send(streamToSocket_, cardList);
			init_sent_ = true;
		}

		if(data_vec.size() > 500 ||
		   std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(
		       std::chrono::steady_clock::now() - last_send_time)
		           .count() > 0.1)
		{
			if(data_vec.size() > 0)
			{
				__CFG_MOUT__ << "Sending data..." << std::endl;
				TransmitterSocket::send(streamToSocket_, data_vec);
				last_send_time = std::chrono::steady_clock::now();
				data_vec.clear();
			}
		}
	}
	__MOUT__ << "Ending FECAMACInterface workloop." << std::endl;

	return false;  // WorkLoop::continueWorkLoop_;//otherwise it stops!!!!!
}

//========================================================================================================================

DEFINE_OTS_INTERFACE(ots::FECAMACInterface)
