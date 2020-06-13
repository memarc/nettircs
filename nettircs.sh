#!/bin/sh

if [ -z "$1" -o -z "$2" -o -z "$3" ]
then
	echo "Syntax: ./$0 RCS_dir nrcsuser project"
	exit
fi


PROJECT_PATH=/var/www/htdocs/nettircs/projects/
mv $1 $PROJECT_PATH/$2/$3
cd $PROJECT_PATH/$2/$3
rcs -u RCS/*
[ -e "RCS/README.md,v" ] && co README.md
tar -czf $3.rcs.tar.gz RCS
