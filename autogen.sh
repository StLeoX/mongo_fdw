#! /bin/bash

#-------------------------------------------------------------------------
#
# autogen.sh
#
#-------------------------------------------------------------------------

##
# Pull the latest version of Monggo C Driver's master branch
#
function checkout_mongo_driver
{
	rm -rf mongo-c-driver
	wget https://github.com/mongodb/mongo-c-driver/releases/download/1.18.0/mongo-c-driver-1.18.0.tar.gz
	tar -zxvf mongo-c-driver-1.18.0.tar.gz
	mv mongo-c-driver-1.18.0 mongo-c-driver
	rm -rf mongo-c-driver-1.18.0.tar.gz
}

##
# Configure and install the Mongo C Driver and libbson
#
function install_mongoc_driver
{
	cd mongo-c-driver
	./autogen.sh
	configure --with-libbson=system
	make install
	cd ..
}

##
# Pull the json-c library
#
function checkout_json_lib
{
	echo $PWD
	rm -rf json-c
	wget https://github.com/json-c/json-c/archive/json-c-0.12-20140410.tar.gz
	tar -zxvf json-c-0.12-20140410.tar.gz
	mv json-c-json-c-0.12-20140410 json-c
	cd json-c
	patch -p1 < ../json_compilation_error.patch
	cd ..
	rm -rf json-c-0.12-20140410.tar.gz
	echo $PWD
}

##
# Compile and instal json-c library
#
function install_json_lib
{
	cd json-c
	sh ./autogen.sh
	./configure
	make install
	cd ..
}

# main
checkout_mongo_driver
install_mongoc_driver
export PKG_CONFIG_PATH=mongo-c-driver/src/:mongo-c-driver/src/libbson/src
checkout_json_lib
install_json_lib
echo "Done"
