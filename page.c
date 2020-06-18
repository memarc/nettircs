#include <unistd.h> /* pledge(2) */
#include <stdio.h>
#include <string.h>
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

void
send_ptitle(struct khtmlreq * r,
	int p,
	const char * proj,
	const char * file,
	const char * rev)
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
	if (strcmp(rev, "") != 0) {
		strlcat(url, "&rev=", sizeof(url));
		strlcat(url, rev, sizeof(url));
		khtml_puts(r, "&rev=");
		khtml_attr(r, KELEM_A, KATTR_HREF, url, KATTR__MAX);
		khtml_puts(r, rev);
		khtml_closeelem(r, 1); /* a */
	}
	khtml_closeelem(r, 1); /* h3 */
}

static void
send_project_list(struct khtmlreq * r, int page, const char * ppath)
{
	// read directory
	struct dirent * de;
	DIR * dir;
	FILE * fp;
	char buf[255];
	char filepath[255];

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

void
sendfile(struct khtmlreq * r, struct kreq * req, const char * ppath)
{
	FILE * fp;
	char buf[255];
	char file[255];
	char rev[255];
	char filepath[255];
	char cmd[512];
	char c;

	if (req->fieldmap[KEY_REV] != NULL) {
		strlcpy(rev,
			req->fieldmap[KEY_REV]->parsed.s,
			sizeof(rev));
	} else
		rev[0] = '\0';
	strlcpy(file, req->fieldmap[KEY_FILE]->parsed.s, sizeof(file));
	sprintf(filepath, "%s/%s/RCS/%s", ppath, req->path, file);
	send_ptitle(r, req->page, req->path, file, rev);
	if (access(filepath, F_OK) != -1) {
		if (strcmp(rev, "") != 0) {
			sprintf(cmd, "co -p -r%s %s", rev, filepath);
			fp = popen(cmd, "r");
			khtml_elem(r, KELEM_CODE);
			khtml_elem(r, KELEM_PRE);
			while (fgets(buf, sizeof(buf), fp) != NULL)
				khtml_puts(r, buf);
			pclose(fp);
			return;
		}
		sprintf(cmd, "rlog -r: %s", filepath);
		fp = popen(cmd, "r");
		if (fp == NULL)
			khtml_printf(r, "\"%s\" not found", filepath);
		else {
			while (fgets(buf, sizeof(buf), fp) != NULL)
				if (buf[0] == '-')
					break;
			// print info on each version
			khtml_elem(r, KELEM_TABLE);
			khtml_elem(r, KELEM_TR);
			khtml_elem(r, KELEM_TH);
			khtml_puts(r, "Rev");
			khtml_closeelem(r, 1);
			khtml_elem(r, KELEM_TH);
			khtml_puts(r, "Date");
			khtml_closeelem(r, 1);
			khtml_elem(r, KELEM_TH);
			khtml_puts(r, "Submitter");
			khtml_closeelem(r, 1);
			khtml_elem(r, KELEM_TH);
			khtml_puts(r, "Lines");
			khtml_closeelem(r, 1);
			khtml_elem(r, KELEM_TH);
			khtml_puts(r, "Description");
			khtml_closeelem(r, 2);
			while (fscanf(fp, "%s", buf) > 0) {
				khtml_elem(r, KELEM_TR);
				khtml_elem(r, KELEM_TD);
				// First line is revision number
				fscanf(fp, "%s\n", buf);
				sprintf(filepath, "?file=%s&rev=%s",
					file, buf);
				khtml_attr(r, KELEM_A,
					KATTR_HREF, filepath,
					KATTR__MAX);
				khtml_puts(r, buf);
				khtml_closeelem(r, 2);

				// all these fscanfs remove labels
				fscanf(fp, "%s", buf);
				khtml_elem(r, KELEM_TD);
				c = getc(fp);
				while (c != ';') {
					khtml_putc(r, c);
					c = getc(fp);
				}
				khtml_closeelem(r, 1);

				// and reading until the ';'
				// means you get the data
				// but not the pesky ';'
				fscanf(fp, "%s", buf);
				khtml_elem(r, KELEM_TD);
				c = getc(fp);
				while (c != ';') {
					khtml_putc(r, c);
					c = getc(fp);
				}
				khtml_closeelem(r, 1);

				c = getc(fp);
				while (c != ';')
					c = getc(fp);
				khtml_elem(r, KELEM_TD);
				c = getc(fp);
				if (c != '\n') {
					fscanf(fp, "%s", buf);
					c = getc(fp);
					while (c != ';') {
						khtml_putc(r, c);
						c = getc(fp);
					}
				}
				khtml_closeelem(r, 1);

				khtml_attr(r, KELEM_TD,
					KATTR_STYLE, "max-width: 300px;",
					KATTR__MAX);
				fgets(buf, sizeof(buf), fp);
				while (buf[0] != '-' && buf[0] != '=') {
					khtml_puts(r, buf);
					fgets(buf, sizeof(buf), fp);
				}
				khtml_closeelem(r, 2);
			}
			khtml_closeelem(r, 1);
			pclose(fp);
		}
	}
}

void
senddirlist(struct khtmlreq * r, struct kreq * req, const char * ppath)
{
	FILE * fp;
	struct dirent *de;
	DIR *dir;
	int b;
	int yr, mo, day, hr, min, sec;
	char buf[255];
	char cmd[512];
	char filepath[255];
	char projectpath[255];

	send_ptitle(r, req->page, req->path, "", "");
	sprintf(filepath, "/projects/%s/%s/%s.rcs.tar.gz",
		pages[req->page], req->path, req->path);
	khtml_attr(r, KELEM_A, KATTR_HREF, filepath, KATTR__MAX);
	khtml_ncr(r, 0x2193); /* darr */
	khtml_printf(r, " %s.rcs.tar.gz", req->path);
	khtml_closeelem(r, 1); /* a */
	sprintf(projectpath, "%s/%s/RCS", ppath, req->path);
	dir = opendir(projectpath);
	if (dir == NULL)
		khtml_printf(r, "\"%s\" does not exist", projectpath);
	else {
		khtml_elem(r, KELEM_TABLE);
		khtml_elem(r, KELEM_THEAD);
		khtml_elem(r, KELEM_TH);
		khtml_puts(r, "File");
		khtml_closeelem(r, 1);
		khtml_elem(r, KELEM_TH);
		khtml_puts(r, "Revision");
		khtml_closeelem(r, 1);
		khtml_elem(r, KELEM_TH);
		khtml_puts(r, "Submitter");
		khtml_closeelem(r, 1);
		khtml_elem(r, KELEM_TH);
		khtml_puts(r, "Date");
		khtml_closeelem(r, 2);
		while ((de = readdir(dir)) != NULL) {
			if (de->d_name[0] != '.') {
				khtml_elem(r, KELEM_TR);
				khtml_elem(r, KELEM_TD);
				sprintf(filepath,
					"?file=%s", de->d_name);
				khtml_attr(r, KELEM_A,
					KATTR_HREF, filepath,
					KATTR__MAX);
				khtml_puts(r, de->d_name);
				khtml_closeelem(r, 2);

				khtml_elem(r, KELEM_TD);
				sprintf(filepath, "%s/%s",
					projectpath, de->d_name);
				fp = fopen(filepath, "r");
				memset(buf, '\0', sizeof(buf));
				fgets(buf, sizeof(buf), fp);
				for (b = 0; b < 255; b++)
					if (buf[b] == ';') {
						buf[b] = '\0';
						break;
					}
				khtml_puts(r, buf);
				khtml_closeelem(r, 1);
				khtml_elem(r, KELEM_TD);
				while (strcmp(buf, "\n") != 0)
					fgets(buf, sizeof(buf), fp);
				fscanf(fp, " ");
				fgets(buf, sizeof(buf), fp);
				fscanf(fp, "date %d.%d.%d.%d.%d.%d;",
					&yr, &mo, &day,
					&hr, &min, &sec);
				fscanf(fp, " author %s;", buf);
				for (b = 0; b < 255; b++)
					if (buf[b] == ';') {
						buf[b] = '\0';
						break;
					}
				khtml_puts(r, buf);
				khtml_closeelem(r, 1);
				khtml_elem(r, KELEM_TD);
				khtml_printf(r, "%d-%02d-%02d ",
					yr, mo, day);
				khtml_attr(r, KELEM_SPAN,
					KATTR_CLASS, "time",
					KATTR__MAX);
				khtml_printf(r, " %02d:%02d.%02d",
					hr, min, sec);
				fclose(fp);
				khtml_closeelem(r, 3);
			}
		}
		khtml_closeelem(r, 1); /* table */
		sprintf(filepath, "%s%s/%s/RCS/README.md,v",
			project_path, pages[req->page], req->path);
		if (access(filepath, F_OK) != -1) {
			sprintf(cmd, "/bin/co -p %s", filepath);
			fp = popen(cmd, "r");
			if (fp == NULL)
				khtml_printf(r, "Couldn't execute \"%s\"", cmd);
			else {
				khtml_attr(r, KELEM_H3,
					KATTR_CLASS, "readmetitle",
					KATTR__MAX);
				khtml_puts(r, "README.md:");
				khtml_closeelem(r, 1);
				khtml_attr(r, KELEM_DIV,
					KATTR_CLASS, "readme",
					KATTR__MAX);
				read_md(r, fp);
				khtml_closeelem(r, 1);
				pclose(fp);
			} 
		}

		khtml_elem(r, KELEM_P);
		khtml_puts(r, "Assuming you own this reopsitory, to "
			"upload your RCS directory, type:");
		khtml_closeelem(r, 1);
		khtml_elem(r, KELEM_CODE);
		khtml_elem(r, KELEM_PRE);
		khtml_puts(r, "scp -r RCS "
			"rcs.yksinotso.org:/tmp/RCS\n");
		khtml_printf(r, "ssh rcs.yksinotso.org"
			" ~/bin/nettircs.sh /tmp/RCS %s %s",
			pages[req->page], req->path);
	}
}
