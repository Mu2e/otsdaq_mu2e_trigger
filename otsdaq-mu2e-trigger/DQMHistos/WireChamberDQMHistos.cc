#include "otsdaq-mu2e-trigger/DQMHistos/WireChamberDQMHistos.h"
#include "otsdaq/ConfigurationInterface/ConfigurationTree.h"

#include <iostream>
#include <sstream>
#include <string>

#include <TString.h>

#include <TCanvas.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TFrame.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH2.h>
#include <TH2F.h>
#include <TProfile.h>
#include <TROOT.h>
#include <TRandom.h>
#include <TThread.h>

#include <stdint.h>

// ROOT documentation
// http://root.cern.ch/root/html/index.html

using namespace ots;

//========================================================================================================================
// WireChamberDQMHistos::WireChamberDQMHistos(std::string supervisorApplicationUID,
// std::string bufferUID, std::string processorUID) : theDataDecoder_
//(supervisorApplicationUID, bufferUID, processorUID) , bufferUID_         (bufferUID) ,
// processorUID_      (processorUID)
//{
//}

//========================================================================================================================
WireChamberDQMHistos::WireChamberDQMHistos(void) {}

//========================================================================================================================
WireChamberDQMHistos::~WireChamberDQMHistos(void) {}

//========================================================================================================================
void WireChamberDQMHistos::book(TDirectory*              myDirectory,
                                const ConfigurationTree& theXDAQContextConfigTree,
                                const std::string&       configurationPath)
{
	clear();
	// Make Histograms
	std::stringstream name;
	for(unsigned int ichamber = 0; ichamber < MAX_CHAMBERS; ichamber++)
	{
		name.str("");
		name << "MWPC" << ichamber;
		// ame += ichamber;
		// name += "_";
		// name += filename;
		h2_profile[ichamber] = new TH2F(
		    name.str().c_str(), name.str().c_str(), 128, -0.5, 127.5, 128, -0.5, 127.5);
		h2_profile[ichamber]->SetXTitle("x (mm)");
		h2_profile[ichamber]->SetYTitle("y (mm)");

		for(unsigned int imod = 0; imod < 4; imod++)
		{
			name.str("");
			name << "TDC" << ichamber * 4 + imod + 1;
			h_tdc[ichamber][imod] =
			    new TH1F(name.str().c_str(), name.str().c_str(), 500, -0.5, 499.5);
			h_tdc[ichamber][imod]->SetXTitle("TDC");
		}
	}

	std::cout << __COUT_HDR_FL__ << "Booking done!" << std::endl;
}

//========================================================================================================================
void WireChamberDQMHistos::clear() {}

