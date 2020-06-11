#include <err.h> /* err(3) */
#include <stdlib.h> /* EXIT_xxx */
#include <sys/types.h> /* size_t, ssize_t */
#include <stdarg.h> /* va_list */
#include <stdint.h> /* int64_t */
#include <unistd.h> /* pledge(2) */
#include <string.h> /* strlcpy */

#include <kcgi.h>
#include <kcgihtml.h>

#include "home.h"
#include "comments.h"
#include "global.h"

static enum khttp
sanitise(const struct kreq *r)
{
	if (r->page >= PAGE__MAX)
		return KHTTP_404;
	else if (r->mime != KMIME_TEXT_HTML)
		return KHTTP_404;
	else if (r->method != KMETHOD_GET &&
		r->method != KMETHOD_POST &&
		r->method != KMETHOD_OPTIONS)
		return KHTTP_405;
	return KHTTP_200;
}

int
main(void)
{
	struct kreq req;
	struct khtmlreq	r;
	enum khttp er;
	char sub[255];

	// TODO: Take static variables out of sql, return them.

	// pledges required for sql init
	if (pledge("stdio rpath cpath wpath "
		"flock proc fattr", NULL) == -1)
		err(EXIT_FAILURE, NULL);
	// initialize sql connection
	// You need to touch the database first
	if (!init_sql(sqlite_path))
		err(EXIT_FAILURE, NULL);

	// pledges required for kcgi init
	if (pledge("stdio proc rpath", NULL) == -1)
		err(EXIT_FAILURE, NULL);
	// initialize (parse?) khttp
	if (khttp_parse(&req, keys, KEY__MAX,
		pages, PAGE__MAX, PAGE_INDEX) != KCGI_OK)
		err(EXIT_FAILURE, NULL);

	// rpath to open proper file for the page
	if (pledge("stdio rpath", NULL) == -1)
		err(EXIT_FAILURE, NULL);

	if (req.fieldmap[KEY_SUB] != NULL)
		strlcpy(sub, req.fieldmap[KEY_SUB]->parsed.s, sizeof(sub));
	else
		strlcpy(sub, "", sizeof(sub));
	open_md(req.page, sub);

	// rpath to open proper file for the page
	if (pledge("stdio", NULL) == -1)
		err(EXIT_FAILURE, NULL);

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
		sendpage(&req, sub);
	}

	khttp_free(&req);
	free_sql();

	return 0;
}
