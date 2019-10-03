#cet_make(LIBRARY_NAME TablePluginDataFormat
#		 LIBRARIES
#		 TableDataFormats
#         ${CETLIB}
#)
  
simple_plugin(TopLevelTriggerTable "table"
  TableCore
  ConfigurationInterface 
  Macros #ssssave and close 
  ${CETLIB}
  ${MF_MESSAGELOGGER}
  )
  


  
install_headers()
install_source()
