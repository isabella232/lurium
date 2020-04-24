AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = control_server

nobase_include_HEADERS =

control_server_SOURCES = main.cpp control_server.cpp
control_server_SOURCES += control_server.h

control_server_CPPFLAGS = -std=c++11 -I ../../include -O2
control_server_LDADD = -lboost_system -lboost_chrono -lpthread
