#!/bin/bash
#g++ -I/usr/local/include -L/usr/local/lib -lwiringPi -o"pibuttonpower" main.cpp
g++ -I/usr/local/include -L/usr/local/lib -lrt -lpthread -o"pibuttonpower" main.cpp -lpigpio
#sudo mv pibuttonpower /usr/sbin
