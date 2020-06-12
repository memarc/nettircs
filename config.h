#ifndef CONFIG_H
#define CONFIG_H

#include <kcgi.h>
#include <kcgihtml.h>

static const char * error_path = "/media/plotso.png";
static const char * site_title = "NettiRCS";
static const char * site_title_bold = "Netti";
static const char * site_title_regu = "RCS";
static const char * cgi_exec = "netti_rcs";
static const char * doc_path = "/htdocs/nettircs/docs/%s.md";
static const char * sqlite_path = "/htdocs/nettircs/netti.db";
static const char * project_path = "/htdocs/nettircs/projects/";

// add a user's folders after index
enum	page {
	PAGE_INDEX,
	PAGE_OTSO,
	PAGE__MAX
};

// page names
// these will also be used as the md names
static const char *const pages[PAGE__MAX] = {
	"index",
	"otso",
};

#endif
