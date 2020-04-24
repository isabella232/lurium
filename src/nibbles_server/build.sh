#!/bin/bash
g++ -std=c++11 -I ../include -O2 main.cpp nibbles_server.cpp nibbles_bot.cpp nibbles.cpp control_client.cpp -lboost_system -lboost_chrono -lboost_thread -lpthread

#g++ -std=c++11 -I include -lpthread -fpermissive nibbles.cpp nibbles_server.cpp main.cpp 
