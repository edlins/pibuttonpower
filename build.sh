#!/bin/bash
g++ -I/usr/local/include -L/usr/local/lib -lwiringPi -o"pibuttonpower" main.cpp
#sudo mv pibuttonpower /usr/sbin
