#!/bin/bash
g++ -std=c++11 -I ../include -O2 -o control main.cpp control_server.cpp -lboost_system -lboost_chrono -lpthread

#g++ -std=c++11 -I include -lpthread -fpermissive nibbles.cpp nibbles_server.cpp main.cpp 
