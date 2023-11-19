#!/bin/sh
# executed by `post-install.txt` as `chroot /rootfs find /tmp/extras -type f -exec sh {} \;`
# executed by user as `sudo ./netinst.sh`

echo ""
echo "=== Adding pibuttonpower ==="

# setup kernel option to allow wiringPiSetup() to pass
echo ""
echo "= Adding iomem=relaxed"
sed -i '1{s/$/ iomem=relaxed/}' /boot/cmdline.txt
cat /boot/cmdline.txt

# install sudo
echo ""
echo "= Adding sudo"
/usr/bin/apt-get -y --no-install-recommends install sudo

# install make
echo ""
echo "= Adding make"
/usr/bin/apt-get -y --no-install-recommends install make

# install g++
echo ""
echo "= Adding g++"
/usr/bin/apt-get -y --no-install-recommends install g++

# download, unzip, build, and install pigpio
wget https://github.com/joan2937/pigpio/archive/master.zip
unzip master.zip
cd pigpio-master
make
make install

# download pibuttonpower
echo ""
echo "= Cloning pibuttonpower"
/usr/bin/git clone git://github.com/edlins/pibuttonpower /usr/local/src/pibuttonpower

# build and install pibuttonpower
cd /usr/local/src/pibuttonpower
echo ""
echo "= Building pibuttonpower"
./build.sh
echo ""
echo "= Installing pibuttonpower"
./install.sh
