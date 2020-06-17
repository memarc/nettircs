#include <unistd.h> /* pledge(2) */
#include <stdio.h>
#include <dirent.h> /* opendir */

#include "page.h"
#include "global.h"
#include "config.h"

void
senderror(struct khtmlreq * r, enum khttp http)
{
	khtml_elem(r, KELEM_H2);
	switch (http) {
	case KHTTP_404:
		khtml_puts(r, "404, page not found");
		break;
	case KHTTP_405:
		khtml_puts(r, "405, invalid method");
		break;
	default:
		khtml_puts(r, "Something went wrong");
	}

	khtml_elem(r, KELEM_P);
	khtml_attr(r, KELEM_IMG,
		KATTR_SRC, error_path, KATTR__MAX);
}

static void
send_project_list(struct khtmlreq * r, int page, const char * ppath)
{
	// read directory
	struct dirent * de;
	DIR * dir;
	FILE * fp;
	char buf[256];
	char filepath[256];

	// list projects on user pages
	dir = opendir(ppath);
	if (dir == NULL)
		khtml_printf(r, "Couldn't open " "\"%s\"", ppath);
	khtml_attr(r, KELEM_DIV,
		KATTR_CLASS, "project_list",
		KATTR__MAX);
	while ((de = readdir(dir)) != NULL) {
		if (de->d_name[0] != '.') {
			sprintf(filepath, "%s/%s",
				pages[page], de->d_name);
			khtml_attr(r, KELEM_A,
				KATTR_HREF, filepath,
				KATTR__MAX);
			khtml_elem(r, KELEM_DIV);
			khtml_puts(r, de->d_name);
			khtml_closeelem(r, 1);

			sprintf(buf, doc_path, filepath);
			khtml_elem(r, KELEM_BR);
			khtml_elem(r, KELEM_DIV);
			if (access(buf, F_OK) != -1) {
				fp = fopen(buf, "r");
				read_md(r, fp);
				fclose(fp);
			} else
				khtml_puts(r, "No description found");
			khtml_closeelem(r, 1);
		}
	}
}

void
sendindex(struct khtmlreq * r, int page, const char * ppath)
{
	char mdpath[255];
	FILE * fp;
	sprintf(mdpath, doc_path, pages[page]);
	fp = fopen(mdpath, "r");
	if (read_md(r, fp) != 0)
		senderror(r, KHTTP_404);
	fclose(fp);
	if (page != PAGE_INDEX)
		send_project_list(r, page, ppath);
}
