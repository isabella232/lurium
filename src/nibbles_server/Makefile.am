AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = nibbles_server

EXTRA_DIST = default0.ini small0.ini

nibbles_server_SOURCES = main.cpp nibbles_server.cpp nibbles_bot.cpp nibbles.cpp ../control_client.cpp
nibbles_server_SOURCES +=  nibbles_server.h nibbles_bot.h nibbles.h

nibbles_server_CPPFLAGS = -std=c++11 -I ../../include -O2
nibbles_server_LDADD = -lboost_system -lboost_chrono -lpthread
