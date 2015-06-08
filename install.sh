#!/bin/bash
sudo cp pibuttonpowersvc /etc/init.d
sudo chmod 755 /etc/init.d/pibuttonpowersvc
sudo update-rc.d pibuttonpowersvc defaults
