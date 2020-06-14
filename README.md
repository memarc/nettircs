#NettiRCS

A program for displaying RCS projects on the netti.

###Dependencies

-RCS (optional for the web app actually, but necessary for the helper
script)
-[kcgi](https://kristaps.bsd.lv/kcgi/)
-pkg-config (or edit *config.mk* to point at the libraries manually)

###Installation
Check *Makefile* and set the htdocs for your system's web-root.
Then edit *config.h* and set the paths for your system. Finally, just 
`doas make clean install`

Copy the directory to your web-root, and point your server at it.

Here's */etc/httpd.conf*:
`
ext_ip = "*"

server "localhost" {
	listen on $ext_ip port 80

	root "/htdocs/nettircs"

	location "/" {
		block return 301 "/cgi-bin/nettircs"
	}
	location "/cgi-bin/*" {
		fastcgi
		root "/"
	}
}

#[ TYPES ]
types {
	include "/usr/share/misc/mime.types"
}
`

Finally, copy the script (*nettircs.sh*) to a place that will be easy to execute.
I leave mine in $HOME/bin/

