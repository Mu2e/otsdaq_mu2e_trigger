simple_plugin(FSSRInterface "interface" 
  Configurable
  DAQHardware
  WorkLoopManager
  DetectorConfiguration
  ConfigurationInterface
  TableCore
  FECore
  NetworkUtilities
  Macros
  artdaq_DAQdata
  MF_MessageLogger
  ${XDAQ_XERCES-C}
  ${Boost_SYSTEM_LIBRARY}
  pthread
  )
  
# simple_plugin(FEOtsUDPFSSRInterface "interface"
#  DAQHardware
#  FECore
#  WorkLoopManager
#  DetectorConfiguration
#  ConfigurationInterface
#  TableCore
#  NetworkUtilities
#  ${CETLIB}
#  )
  
  simple_plugin(FTBFWireChamberInterface "interface"
  Configurable
  DAQHardware
  FECore
  WorkLoopManager
#  MessageFacility
  NetworkUtilities
  ConfigurationInterface
  TableCore
  TCPSocket
  Macros
  artdaq_DAQdata
  ${CETLIB}
  ${CETLIB_EXCEPT}
  ${Boost_SYSTEM_LIBRARY}
	${MF_MESSAGELOGGER}
  )
  
  simple_plugin(FECAMACInterface "interface"
  Configurable
  FECore
  WorkLoopManager
  ConfigurationInterface
  TableCore
  NetworkUtilities
  Macros
  artdaq_DAQdata
  xx_usb
  usb
  ${XDAQ_XERCES-C}
  ${Boost_SYSTEM_LIBRARY}
  MF_MessageLogger
  pthread
  )
  
  simple_plugin(FECAMACTCPInterface "interface"
  Configurable
  FECore
  WorkLoopManager
  ConfigurationInterface
  TableCore
  NetworkUtilities
  Macros
  artdaq_DAQdata
  xx_usb
  usb
  ${XDAQ_XERCES-C}
  ${Boost_SYSTEM_LIBRARY}
  MF_MessageLogger
  pthread
  )

 simple_plugin(FEPurdueNetworkMasterInterface "interface"
 Configurable
  DAQHardware
  WorkLoopManager
  FECore
  DetectorConfiguration
  ConfigurationInterface
  TableCore
  NetworkUtilities
  Macros
  artdaq_DAQdata
  ${MF_MESSAGELOGGER}
  ${XDAQ_XERCES-C}
  ${Boost_SYSTEM_LIBRARY}
  pthread
  )

simple_plugin(FEPixelTelescopeInterface "interface"
  Configurable
  FECore
  DetectorConfiguration
  WorkLoopManager
  NetworkUtilities
  ConfigurationInterface
  TableCore
  Macros
  artdaq_DAQdata
  ${CETLIB}
	${MF_MESSAGELOGGER}
  ${XDAQ_XERCES-C}
  ${Boost_SYSTEM_LIBRARY}
  pthread
  )

simple_plugin(FECommanderInterface "interface"
  Configurable
  FECore
  DetectorConfiguration
  WorkLoopManager
  NetworkUtilities
  ConfigurationInterface
  Macros
  artdaq_DAQdata
  TableCore
  ${CETLIB}
  ${MF_MESSAGELOGGER}
  ${XDAQ_XERCES-C}
  ${Boost_SYSTEM_LIBRARY}
  pthread
  )
