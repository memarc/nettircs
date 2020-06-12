#ifndef GLOBAL_H
#define GLOBAL_H

void open_md(const char *);
int read_md(struct khtmlreq *);
void sendpng(struct khtmlreq *,
	const char *,
	const char *,
	const char *);

#endif
