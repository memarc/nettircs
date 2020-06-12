#include <sys/types.h> /*r size_t, ssize_t */
#include <stdarg.h> /* va_list */
#include <string.h> /* strcmp */
#include <stdint.h> /* int64_t */
#include <stdlib.h> /* free */
#include <stdio.h>

#include <kcgi.h>
#include <kcgihtml.h>

#include "global.h"
#include "config.h"

// Actually not markdown entirely.
// Just a few pieces of it for simplicity
static FILE * fp;

void
open_md(const char * path)
{
	fp = fopen(path, "r");
}

void
get_md_link(char * buf, char * url, char * title)
{
	char c;
	int n = 0;

	c = getc(fp);

	while (c != ']' && n < 254) {
		buf[n++] = c;
		c = getc(fp);
		if (c == '\\') {
			c = getc(fp);
			buf[n++] = c;
			c = getc(fp);
		}
	}
	n = 0;
	c = getc(fp);
	if (c == '(') {
		c = getc(fp);
		while (c != ' ' && c != ')') {
			url[n++] = c;
			c = getc(fp);
			if (c == '\\') {
				c = getc(fp);
				url[n++] = c;
				c = getc(fp);
			}
		}
		if (c == ' ') {
			c = getc(fp);
			n = 0;
			while (c != ')') {
				title[n++] = c;
				c = getc(fp);
				if (c == '\\') {
					c = getc(fp);
					title[n++] = c;
					c = getc(fp);
				}
			}
		}
	}
}

int
read_md(struct khtmlreq *r)
{
	char c;
	char buf[255];
	char url[255];
	char title[255];
	int n;
	int emph = 0;
	int strike = 0;
	int li = 0, ul = 0, ol = 0;
	int p = 0;
	int cr = 0;

	/* perform operations on the FILE * */
	if (!fp)
		return 1;
	c = getc(fp);
	while (c != EOF) {
		n = 0;
		switch (c) {
		case '\\':
			c = getc(fp);
			if (c == '\n')
				khtml_elem(r, KELEM_BR);
			else {
				if (cr > 1 && !ol && !ul) {
					cr = 0;
					p = 1;
					khtml_elem(r, KELEM_P);
				}
				khtml_putc(r, c);
			}
			break;
		case '#':
			c = getc(fp);
			while (c == '#') {
				n++;
				c = getc(fp);
			}
			switch (n) {
			case 0:
				khtml_elem(r, KELEM_H6);
				break;
			case 1:
				khtml_elem(r, KELEM_H5);
				break;
			case 2:
				khtml_elem(r, KELEM_H4);
				break;
			case 3:
				khtml_elem(r, KELEM_H3);
				break;
			case 4:
				khtml_elem(r, KELEM_H2);
				break;
			case 5:
				khtml_elem(r, KELEM_H1);
				break;
			}
			while (c != '\n') {
				khtml_putc(r, c);
				c = getc(fp);
			}
			khtml_closeelem(r, 1); /* h* */
			break;
		case '*':
			c = getc(fp);
			if (c == '*') {
				if (emph & 0x10) {
					emph = emph & 0x01;
					khtml_closeelem( r, 1);
				} else {
					if (cr > 1 && !ol && !ul) {
						p = 1;
						khtml_elem(r, KELEM_P);
					}
					emph = emph | 0x10;
					khtml_elem(r, KELEM_EM);
				}
			} else {
				ungetc(c, fp);
				if (emph & 0x01) {
					emph = emph & 0x10;
					khtml_closeelem( r, 1);
				} else {
					if (cr > 1 && !ol && !ul) {
						p = 1;
						khtml_elem(r, KELEM_P);
					}
					emph = emph | 0x01;
					khtml_elem(r, KELEM_B);
				}
			}
			cr = 0;
			break;
		case '~':
			if (strike) {
				strike = 0;
				khtml_closeelem(r, 1);
			} else {
				if (cr > 1 && !ol && !ul) {
					p = 1;
					khtml_elem(r, KELEM_P);
				}
				strike = 1;
				khtml_attr(r, KELEM_SPAN,
					KATTR_CLASS, "strikeout",
					KATTR__MAX);
			}
			break;
		case '[':
			// If your urls are too long
			// I won't link to you
			memset(url, '\0', sizeof(url));
			memset(buf, '\0', sizeof(buf));
			memset(title, '\0', sizeof(title));
			get_md_link(buf, url, title);
			khtml_attr(r, KELEM_A,
				KATTR_HREF, url,
				KATTR_TITLE, title,
				KATTR__MAX);
			khtml_puts(r, buf);
			khtml_closeelem(r, 1); 
			// not sure why. A sometimes forces para-end
			cr = 0;
			break;
		case '!':
			// same as link basically
			// get rid of '['
			c = getc(fp);
			memset(url, '\0', sizeof(url));
			memset(buf, '\0', sizeof(buf));
			memset(title, '\0', sizeof(title));
			get_md_link(buf, url, title);
			khtml_attr(r, KELEM_A,
				KATTR_HREF, url,
				KATTR_TITLE, title,
				KATTR__MAX);
			khtml_attr(r, KELEM_IMG,
				KATTR_SRC, url,
				KATTR_ALT, buf,
				KATTR__MAX);
			khtml_closeelem(r, 1);
			khtml_elem(r, KELEM_BR);
			break;
		case '-':
			if (!cr) {
				khtml_putc(r, c);
				break;
			}
			if (!ul) {
				khtml_elem(r, KELEM_UL);
				ul = 1;
			}
			khtml_elem(r, KELEM_LI);
			li = 1;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (!cr) {
				khtml_putc(r, c);
				break;
			}
			if (!ol) {
				khtml_elem(r, KELEM_OL);
				ol = 1;
			}
			khtml_elem(r, KELEM_LI);
			li = 1;
			break;
		case '`':
			n++;

			c = getc(fp);
			if (c == '`') {
				n++;
				c = getc(fp);
			}

			p = 0;
			khtml_elem(r, KELEM_PRE);
			khtml_elem(r, KELEM_CODE);
			while (n > 0) {
				khtml_putc(r, c);
				c = getc(fp);
				if (c == '`') {
					if (n == 2) {
						c = getc(fp);
						if (c == '`')
							n = 0;
						else
							khtml_putc(r, '`');
					} else
						n = 0;
				}
			}
			khtml_closeelem(r, 2); /* pre, code */
			break;
		default:
			if (c == '\n') {
				if (li) {
					li = 0;
					khtml_closeelem(r, 1);
				}
				if (cr && (p || ul || ol)) {
					khtml_closeelem(r, 1);
					ul = 0;
					ol = 0;
					p = 0;
				}
				cr++;
				khtml_putc(r, ' ');
			} else {
				if (cr > 1 && !ol && !ul) {
					p = 1;
					khtml_elem(r, KELEM_P);
				}
				cr = 0;
				khtml_putc(r, c);
			}
		}
		c = getc(fp);
	}
	if (p || ul || ol)
		khtml_closeelem(r, 1);
	fclose(fp);
	return 0;
}
