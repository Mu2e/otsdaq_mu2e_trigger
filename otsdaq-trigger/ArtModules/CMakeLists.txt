art_make( BASENAME_ONLY
 LIBRARY_NAME otsdaq-trigger_ArtModules
 SUBDIRS detail
 NO_PLUGINS
 EXCLUDE anatka.C 
 DICT_DICTIONARIES
 ${ART_PERSISTENCY_PROVENANCE}
 ${CANVAS_PERSISTENCY_PROVENANCE}
 LIB_LIBRARIES
canvas
 otsdaq-trigger_Overlays
 #otsdaq-utilities_ECLWriter
 artdaq-core_Utilities
 artdaq_ArtModules
 artdaq-core_Data
 artdaq-core_Core
  art_Framework_Core
  art_Framework_IO_Sources
  art_Framework_Principal
#  canvas_Persistency_Common
  art_Persistency_Provenance
#  canvas_Persistency_Provenance
# canvas_Utilities
  MF_MessageLogger
  ${ROOT_CINTEX}
  ${ROOT_RIO}
  ${ROOT_CORE}
  ${ROOT_NET}
  ${ROOT_REFLEX}
  cetlib
  cetlib_except
  DTCInterface
)

# simple_plugin(WireChamberDQM "module"
# 	otsdaq-trigger_Overlays
#         ECLWriterCore
#   NetworkUtilities
# 	artdaq-core_Data
# 	${CETLIB_EXCEPT}
# 	${ROOT_GPAD}
# 	${ROOT_HIST}
# 	${ROOT_GUI}
# 	${ROOT_RIO}
#   ${ROOT_NET}
# #	MessageFacility
# 	${MF_MESSAGELOGGER}
# 	${MF_UTILITIES}
# 	${XMLRPC_SERVER_ABYSSPP}
# 	${XMLRPC_SERVER_ABYSS}
# 	${XMLRPC_SERVERPP}
# 	${XMLRPC_CLIENT}
# 	${XMLRPC_CLIENTPP}
# 	${XMLRPC_SERVER}
# 	${XMLRPC_ABYSS}
# 	${XMLRPCPP}
# 	${XMLRPC}
# 	${XMLRPC_UTIL}
# 	${XMLRPC_UTILPP}
# 	${XMLRPC_XMLPARSE}
# 	${XMLRPC_XMLTOK}
# 	curl
# 	pthread
# )
# simple_plugin(WireChamberDQMECL "module"
# 	otsdaq-trigger_Overlays
#         ECLWriterCore
#   NetworkUtilities
# 	artdaq-core_Data
# 	${CETLIB_EXCEPT}
# 	${ROOT_GPAD}
# 	${ROOT_HIST}
# 	${ROOT_GUI}
# 	${ROOT_RIO}
#   ${ROOT_NET}
# #	MessageFacility
# 	${MF_MESSAGELOGGER}
# 	${MF_UTILITIES}
# 	${XDAQ_XERCES-C}
# 	pthread
# )


# simple_plugin(WireChamberPythonOutput "module"
#         otsdaq-trigger_Overlays
#         artdaq-core_Data
#         ${CETLIB_EXCEPT}
# #        MessageFacility
#         ${MF_MESSAGELOGGER}
#         ${MF_UTILITIES}
# )

# simple_plugin(STIBDQM "module"
# 	otsdaq-trigger_Overlays
# 	     DataDecoders
#   NetworkUtilities
# 	artdaq-core_Data
# 	${CETLIB_EXCEPT}
# 	${ROOT_GPAD}
# 	${ROOT_HIST}
# 	${ROOT_GUI}
# 	${ROOT_RIO}
#   ${ROOT_NET}
# #	MessageFacility
# 	${MF_MESSAGELOGGER}
# 	${MF_UTILITIES}
# )

simple_plugin(TriggerCounter "module"
        artdaq-core_Utilities
	otsdaq-trigger_Overlays
	NetworkUtilities
	artdaq-core_Data
	${CETLIB_EXCEPT}
	${ROOT_GPAD}
	${ROOT_HIST}
	${ROOT_GUI}
	${ROOT_RIO}
  ${ROOT_NET}
#	MessageFacility
	${MF_MESSAGELOGGER}
	${MF_UTILITIES}
)

# simple_plugin(FTBFInput "source"
#   artdaq_ArtModules
#   artdaq_DAQdata
#   otsdaq-trigger_Overlays
#   artdaq-utilities_Plugins
#   artdaq-core_Core
#   artdaq-core_Utilities
#   rt
#   art_Framework_Core
#   art_Framework_IO_Sources
#   art_Framework_Principal
#   art_Persistency_Provenance
#   art_Utilities
#   MF_MessageLogger
# )

# simple_plugin(EventDataServer "module"
# otsdaq-trigger_Overlays
# artdaq_DAQrate
# artdaq_DAQdata
# artdaq-core_Data
# artdaq-core_Core
# MF_MessageLogger
# ${Boost_SYSTEM_LIBRARY}
# ${Boost_THREAD_LIBRARY}
# )

# simple_plugin(DummyAnalyzer "module"
# otsdaq-trigger_Overlays
# artdaq_DAQrate
# artdaq_DAQdata
# artdaq-core_Data
# artdaq-core_Core
# MF_MessageLogger
# ${Boost_SYSTEM_LIBRARY}
# ${Boost_THREAD_LIBRARY}
# )

simple_plugin(CaloSpy "module"
otsdaq-trigger_Overlays
artdaq_DAQrate
artdaq_DAQdata
artdaq-core_Data
artdaq-core_Core
MF_MessageLogger
${Boost_SYSTEM_LIBRARY}
${Boost_THREAD_LIBRARY}
)

# simple_plugin(PrescaleEvent "module"
# otsdaq-trigger_Overlays
# artdaq_DAQrate
# artdaq_DAQdata
# artdaq-core_Data
# artdaq-core_Core
# MF_MessageLogger
# ${Boost_SYSTEM_LIBRARY}
# ${Boost_THREAD_LIBRARY}
# )

# simple_plugin(OfflineFragmentsDumper "module"
#   mu2e-artdaq-core_Overlays
#   ${CETLIB_EXCEPT}
# )

# simple_plugin(OfflineFragmentReader "source"
#   MF_MessageLogger
#   artdaq_DAQdata
#   otsdaq-trigger_ArtModules
#   )