//========================================================================================================================
// ERIC LOOK HERE FOR DATA FORMAT/UNPACKING
void WireChamberDQMHistos::convertSpillData(const std::string& spillData)
{
	// Unpack Controller Header
	[[maybe_unused]]
	unsigned int words, spillLinkStatus, spillTDCStatus, spillTriggerCount{0}, minute,
	    second, day, hour, year, month, spillCount, totalWordCount, index;
	unsigned int       tmpInt;
	const unsigned int wordSz = 2;
	unsigned int       it;
	bool               doneFlag = false;

	for(it = 0; it < spillData.size() - (wordSz - 1) && !doneFlag; it += wordSz)
	{
		tmpInt = 0;
		for(unsigned int j = 0; j < wordSz; ++j)
			tmpInt |= (((unsigned int)spillData[it + j]) & 0x0FF)
			          << (8 * (wordSz - 1 - j));

		switch(it / wordSz)  // word index
		{
		case 0:  // word count
			totalWordCount = tmpInt << 16;
			break;
		case 1:  // word count
			totalWordCount |= tmpInt;
			break;
		case 2:  // spill count
			spillCount = tmpInt;
			break;
		case 3:  // year and month
			year  = tmpInt >> 8;
			month = tmpInt & 0x00FF;
			break;
		case 4:  // day and hour
			day  = tmpInt >> 8;
			hour = tmpInt & 0x00FF;
			break;
		case 5:  // minute and second
			minute = tmpInt >> 8;
			second = tmpInt & 0x00FF;
			break;
		case 6:  // spillTriggerCount 32 bit number
			spillTriggerCount = tmpInt << 16;
			break;
		case 7:  // spillTriggerCount
			spillTriggerCount |= tmpInt;
			break;
		case 8:  // spillTDCStatus
			spillTDCStatus = tmpInt;
			break;
		case 9:  // spillLinkStatus
			spillLinkStatus = tmpInt;
			doneFlag        = true;  // done with header
			words           = it / wordSz + 1;
			break;
		default:;
		}
	}
	//	__MOUT__ << "////////////////////////HEADER////////////////////// " << std::endl;
	//	__MOUT__ << "wordCount " << totalWordCount << std::endl;
	//
	//	__MOUT__ << "SPILL\t" << spillCount << std::endl;
	//	//outFile_ << SDATE
	//	__MOUT__ << "Date: " << month << "/" << day << "/" << year << std::endl;
	//	__MOUT__ << "Time: " << hour << ":" << minute << ":" << second << std::endl;
	//	__MOUT__ << "spillTriggerCount " << spillTriggerCount << std::endl;
	//	__MOUT__ << "spillTDCStatus " << spillTDCStatus << std::endl;
	//	__MOUT__ << "spillLinkStatus " << spillLinkStatus << std::endl;
	//	__MOUT__ << "words  " << words << " = " << it/2 << std::endl;
	//	__MOUT__ << "//////////////////////////////////////////////////// " << std::endl;

	// Unpack TDC Spill Header
	// unsigned int tdcSpillWordCount, tdcNumber, tdcSpillTriggerCount, tdcSpillStatus,
	// tdcWords = 1;
	unsigned int tdcHeaderIndex = 0;
	// unsigned int numberOfTDCs = 16;
	// while(it < /*spillData.size()-(wordSz-1)*/)//FIXME Tries to loop through all the
	// data, need to stop after header is finished; Need to detect TDC Header Size
	while(tdcHeaderIndex < NUMBER_OF_TDCs)
	{
		unsigned int startIt = it;
		doneFlag             = false;
		struct TDCHeaderStruct tdcHeader;
		for(; it < spillData.size() - (wordSz - 1) && !doneFlag; it += wordSz)
		{
			tmpInt = 0;
			for(unsigned int j = 0; j < wordSz; ++j)
				tmpInt |= (((unsigned int)spillData[it + j]) & 0x0FF)
				          << (8 * (wordSz - 1 - j));

			switch((it - startIt) / wordSz)  // word index
			{
			case 0:  // tdcSpillWordCount 32 bit number
				tdcHeader.tdcSpillWordCount = tmpInt << 16;
				break;
			case 1:  // tdcSpillWordCount
				tdcHeader.tdcSpillWordCount |= tmpInt;
				break;
			case 2:  // tdcNumber
				tdcHeader.tdcNumber = tmpInt;
				break;
			case 3:
				tdcHeader.tdcSpillTriggerCount = tmpInt << 16;
				break;
			case 4:
				tdcHeader.tdcSpillTriggerCount |= tmpInt;
				break;
			case 5:
				tdcHeader.tdcSpillStatus = tmpInt;
				doneFlag                 = true;
				tdcHeader.tdcWords       = (it - startIt) / wordSz + 1;
				break;
			default:
				break;
			}
		}
		tdcHeader.tdcHeaderIndex = tdcHeaderIndex;

		//		__MOUT__ << "/////////////////// TDC HEADER////////////////////// " 	<<
		// std::endl;
		//		//		__MOUT__ << "tdcHeaderIndex " 		<< tdcHeader.tdcHeaderIndex
		//<<  std::endl;
		//		__MOUT__ << "tdcSpillWordCount " 	<< tdcHeader.tdcSpillWordCount 		<<
		// std::endl;
		//		__MOUT__ << "tdcNumber " 			<< tdcHeader.tdcNumber 				<<
		// std::endl;
		//		__MOUT__ << "tdcSpillTriggerCount " << tdcHeader.tdcSpillTriggerCount 	<<
		// std::endl;
		//		__MOUT__ << "tdcSpillStatus " 		<< tdcHeader.tdcSpillStatus 		<<
		// std::endl;
		//		__MOUT__ << "tdcWords " 			<< tdcHeader.tdcWords 				<<
		// std::endl;
		//		__MOUT__ << "//////////////////////////////////////////////////// " 	<<
		// std::endl;

		++tdcHeaderIndex;
	}

	//	__MOUT__ << "/////////////////// END OF DATA////////////////////// " << std::endl;
	//	__MOUT__ << "//////////////////////////////////////////////////// " << std::endl;
	//	__MOUT__ << "///////// unpacking tdc events ///////////////////// " << std::endl;
	//	__MOUT__ << "spillTriggerCount: " << spillTriggerCount << std::endl;
	//	__MOUT__ << "Current data index: " << it << std::endl;
	//	__MOUT__ << "Spill data size: " << spillData.size() << std::endl;
	// Unpack TDC Events
	for(unsigned int triggerIndex = 0; triggerIndex < spillTriggerCount; ++triggerIndex)
	{
		//		__MOUT__ << "Trigger Index: " << triggerIndex << std::endl;

		for(unsigned int numTDCsIndex = 0; numTDCsIndex < NUMBER_OF_TDCs; ++numTDCsIndex)
		{
			//			__MOUT__ << "numTDCsIndex: " << numTDCsIndex << std::endl;

			unsigned int startIt = it;
			doneFlag             = false;
			struct TDCEvent tdcEvent;
			for(; it < spillData.size() - (wordSz - 1) && !doneFlag; it += wordSz)
			{
				tmpInt = 0;
				for(unsigned int j = 0; j < wordSz; ++j)
					tmpInt |= (((unsigned int)spillData[it + j]) & 0x0FF)
					          << (8 * (wordSz - 1 - j));

				switch((it - startIt) / wordSz)  // word index
				{
				case 0:  // tdcEvent Word Count
					tdcEvent.wordCount = tmpInt;
					break;
				case 1:  // tdcEvent tdcNumber
					tdcEvent.tdcNumber = tmpInt;
					break;
				case 2:  // tdcEvent Event Status
					tdcEvent.eventStatus = tmpInt;
					break;
				case 3:  // tdcEvent Trigger Number (32 bit number)
					tdcEvent.triggerNumber = tmpInt << 16;
					break;
				case 4:
					tdcEvent.triggerNumber |= tmpInt;
					break;
				case 5:  // tdcEvent Trigger Type
					tdcEvent.triggerType = tmpInt;
					break;
				case 6:  // tdcEvent Controller Timestamp
					tdcEvent.controllerEventTimeStamp = tmpInt;
					break;
				case 7:  // tdcEvent Event TimeStamp (32 bit number)
					tdcEvent.tdcEventTimeStamp = tmpInt << 16;
					break;
				case 8:
					tdcEvent.tdcEventTimeStamp |= tmpInt;
					doneFlag           = true;
					tdcEvent.dataWords = (it - startIt) / wordSz + 1;
					tdcEvent.words     = 0;
					break;
				default:
					break;
				}
			}

			//			__MOUT__ << "/////////////////// TDC Event////////////////////// "
			//<< 	std::endl;
			//			__MOUT__ << "wordCount " 				<< tdcEvent.wordCount
			//<<  std::endl;
			//			__MOUT__ << "tdcNumber " 				<< tdcEvent.tdcNumber
			//<<  std::endl;
			//			__MOUT__ << "eventStatus " 				<< tdcEvent.eventStatus
			//<<  std::endl;
			//			__MOUT__ << "triggerNumber " 			<< tdcEvent.triggerNumber
			//<< std::endl;
			//			__MOUT__ << "triggerType " 				<< tdcEvent.triggerType
			//<<  std::endl;
			//			__MOUT__ << "controllerEventTimeStamp " <<
			// tdcEvent.controllerEventTimeStamp	<< std::endl;
			//			__MOUT__ << "tdcEventTimeStamp " 		<<
			// tdcEvent.tdcEventTimeStamp
			//<< std::endl;
			//			__MOUT__ << "////////////////////////////////////////////////////
			//"
			//<< std::endl;

			//			__MOUT__ << "////////////////////tdcData/////////////////" <<
			// std::endl;
			//			__MOUT__ << "dataWords " 				<< tdcEvent.dataWords
			//<<  std::endl;
			//			__MOUT__ << "wordCount " 				<< tdcEvent.wordCount
			//<<  std::endl;

			for(; tdcEvent.dataWords < tdcEvent.wordCount &&
			      it < spillData.size() - (wordSz - 1);
			    it += wordSz)
			{
				tmpInt = 0;
				for(unsigned int j = 0; j < wordSz; ++j)
					tmpInt |= (((unsigned int)spillData[it + j]) & 0x0FF)
					          << (8 * (wordSz - 1 - j));

				tdcEvent.tdcData.push_back(tmpInt);
				++tdcEvent.dataWords;
				std::cout << tmpInt << ", ";
			}
			//			__MOUT__ << "////////////////End of tdcData/////////////" 	<<
			// std::endl;
			//			__MOUT__ << "dataWords " << tdcEvent.dataWords				<<
			// std::endl;
			//			__MOUT__ << "///////////////////////////////////////////" 	<<
			// std::endl;

			vectorOfTDCEvents_.push_back(tdcEvent);
		}
	}
}
//========================================================================================================================
void WireChamberDQMHistos::fill(std::string&                       buffer,
                                std::map<std::string, std::string> header)
{
	// std::map<unsigned int, std::pair<unsigned int, unsigned int> >
	unsigned int nhit_x;
	unsigned int wire_x[MAX_HITS];
	//unsigned int time_x[MAX_HITS];

	unsigned int nhit_y;
	unsigned int wire_y[MAX_HITS];
	//unsigned int time_y[MAX_HITS];

	unsigned int chamber;
	unsigned int chamberModuleNumber;  // module number within chamber, 0-3
	unsigned int xory;                 // 0 = x-wire, 1 = y-wire
	unsigned int first_or_second;

	convertSpillData(buffer);

	nhit_x = 0;
	nhit_y = 0;
	for(auto event : vectorOfTDCEvents_)
	{
		chamber = (event.tdcNumber - 1) / 4;
		chamberModuleNumber =
		    (event.tdcNumber - 1) % 4;                // module number within chamber, 0-3
		xory            = (chamberModuleNumber) / 2;  // 0 = x-wire, 1 = y-wire
		first_or_second = (chamberModuleNumber) % 2;
		for(auto data : event.tdcData)
		{
			unsigned int chan = (data & 0xFC00) >> 10;
			unsigned int tdc  = data & 0x03FF;

			//			__MOUT__ << std::hex << "RAW: " << data << " CHANNEL: " << chan <<
			//"  TDC: " << tdc
			//					<< " CHAMBER: " << chamber << " MODULENUMBER: " <<
			// chamberModuleNumber << std::endl;
			// Fill tdc hit distribution
			h_tdc[chamber][chamberModuleNumber]->Fill(tdc);

			if(xory == 0)  // x-wire
			{
				if(nhit_x < MAX_HITS - 1)
				{
					wire_x[nhit_x] = chan + 64 * first_or_second;
					//time_x[nhit_x] = tdc;
					nhit_x++;
				}
				else
				{
					__MOUT__ << "Too many x-hits, mwpc " << event.tdcNumber << ", "
					         << nhit_x << " hits!" << std::endl;
				}
			}
			else if(xory == 1)
			{
				if(nhit_y < MAX_HITS - 1)
				{
					wire_y[nhit_y] = chan + 64 * first_or_second;
					//time_y[nhit_y] = tdc;
					nhit_y++;
				}
				else
				{
					__MOUT__ << "Too many y-hits, mwpc " << event.tdcNumber << ", "
					         << nhit_y << " hits!" << std::endl;
				}
			}
		}
		if(chamberModuleNumber == 3)
		{
			//__MOUT__ << "CHAMBERMODULENUMBER = 3!!! FILLING HISTOGRAM MWPC!" <<
			// std::endl;
			//__MOUT__ << "NumHitsX: " << nhit_x << " NumHitsY: " << nhit_y << std::endl;

			int onlygoodhits = 0;
			for(unsigned int ix = 0; ix < nhit_x; ix++)
			{
				float xpos = wire_x[ix];
				//int   xtdc = time_x[ix];
				//int   xmod = xpos / 64;

				// Use only hits within first peak
				if(onlygoodhits)  // && xtdc > tdcmean[xmod]+2.0*tdcsigma[xmod] )
				{
					continue;
				}

				for(unsigned int iy = 0; iy < nhit_y; iy++)
				{
					int ywire = wire_y[iy];
					// invert ywire to geometric position
					float ypos = (MAX_WIRES - 1) - ywire;
					//int   ytdc = time_y[iy];
					//int   ymod = 2 + ywire / 64;

					// Use only hits within first peak
					if(onlygoodhits)  //&& ytdc > tdcmean[ymod]+2.0*tdcsigma[ymod] )
					{
						continue;
					}
					//__MOUT__ << "MWPC \t xpos " << xpos << ", ypos " << ypos <<
					// std::endl;
					h2_profile[chamber]->Fill(xpos, ypos);
				}
			}
			nhit_x = 0;
			nhit_y = 0;
		}
	}
}

//========================================================================================================================
void WireChamberDQMHistos::load(std::string fileName)
{
	/*LORE 2016 MUST BE FIXED THIS MONDAY
	DQMHistosBase::openFile (fileName);
	numberOfTriggers_ = (TH1I*)theFile_->Get("General/NumberOfTriggers");

	std::string directory = "Planes";
	std::stringstream name;
	for(unsigned int p=0; p<4; p++)
	{
	    name.str("");
	    name << directory << "/Plane_" << p << "_Occupancy";
	    //FIXME Must organize better all histograms!!!!!
	    //planeOccupancies_.push_back((TH1I*)theFile_->Get(name.str().c_str()));
	}
	//canvas_ = (TCanvas*) theFile_->Get("MainDirectory/MainCanvas");
	//histo1D_ = (TH1F*) theFile_->Get("MainDirectory/Histo1D");
	//histo2D_ = (TH2F*) theFile_->Get("MainDirectory/Histo2D");
	//profile_ = (TProfile*) theFile_->Get("MainDirectory/Profile");
	closeFile();
	 */
}
