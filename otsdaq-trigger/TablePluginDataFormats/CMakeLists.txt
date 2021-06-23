#cet_make(LIBRARY_NAME TablePluginDataFormat
#		 LIBRARIES
#		 TableDataFormats
#         ${CETLIB}
#)
  
simple_plugin(TopLevelTriggerTable "table"
 artdaq_core::artdaq-core_Utilities
 otsdaq::ConfigurationInterface
  )
  


  
install_headers()
install_source()
