pkglib_LTLIBRARIES += communications/comm.la

comm_comm_la_CPPFLAGS =
comm_comm_la_CPPFLAGS += $(AM_CPPFLAGS)
comm_comm_la_CPPFLAGS += $(FNCS_CPPFLAGS)

comm_comm_la_LDFLAGS =
comm_comm_la_LDFLAGS += $(AM_LDFLAGS)
comm_comm_la_LDFLAGS += $(FNCS_LDFLAGS)

comm_comm_la_LIBADD = 
comm_comm_la_LIBADD += $(FNCS_LIBS)

comm_comm_la_SOURCES =
comm_comm_la_SOURCES += communications/comm.h
comm_comm_la_SOURCES += communications/init.cpp
comm_comm_la_SOURCES += communications/main.cpp
comm_comm_la_SOURCES += communications/network.cpp
comm_comm_la_SOURCES += communications/network.h
comm_comm_la_SOURCES += communications/network_interface.cpp
comm_comm_la_SOURCES += communications/network_interface.h
comm_comm_la_SOURCES += communications/mpi_network_message.cpp
comm_comm_la_SOURCES += communications/mpi_network_message.h
comm_comm_la_SOURCES += communications/market_network_interface.cpp
comm_comm_la_SOURCES += communications/market_network_interface.h
comm_comm_la_SOURCES += communications/controller_network_interface.cpp
comm_comm_la_SOURCES += communications/controller_network_interface.h
comm_comm_la_SOURCES += communications/transmission_interface.cpp
comm_comm_la_SOURCES += communications/transmission_interface.h
comm_comm_la_SOURCES += communications/network_message.cpp
comm_comm_la_SOURCES += communications/network_message.h
