art_make(BASENAME_ONLY
  LIBRARY_NAME otsdaq-trigger_Generators
  NO_PLUGINS
  EXCLUDE Mu2eReceiver_generator.cc STIBReceiver_generator.cc STIBEventReceiver_generator.cc WireChamberEventReceiver_generator.cc WireChamberReceiver_generator.cc CAMACReceiver_generator.cc
  LIB_LIBRARIES
  artdaq-core_Data
  artdaq_DAQdata
  artdaq_Application
  ${FHICLCPP}
  ${MF_MESSAGELOGGER}
  ${CETLIB}
  ${CETLIB_EXCEPT}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  )

# simple_plugin(Mu2eReceiver "generator"
#   mu2e-artdaq-core_Overlays
#   artdaq-utilities_Plugins
#   artdaq-core_Utilities
#   #  artdaq_Application
#   artdaq_Generators
#   artdaq_DAQdata
#   artdaq_DAQrate
#   artdaq-core_Data
#   ${CANVAS_LIB_LIST}
#   ${FHICLCPP}
#   ${CETLIB}
#   ${CETLIB_EXCEPT}
#   MF_MessageLogger
#   DTCInterface
#   ${Boost_SYSTEM_LIBRARY}
#   )

