pkglib_LTLIBRARIES += comm/comm.la

comm_comm_la_CPPFLAGS =
comm_comm_la_CPPFLAGS += $(AM_CPPFLAGS)
comm_comm_la_CPPFLAGS += $(FNCS_CPPFLAGS)

comm_comm_la_LDFLAGS =
comm_comm_la_LDFLAGS += $(AM_LDFLAGS)
comm_comm_la_LDFLAGS += $(FNCS_LDFLAGS)

comm_comm_la_LIBADD = 
comm_comm_la_LIBADD += $(FNCS_LIBS)

comm_comm_la_SOURCES =
comm_comm_la_SOURCES += comm/comm.h
comm_comm_la_SOURCES += comm/init.cpp
comm_comm_la_SOURCES += comm/main.cpp
comm_comm_la_SOURCES += comm/network.cpp
comm_comm_la_SOURCES += comm/network.h
comm_comm_la_SOURCES += comm/network_interface.cpp
comm_comm_la_SOURCES += comm/network_interface.h
comm_comm_la_SOURCES += comm/mpi_network_message.cpp
comm_comm_la_SOURCES += comm/mpi_network_message.h
comm_comm_la_SOURCES += comm/market_network_interface.cpp
comm_comm_la_SOURCES += comm/market_network_interface.h
comm_comm_la_SOURCES += comm/controller_network_interface.cpp
comm_comm_la_SOURCES += comm/controller_network_interface.h
comm_comm_la_SOURCES += comm/transmissioncom.cpp
comm_comm_la_SOURCES += comm/transmissioncom.h
comm_comm_la_SOURCES += comm/network_message.cpp
comm_comm_la_SOURCES += comm/network_message.h
