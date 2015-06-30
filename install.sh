#!/bin/bash
cp pibuttonpower /usr/sbin
cp init.d/pibuttonpower /etc/init.d
chmod 755 /etc/init.d/pibuttonpower
update-rc.d pibuttonpower defaults
