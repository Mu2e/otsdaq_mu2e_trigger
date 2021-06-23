cet_make(LIBRARY_NAME FermilabTestbeamDQMHistos
		 LIBRARIES
		 DataDecoders
		 otsdaq::ConfigurationInterface
		 otsdaq::NetworkUtilities
		 ${ROOT_BASIC_LIB_LIST}
)

install_headers()
install_source()
