#include "otsdaq-trigger/DataDecoders/VIPICData.h"

#include <iostream>

using namespace ots;

//========================================================================================================================
VIPICData::VIPICData(void) {}

//========================================================================================================================
VIPICData::~VIPICData(void) {}

//========================================================================================================================
bool VIPICData::isVIPIC(uint32_t data)
{
	int type = data & 0x0f;
	if(type == 2)
		return true;
	else
		return false;
}

//========================================================================================================================
VIPICData& VIPICData::decode(uint32_t word)
{
	// stibId_        =  0x03 &  (word >> 30) ; //FIXME: Missing in Jones' code
	// channelNumber_ =  0x07 &  (word >> 27) ;
	// chipId_        =  0x07 &  (word >> 24) ;
	// set_           =  0x1f &  (word >> 12) ;
	// stripNumber_   =  0x0f &  (word >> 17) ;
	// bco_           =  0xff &  (word >> 4)  ;
	// adc_           =  0x7  &  (word >> 1)  ;

	stibId_        = 0x03 & (word >> 30);  // FIXME: Missing in Jones' code
	channelNumber_ = 0x07 & (word >> 27);
	chipId_        = 0x07 & (word >> 24);
	set_           = 0x1f & (word >> 12);
	stripNumber_   = 0x0f & (word >> 17);
	bco_           = 0xff & (word >> 8);
	pixel_         = 0xfff & (word >> 20);

	return *this;

	// switch (type)
	// {
	// 		case 8:
	// 		{
	// 			switch(datatype)
	// 			{
	// 				case 2:
	// 				{
	// 					bco_counter &= 0x0000000000ffffffULL;
	// 					bco_counter |= ((uint64_t)data)<<24;
	// 					std::cout << __COUT_HDR_FL__ << "BCO = " << hex << bco_counter <<
	// dec
	// <<
	// "
	// ("
	// << bco_counter-last_bco << ")" << std::endl; 					if ( bco_counter -
	// last_bco > 256 ) { 						nhit = 0;
	// 					}
	// 					break;
	// 				}
	// 				case 1:
	// 				{
	// 					last_bco = bco_counter;
	// 					bco_counter &= 0xffffffffff000000ULL;
	// 					bco_counter |= (uint64_t)data;
	// 					break;
	// 				}
	// 				case 0x0a: {
	// 					int bco = (word>>8)&0xff;
	// 					int trignum_low = word>>16;
	// 					std::cout << __COUT_HDR_FL__ << "Trigger low = " << trignum_low <<
	// ", bco
	// =
	// "
	// << hex << bco << dec << std::endl; 					break;
	// 				}
	// 				case 0x0b: {
	// 					int trignum_high = word>>8;
	// 					std::cout << __COUT_HDR_FL__ << "Trigger high = " << trignum_high
	// << std::endl; 					break;
	// 				}
	// 				case 0x0c:
	// 				case 0x0d:
	// 				case 0x0e:
	// 				case 0x0f: {
	// 					int trig0 = (word>>16)&0xff;
	// 					int trig1 = (word>>24)&0xff;
	// 					int bco = (word>>8)&0xff;
	// 					std::cout << __COUT_HDR_FL__ << "Trigger word " << 15-datatype <<
	// "
	// :
	// "
	// << std::endl; 					for ( int k=7; k>=0; k-- ) {
	// 						if ( (trig1&(1<<k)) ) {
	// 							std::cout << __COUT_HDR_FL__ << "#" << std::endl;
	// 						}
	// 						else {
	// 							std::cout << __COUT_HDR_FL__ << "_" << std::endl;
	// 						}
	// 					}
	// 					std::cout << __COUT_HDR_FL__ << "  " << std::endl;
	// 					for ( int k=7; k>=0; k-- ) {
	// 						if ( (trig0&(1<<k)) ) {
	// 							std::cout << __COUT_HDR_FL__ << "#" << std::endl;
	// 						}
	// 						else {
	// 							std::cout << __COUT_HDR_FL__ << "_" << std::endl;
	// 						}
	// 					}
	// 					std::cout << __COUT_HDR_FL__ << "  " << hex << bco << dec <<
	// std::endl; 					break;
	// 				}
	// 				default:
	// 					//FIXME: Raises error of invalid data
	// 					break;
	// 			}
	// 			}
	// 		}

	// if ( type == 8 ) {
	//   if ( datatype == 2 ) {
	//     bco_counter &= 0x0000000000ffffffULL;
	//     bco_counter |= ((uint64_t)data)<<24;
	//     std::cout << __COUT_HDR_FL__ << "BCO = " << hex << bco_counter << dec << " ("
	//     << bco_counter-last_bco << ")" << std::endl; if ( bco_counter - last_bco > 256
	//     ) {
	//       nhit = 0;
	//     }
	//   }
	//   if ( datatype == 1 ) {
	//     last_bco = bco_counter;
	//     bco_counter &= 0xffffffffff000000ULL;
	//     bco_counter |= (uint64_t)data;
	//   }
	//   if ( datatype <= 0x0f && datatype >= 0x0c ) {
	//     int trig0 = (word>>16)&0xff;
	//     int trig1 = (word>>24)&0xff;
	//     int bco = (word>>8)&0xff;
	//     std::cout << __COUT_HDR_FL__ << "Trigger word " << 15-datatype << " : " <<
	//     std::endl; for ( int k=7; k>=0; k-- ) {
	//       if ( (trig1&(1<<k)) ) {
	//         std::cout << __COUT_HDR_FL__ << "#" << std::endl;
	//       }
	//       else {
	//         std::cout << __COUT_HDR_FL__ << "_" << std::endl;
	//       }
	//     }
	//     std::cout << __COUT_HDR_FL__ << "  " << std::endl;
	//     for ( int k=7; k>=0; k-- ) {
	//       if ( (trig0&(1<<k)) ) {
	//         std::cout << __COUT_HDR_FL__ << "#" << std::endl;
	//       }
	//       else {
	//         std::cout << __COUT_HDR_FL__ << "_" << std::endl;
	//       }
	//     }
	//     std::cout << __COUT_HDR_FL__ << "  " << hex << bco << dec << std::endl;
	//   }
	//   if ( datatype == 0x0a ) {
	//     int bco = (word>>8)&0xff;
	//     int trignum_low = word>>16;
	//     std::cout << __COUT_HDR_FL__ << "Trigger low = " << trignum_low << ", bco = "
	//     << hex << bco << dec << std::endl;
	//   }
	//   if ( datatype == 0x0b ) {
	//     int trignum_high = word>>8;
	//     std::cout << __COUT_HDR_FL__ << "Trigger high = " << trignum_high << std::endl;
	//   }
	// }
	//      else if (type & 1) {
	//        std::cout << __COUT_HDR_FL__ << "Channel: " << chan_ << " chipid: " <<
	//        chipid_ << " set: " << set_ << " strip: " << strip_ << " adc: " << adc_ << "
	//        bco: " << hex << bco << dec << " istrip: " << istrip_ << std::endl; if (
	//        chan > 6 || istrip > 639 ) {
	//          std::cout << __COUT_HDR_FL__ << "Bad word " << i << " = " << hex << word
	//          << dec << " = " << chan << "," << chipid << "," << set << "," << strip <<
	//          " = " << istrip << std::endl;
	//        }
	//        nhit += 1;
	//      }
	//      else if ( type == 2 ) {
	//        int pix = (word>>20)&0xfff;
	//        int count = (word>>16)&0x0f;
	//        int bco = (word>>8)&0xff;
	//        std::cout << __COUT_HDR_FL__ << hex << setw(8) << setfill('0') << word <<
	//        dec << "   VIPIC pixel " << pix << " count = " << count << ", bco = " << hex
	//        << bco << dec << std::endl;
	//      }
	//      else if ( type == 10 ) {
	//        int chan = (word>>24)&0x0f;
	//        int count = (word>>16)&0xff;
	//        int bco = (word>>8)&0xff;
	//        std::cout << __COUT_HDR_FL__ << hex << setw(8) << setfill('0') << word <<
	//        dec << "  PSI46 channel " << chan << " count = " << count << ", bco = " <<
	//        hex << bco << dec << std::endl;
	//      }
	//      else if ( type == 6 ) {
	//        int adc = (word>>8)&0xff;
	//        int row = (word>>16)&0x3f;
	//        int col = (word>>22)&0xf;
	//        std::cout << __COUT_HDR_FL__ << hex << setw(8) << setfill('0') << word <<
	//        dec << "  PSI46 hit col " << col << " row " << row << ", adc = " << adc <<
	//        std::endl;
	//      }

	// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << " Chan: " << chan << "
	// chipId: " << (int)chipId_
	//    << " set: " << (unsigned int)set_ << " strip: " << (int)stripNumber_ << " bco: "
	//    << (int)bco_ << std::endl;
}

//========================================================================================================================
unsigned int VIPICData::getStibId(void) { return (unsigned int)stibId_; }

//========================================================================================================================
unsigned int VIPICData::getChannelNumber(void) { return (unsigned int)channelNumber_; }

//========================================================================================================================
unsigned int VIPICData::getChipId(void) { return (unsigned int)chipId_; }

//========================================================================================================================
unsigned int VIPICData::getStripNumber(void) { return (unsigned int)stripNumber_; }

//========================================================================================================================
unsigned int VIPICData::getBco(void) { return (unsigned int)bco_; }

//========================================================================================================================
unsigned int VIPICData::getCol(void) { return (unsigned int)pixel_ % 64; }

//========================================================================================================================
unsigned int VIPICData::getRow(void) { return (unsigned int)pixel_ / 64; }
