AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = scorch_server

EXTRA_DIST = default0.ini small0.ini 

scorch_server_SOURCES = main.cpp scorch_server.cpp scorch_bot.cpp scorch.cpp ../control_client.cpp
scorch_server_SOURCES += scorch_message.h scorch_server.h scorch_bot.h ground.h scorch.h

scorch_server_CPPFLAGS = -std=c++11 -I ../../include -O2
scorch_server_LDADD = -lboost_system -lboost_chrono -lpthread
