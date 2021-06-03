simple_plugin(FSSRDQMHistosConsumer "processor"
    FermilabTestbeamDQMHistos
    artdaq_core::artdaq-core_Utilities
    otsdaq::DataManager
    otsdaq::RootUtilities
  )

simple_plugin(FSSRRawDataSaverConsumer "processor"
    FermilabTestbeamDQMHistos
    artdaq_core::artdaq-core_Utilities
    otsdaq::DataManager
    otsdaq::RootUtilities
  )

simple_plugin(WireChamberDQMHistosConsumer "processor"
    FermilabTestbeamDQMHistos
    artdaq_core::artdaq-core_Utilities
    otsdaq::DataManager
    otsdaq::RootUtilities
  )

simple_plugin(WireChamberDataSaverConsumer "processor"
    FermilabTestbeamDQMHistos
    artdaq_core::artdaq-core_Utilities
    otsdaq::DataManager
    otsdaq::RootUtilities
  )

simple_plugin(RootDQMHistosConsumer "processor"
    FermilabTestbeamDQMHistos
    artdaq_core::artdaq-core_Utilities
    otsdaq::DataManager
    otsdaq::RootUtilities
  )


install_headers()
install_source()
