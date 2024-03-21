
include(otsdaq::table)
  
cet_build_plugin(TopLevelTriggerTable otsdaq::table LIBRARIES REG
  )

cet_build_plugin(TriggerTableFromJSON otsdaq::table LIBRARIES REG
  )


  
install_headers()
install_source()
