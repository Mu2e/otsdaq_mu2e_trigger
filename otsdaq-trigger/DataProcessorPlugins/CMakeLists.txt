simple_plugin(FSSRDQMHistosConsumer "processor"
    Configurable
	DataManager
	RootUtilities
	FermilabTestbeamDQMHistos
        WorkLoopManager
	${CETLIB}
	MF_MessageLogger
  )

simple_plugin(FSSRRawDataSaverConsumer "processor"
	DataManager
        WorkLoopManager
	${CETLIB}
	MF_MessageLogger
  )

simple_plugin(WireChamberDQMHistosConsumer "processor"
    Configurable
	DataManager
	RootUtilities
	FermilabTestbeamDQMHistos
        WorkLoopManager
	${CETLIB}
  )

simple_plugin(WireChamberDataSaverConsumer "processor"
	DataManager
        WorkLoopManager
	${CETLIB}
	${MF_MESSAGELOGGER}
  )

simple_plugin(RootDQMHistosConsumer "processor"
    Configurable
    RootUtilities
    DataManager
    WorkLoopManager
    ConfigurationInterface
    TableCore
	Macros
	${ROOT_BASIC_LIB_LIST}
	${MF_MESSAGELOGGER}
  )


install_headers()
install_source()
