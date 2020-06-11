#ifndef CONFIG_H
#define CONFIG_H

#include <kcgi.h>
#include <kcgihtml.h>

static const char * error_path = "/media/plotso.png";
static const char * site_title = "NettiRcs";
static const char * site_title_bold = "netti";
static const char * site_title_regu = "rcs";
static const char * cgi_exec = "netti_rcs";
static const char * doc_path = "/htdocs/nettircs/docs/%s.md";
static const char * sqlite_path = "/htdocs/nettircs/netti.db";

enum	page {
	PAGE_INDEX,
	PAGE__MAX
};

// page names
// these will also be used as the md names
static const char *const pages[PAGE__MAX] = {
	"index",
};

#endif
