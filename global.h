#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>

int read_md(struct khtmlreq *, FILE *);
void sendpng(struct khtmlreq *,
	const char *,
	const char *,
	const char *);

#endif
