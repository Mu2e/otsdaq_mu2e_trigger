
cet_build_plugin(Mu2ePrescaler art::module
  LIBRARIES REG
  fhiclcpp::types
  artdaq-core::Data
  artdaq-core-mu2e::Data
  artdaq-core-mu2e::Data_dict
  artdaq::DAQdata
  otsdaq::Macros
)

install_headers(SUBDIRS detail)
install_source()
