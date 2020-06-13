#!/bin/sh

if [ -z "$1" -o -z "$2" ]
then
	echo "Syntax: ./$0 nrcsuser project"
	exit
fi


PROJECT_PATH=/var/www/htdocs/nettircs/projects/
cd $PROJECT_PATH/$1/$2
rcs -u RCS/*
[ -e "RCS/README.md,v" ] && co README.md
tar -czf $2.rcs.tar.gz RCS
