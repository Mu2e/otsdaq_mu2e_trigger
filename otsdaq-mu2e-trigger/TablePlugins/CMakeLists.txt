
include(otsdaq::table)

cet_build_plugin(TriggerConfigTable otsdaq::table LIBRARIES REG
  )

cet_make_library(LIBRARY_NAME DBServiceTable
  SOURCE DBServiceTableImpl.cc
  LIBRARIES PUBLIC
    otsdaq::ConfigurationInterface
  )

cet_build_plugin(DBServiceTable otsdaq::table LIBRARIES REG
    otsdaq-mu2e-trigger::DBServiceTable
  )

cet_build_plugin(DBServiceCIDTable otsdaq::table LIBRARIES REG
    otsdaq-mu2e-trigger::DBServiceTable
  )

install_headers()
install_source()
