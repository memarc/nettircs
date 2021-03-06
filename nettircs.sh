#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$3" ]
then
	echo "Syntax: ./$0 RCS_dir nrcsuser project"
	exit
fi

PROJECT_PATH=/var/www/htdocs/nettircs/projects/
[ -e $PROJECT_PATH/$2/$3 ] && mkdir -p $PROJECT_PATH/$2/$3
rm -rf $PROJECT_PATH/$2/$3/RCS
cp -r $1 $PROJECT_PATH/$2/$3/RCS
rm -fr $1
cd $PROJECT_PATH/$2/$3
rcs -u RCS/*
tar -czf $3.rcs.tar.gz RCS
