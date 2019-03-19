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
  )
  


  
install_headers()
install_source()
