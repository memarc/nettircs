#ifndef PAGE_H
#define PAGE_H

enum	key {
	KEY_FILE,
	KEY_REV,
	KEY__MAX
};

#include <stdarg.h> /* va_list */

#include <kcgi.h>
#include <kcgihtml.h>

void senderror(struct khtmlreq *, enum khttp);
void sendindex(struct khtmlreq *, int, const char *);
void sendfile(struct khtmlreq *, struct kreq *, const char *);
void senddirlist(struct khtmlreq *, struct kreq *, const char *);

#endif
