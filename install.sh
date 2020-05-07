#!/bin/bash

###### INSTALLATION SCRIPT #######

echo "======== Install dependencies =========="
echo "Install libtac"
echo "----------------------------------------"
echo "apt install zip -y"
apt install zip -y
echo "----------------------------------------"

echo "cd ./dependencies"
cd ./dependencies
echo "----------------------------------------"

echo "unzip pam_tacplus-1.4.1.zip"
unzip pam_tacplus-1.4.1.zip
echo "----------------------------------------"

echo "apt-get update"
apt-get update
echo "----------------------------------------"

echo "cd ./pam_tacplus-1.4.1"
cd ./pam_tacplus-1.4.1
echo "----------------------------------------"

echo "apt install libtool autoconf make cmake libpam-dev lipcap-dev pkg-config openssl libssl-dev\n"
apt install libtool -y
apt install autoconf -y
apt install make -y
apt install cmake -y
apt install libpam-dev -y
apt install libcap-dev -y
apt install pkg-config -y
apt install openssl -y
apt install libssl-dev -y
echo "----------------------------------------"

echo "apt-get update"
apt-get update
echo "----------------------------------------"

echo "-------- Install pam_tacplus ----------"
echo "./auto.sh && ./configure && make -j4 && make install"
./auto.sh && ./configure && make -j4 && make install
echo "----------------------------------------"

echo "cd ../.."
cd ../..
echo "----------------------------------------"

echo "-------- Install libaudit -----------"
echo "apt install libaudit-dev -y"
apt install libaudit-dev -y
echo "----------------------------------------"

echo "-------- Install libreadline ---------"
echo "apt install libreadline-dev -y"
apt install libreadline-dev -y
echo "----------------------------------------"

echo "cp /usr/local/lib/libtac.la  /usr/local/lib/libtac.so  /usr/local/lib/libtac.so.2  /usr/local/lib/libtac.so.2.0.0 /usr/lib/"

cp /usr/local/lib/libtac.la  /usr/local/lib/libtac.so  /usr/local/lib/libtac.so.2  /usr/local/lib/libtac.so.2.0.0 /usr/lib/
echo "----------------------------------------"


echo "======== Build CLI =========="
echo "cmake . && make -j4 && make install"
cmake . && make -j4 && make install
echo "----------------------------------------"

echo "cp /usr/local/lib/libsonic-cli.so* /usr/lib/"
cp /usr/local/lib/libsonic-cli.so* /usr/lib/
echo "----------------------------------------"

echo "======== DONE! ========"
echo "run sonic-cli: $ cli"
 




