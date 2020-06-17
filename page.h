#ifndef PAGE_H
#define PAGE_H

#include <stdarg.h> /* va_list */

#include <kcgi.h>
#include <kcgihtml.h>

void senderror(struct khtmlreq *, enum khttp);
void sendindex(struct khtmlreq *, int, const char *);

#endif
