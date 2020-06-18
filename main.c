#include <err.h> /* err(3) */
#include <stdlib.h> /* EXIT_xxx */
#include <sys/types.h> /* size_t, ssize_t */
#include <stdarg.h> /* va_list */
#include <stdint.h> /* int64_t */
#include <unistd.h> /* pledge(2) */
#include <string.h> /* strlcpy */
#include <stdio.h> /* sprintf */
#include <sys/stat.h> /* stat */

#include <kcgi.h>
#include <kcgihtml.h>

#include "global.h"
#include "page.h"
#include "config.h"

static const struct kvalid keys[KEY__MAX] = {
	{ kvalid_stringne, "file" },
};


void
sendheader(struct khtmlreq *r, int p)
{
	char	url[255];
	int	l;

	khtml_elem(r, KELEM_DOCTYPE);
	khtml_elem(r, KELEM_HTML);
	khtml_elem(r, KELEM_HEAD);
	khtml_attr(r, KELEM_META,
		KATTR_CHARSET, "UTF-8", KATTR__MAX);
	khtml_elem(r, KELEM_TITLE);
	khtml_puts(r, site_title);
	if (p != PAGE_INDEX) {
		khtml_puts(r, " - ");
		khtml_puts(r, pages[p]);
	}
	khtml_closeelem(r, 1); /* title */
	khtml_attr(r, KELEM_LINK,
		KATTR_REL, "stylesheet",
		KATTR_TYPE, "text/css",
		KATTR_HREF, "/main.css", KATTR__MAX);
	khtml_closeelem(r, 1); /* head */
	khtml_elem(r, KELEM_BODY);
	khtml_attr(r, KELEM_DIV,
		KATTR_CLASS, "header-bar", KATTR__MAX);
	khtml_elem(r, KELEM_B);
	khtml_puts(r, site_title_bold);
	khtml_closeelem(r, 1); /* b */
	khtml_puts(r, site_title_regu);
	khtml_closeelem(r, 1); /* div */
	khtml_attr(r, KELEM_DIV,
		KATTR_ID, "nav-bar", KATTR__MAX);
	for (l = 0; l < PAGE__MAX; l++) {
		sprintf(url, "/cgi-bin/%s/%s", cgi_exec, pages[l]);
		if (l == p)
			khtml_attr(r, KELEM_A,
				KATTR_HREF, url,
				KATTR_CLASS, "active",
				KATTR__MAX);
		else
			khtml_attr(r, KELEM_A,
				KATTR_HREF, url,
				KATTR__MAX);
		khtml_puts(r, pages[l]);
		khtml_puts(r, " ");
		khtml_closeelem(r, 1); /* a */
	}
	khtml_closeelem(r, 1); /* div */
	khtml_attr(r, KELEM_DIV,
		KATTR_ID, "main", KATTR__MAX);
}


void
resp_open(struct kreq *req, enum khttp http)
{
	enum kmime	mime;

	// Default to an octet-stream response
	if (KMIME__MAX == (mime = req->mime))
		mime = KMIME_APP_OCTET_STREAM;

	khttp_head(req, kresps[KRESP_STATUS],
		"%s", khttps[http]);
	khttp_head(req, kresps[KRESP_CONTENT_TYPE],
		"%s", kmimetypes[mime]);
	khttp_body(req);
}

static enum khttp
sanitise(const struct kreq *r)
{
	if (r->page >= PAGE__MAX ||
		(r->path[0] != '\0' && r->page == PAGE_INDEX))
		return KHTTP_404;
	else if (r->mime != KMIME_TEXT_HTML)
		return KHTTP_404;
	else if (r->method != KMETHOD_GET &&
		r->method != KMETHOD_POST &&
		r->method != KMETHOD_OPTIONS)
		return KHTTP_405;
	return KHTTP_200;
}

void
send_ppath(struct khtmlreq * r, int p, char * proj, char * file)
{
	char url[255];
	khtml_elem(r, KELEM_H3);
	sprintf(url, "/cgi-bin/%s/%s", cgi_exec, pages[p]);
	khtml_attr(r, KELEM_A, KATTR_HREF, url, KATTR__MAX);
	khtml_puts(r, pages[p]);
	khtml_closeelem(r, 1); /* a */
	khtml_puts(r, "/");
	strlcat(url, "/", sizeof(url));
	strlcat(url, proj, sizeof(url));
	khtml_attr(r, KELEM_A, KATTR_HREF, url, KATTR__MAX);
	khtml_puts(r, proj);
	khtml_closeelem(r, 1); /* a */
	if (strcmp(file, "") != 0) {
		strlcat(url, "?file=", sizeof(url));
		strlcat(url, file, sizeof(url));
		khtml_puts(r, "?file=");
		khtml_attr(r, KELEM_A, KATTR_HREF, url, KATTR__MAX);
		khtml_puts(r, file);
		khtml_closeelem(r, 1); /* a */
	}
	khtml_closeelem(r, 1); /* h3 */
}

int
main(void)
{
	struct kreq req;
	struct khtmlreq	r;
	enum khttp er;
	char ppath[255];

	// pledges required for kcgi init
	if (pledge("stdio exec proc rpath cpath", NULL) == -1)
		err(EXIT_FAILURE, NULL);
	// initialize (parse?) khttp
	if (khttp_parse(&req, keys, KEY__MAX,
		pages, PAGE__MAX, PAGE_INDEX) != KCGI_OK)
		err(EXIT_FAILURE, NULL);

	// rpath to open proper file for the page, proc for popen
	if (pledge("stdio exec proc rpath", NULL) == -1)
		err(EXIT_FAILURE, NULL);

	if (req.page != PAGE_INDEX) {
		memset(ppath, '\0', sizeof(ppath));
		strlcpy(ppath, project_path, sizeof(ppath));
		strlcat(ppath, pages[req.page], sizeof(ppath));
	}

	/*
	 * Accept only GET
	 * restrict to text/html and a valid page
	 */
	if ((er = sanitise(&req)) != KHTTP_200) {
		resp_open(&req, er);
		if (req.mime == KMIME_TEXT_HTML) {
			khtml_open(&r, &req, 0);
			sendheader(&r, er);
			senderror(&r, er);
			khtml_close(&r);
		}
	} else {
		resp_open(&req, KHTTP_200);
		khtml_open(&r, &req, 0);

		sendheader(&r, req.page);
		if (req.page == PAGE_INDEX ||
			req.path == NULL ||
			req.path[0] == '\0')
			sendindex(&r, req.page, ppath);
		else if (req.fieldmap[KEY_FILE] != NULL)
			sendfile(&r, &req, ppath);
		else
			senddirlist(&r, &req, ppath);
		khtml_close(&r); /* all scopes */
	}

	khttp_free(&req);

	return 0;
}
