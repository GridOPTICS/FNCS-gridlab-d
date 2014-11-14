pkglib_LTLIBRARIES += communications/comm.la

communications_comm_la_CPPFLAGS =
communications_comm_la_CPPFLAGS += $(AM_CPPFLAGS)
communications_comm_la_CPPFLAGS += $(FNCS_CPPFLAGS)/fncs

communications_comm_la_LDFLAGS =
communications_comm_la_LDFLAGS += $(AM_LDFLAGS)
communications_comm_la_LDFLAGS += $(FNCS_LDFLAGS)

communications_comm_la_LIBADD = 
communications_comm_la_LIBADD += $(FNCS_LIBS)

communications_comm_la_SOURCES =
communications_comm_la_SOURCES += communications/comm.h
communications_comm_la_SOURCES += communications/init.cpp
communications_comm_la_SOURCES += communications/main.cpp
communications_comm_la_SOURCES += communications/network.cpp
communications_comm_la_SOURCES += communications/network.h
communications_comm_la_SOURCES += communications/network_interface.cpp
communications_comm_la_SOURCES += communications/network_interface.h
communications_comm_la_SOURCES += communications/mpi_network_message.cpp
communications_comm_la_SOURCES += communications/mpi_network_message.h
communications_comm_la_SOURCES += communications/market_network_interface.cpp
communications_comm_la_SOURCES += communications/market_network_interface.h
communications_comm_la_SOURCES += communications/controller_network_interface.cpp
communications_comm_la_SOURCES += communications/controller_network_interface.h
communications_comm_la_SOURCES += communications/transmission_interface.cpp
communications_comm_la_SOURCES += communications/transmission_interface.h
communications_comm_la_SOURCES += communications/network_message.cpp
communications_comm_la_SOURCES += communications/network_message.h
