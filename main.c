#include <err.h> /* err(3) */
#include <stdlib.h> /* EXIT_xxx */
#include <sys/types.h> /* size_t, ssize_t */
#include <stdarg.h> /* va_list */
#include <stdint.h> /* int64_t */
#include <unistd.h> /* pledge(2) */
#include <string.h> /* strlcpy */
#include <stdio.h> /* sprintf */
#include <dirent.h> /* opendir */
#include <sys/stat.h> /* stat */

#include <kcgi.h>
#include <kcgihtml.h>

#include "global.h"
#include "config.h"

enum	key {
	KEY_FILE,
	KEY__MAX
};

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
senderror(struct khtmlreq *r, enum khttp http)
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
	char mdpath[255];
	char ppath[255];
	// read directory
	struct dirent *de;
	DIR *dir;
	// read file
	char filepath[255];
	char buf[255];
	char date[21];
	int yr, mo, day, hr, min, sec;
	int b;
	FILE *fp;

	// pledges required for kcgi init
	if (pledge("stdio proc rpath cpath", NULL) == -1)
		err(EXIT_FAILURE, NULL);
	// initialize (parse?) khttp
	if (khttp_parse(&req, keys, KEY__MAX,
		pages, PAGE__MAX, PAGE_INDEX) != KCGI_OK)
		err(EXIT_FAILURE, NULL);

	// rpath to open proper file for the page, proc for popen
	if (pledge("stdio proc rpath", NULL) == -1)
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
			req.path == NULL || req.path[0] == '\0') {
			sprintf(mdpath, doc_path, pages[req.page]);
			fp = fopen(mdpath, "r");
			if (read_md(&r, fp) != 0)
				senderror(&r, KHTTP_404);
			fclose(fp);
			// list projects on user pages
			if (req.page != PAGE_INDEX) {
				dir = opendir(ppath);
				if (dir == NULL)
					khtml_printf(&r, "Couldn't open " "\"%s\"", ppath);
				khtml_attr(&r, KELEM_DIV,
					KATTR_CLASS, "project_list",
					KATTR__MAX);
				while ((de = readdir(dir)) != NULL) {
					if (de->d_name[0] != '.') {
						sprintf(filepath, "%s/%s",
							pages[req.page], de->d_name);
						khtml_attr(&r, KELEM_A,
							KATTR_HREF, filepath,
							KATTR__MAX);
						khtml_elem(&r, KELEM_DIV);
						khtml_puts(&r, de->d_name);
						khtml_closeelem(&r, 1);

						sprintf(buf, doc_path, filepath);
						khtml_elem(&r, KELEM_BR);
						khtml_elem(&r, KELEM_DIV);
						if (access(buf, F_OK) != -1) {
							fp = fopen(buf, "r");
							read_md(&r, fp);
							fclose(fp);
						} else
							khtml_puts(&r, "No description found");
						khtml_closeelem(&r, 1);
					}
				}
			}
		} else if (req.fieldmap[KEY_FILE] != NULL) {
			strlcpy(buf, req.fieldmap[KEY_FILE]->parsed.s, sizeof(buf));
			send_ppath(&r, req.page, req.path, buf);
			sprintf(filepath, "%s/%s/RCS/%s", ppath, req.path, buf);
			if (access(filepath, F_OK) != -1) {
				fp = fopen(filepath, "r");
				if (fp == NULL)
					khtml_printf(&r, "Couldn't open \"%s\"", filepath);
				else {
					khtml_elem(&r, KELEM_CODE);
					khtml_elem(&r, KELEM_PRE);
					while (fgets(buf, sizeof(buf), fp))
						khtml_puts(&r, buf);
					khtml_closeelem(&r, 2);
					fclose(fp);
				} 
			}
		} else {
			send_ppath(&r, req.page, req.path, "");

			sprintf(filepath, "/projects/%s/%s/%s.rcs.tar.gz",
				pages[req.page], req.path, req.path);
			khtml_attr(&r, KELEM_A, KATTR_HREF, filepath, KATTR__MAX);
			khtml_ncr(&r, 0x2193); /* darr */
			khtml_printf(&r, " %s.rcs.tar.gz", req.path);
			khtml_closeelem(&r, 1); /* a */

			sprintf(ppath, "%s/%s/RCS", ppath, req.path);

			dir = opendir(ppath);
			if (dir == NULL)
				khtml_printf(&r, "Error: \"%s\" does not exist", ppath);
			else {
				khtml_attr(&r, KELEM_TABLE,
					KATTR_ID, "project_list", KATTR__MAX);
				khtml_elem(&r, KELEM_THEAD);
				khtml_elem(&r, KELEM_TH);
				khtml_puts(&r, "File");
				khtml_closeelem(&r, 1);
				khtml_elem(&r, KELEM_TH);
				khtml_puts(&r, "Revision");
				khtml_closeelem(&r, 1);
				khtml_elem(&r, KELEM_TH);
				khtml_puts(&r, "Submitter");
				khtml_closeelem(&r, 1);
				khtml_elem(&r, KELEM_TH);
				khtml_puts(&r, "Date");
				khtml_closeelem(&r, 2);
				while ((de = readdir(dir)) != NULL) {
					if (de->d_name[0] != '.') {
						khtml_elem(&r, KELEM_TR);
						khtml_elem(&r, KELEM_TD);
						sprintf(filepath, "?file=%s", de->d_name);
						khtml_attr(&r, KELEM_A,
							KATTR_HREF, filepath,
							KATTR__MAX);
						khtml_puts(&r, de->d_name);
						khtml_closeelem(&r, 2);
						khtml_elem(&r, KELEM_TD);

						sprintf(filepath, "%s/%s",
							ppath, de->d_name);
						fp = fopen(filepath, "r");
						memset(buf, '\0', sizeof(buf));
						fgets(buf, sizeof(buf), fp);
						for (b = 0; b < 255; b++)
							if (buf[b] == ';') {
								buf[b] = '\0';
								break;
							}
						khtml_puts(&r, buf);
						khtml_closeelem(&r, 1);
						khtml_elem(&r, KELEM_TD);
						while (strcmp(buf, "\n") != 0)
							fgets(buf, sizeof(buf), fp);
						fscanf(fp, " ");
						fgets(buf, sizeof(buf), fp);
						fscanf(fp, "date %d.%d.%d.%d.%d.%d;",
							&yr, &mo, &day,
							&hr, &min, &sec);
						fscanf(fp, " author %s;", buf);
						for (b = 0; b <= 21; b++)
							if (date[b] == ';') {
								date[b] = '\0';
								break;
							}
						for (b = 0; b < 255; b++)
							if (buf[b] == ';') {
								buf[b] = '\0';
								break;
							}
						khtml_puts(&r, buf);
						khtml_closeelem(&r, 1);
						khtml_elem(&r, KELEM_TD);
						khtml_printf(&r, "%d-%02d-%02d ",
							yr, mo, day);
						khtml_attr(&r, KELEM_SPAN,
							KATTR_CLASS, "time",
							KATTR__MAX);
						khtml_printf(&r, " %02d:%02d.%02d",
							hr, min, sec);
						fclose(fp);
						khtml_closeelem(&r, 3);
					}
				}
				khtml_closeelem(&r, 1); /* table */
				sprintf(filepath, "%s%s/%s/README.md",
					project_path, pages[req.page], req.path);
				if (access(filepath, F_OK) != -1) {
					fp = fopen(filepath, "r");
					if (fp == NULL)
						khtml_printf(&r, "Couldn't open \"%s\"", filepath);
					else {
						khtml_attr(&r, KELEM_H3, KATTR_CLASS, "readmetitle", KATTR__MAX);
						khtml_puts(&r, "README.md:");
						khtml_closeelem(&r, 1);
						khtml_attr(&r, KELEM_DIV, KATTR_CLASS, "readme", KATTR__MAX);
						read_md(&r, fp);
						khtml_closeelem(&r, 1);
						fclose(fp);
					} 
				}

				khtml_elem(&r, KELEM_P);
				khtml_puts(&r, "Assuming you own this "
					"reopsitory, to upload your "
					"RCS directory, type:");
				khtml_closeelem(&r, 1);
				khtml_elem(&r, KELEM_CODE);
				khtml_elem(&r, KELEM_PRE);
				khtml_puts(&r, "scp -r RCS "
					"rcs.yksinotso.org:/tmp/RCS\n");
				khtml_printf(&r, "ssh rcs.yksinotso.org"
					" ~/bin/nettircs.sh /tmp/RCS %s %s",
					pages[req.page], req.path);
			}
		}
		khtml_close(&r); /* all scopes */
	}

	khttp_free(&req);

	return 0;
}
