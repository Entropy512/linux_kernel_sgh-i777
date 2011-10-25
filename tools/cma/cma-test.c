/*
 * cma-test.c -- CMA testing application
 *
 * Copyright (C) 2010 Samsung Electronics
 *                    Author: Michal Nazarewicz <m.nazarewicz@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* $(CROSS_COMPILE)gcc -Wall -Wextra -g -o cma-test cma-test.c  */

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <unistd.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/cma.h>


static void handle_command(char *line);

int main(void)
{
	unsigned no = 1;
	char line[1024];
	int skip = 0;

	fputs("commands:\n"
	      " l or list                                      list allocated chunks\n"
	      " a or alloc  <dev>/<kind> <size>[/<alignment>]  allocate chunk\n"
	      " A or afrom  <regions>    <size>[/<alignment>]  allocate from region(s)\n"
	      " f or free   [<num>]                            free an chunk\n"
	      " # ...                                          comment\n"
	      " <empty line>                                   repeat previous\n"
	      "\n", stderr);

	while (fgets(line, sizeof line, stdin)) {
		char *nl = strchr(line, '\n');
		if (nl) {
			if (skip) {
				fprintf(stderr, "cma: %d: line too long\n", no);
				skip = 0;
			} else {
				*nl = '\0';
				handle_command(line);
			}
			++no;
		} else {
			skip = 1;
		}
	}

	if (skip)
		fprintf(stderr, "cma: %d: no new line at EOF\n", no);
	return 0;
}



static void cmd_list(char *name, char *line);
static void cmd_alloc(char *name, char *line);
static void cmd_alloc_from(char *name, char *line);
static void cmd_free(char *name, char *line);

static const struct command {
	const char name[8];
	void (*handle)(char *name, char *line);
} commands[] = {
	{ "list",  cmd_list },
	{ "l",     cmd_list },
	{ "alloc", cmd_alloc },
	{ "a",     cmd_alloc },
	{ "afrom", cmd_alloc_from },
	{ "A",     cmd_alloc_from },
	{ "free",  cmd_free },
	{ "f",     cmd_free },
	{ "",      NULL }
};


#define SKIP_SPACE(ch) do while (isspace(*(ch))) ++(ch); while (0)


static void handle_command(char *line)
{
	static char last_line[1024];

	const struct command *cmd;
	char *name;

	SKIP_SPACE(line);
	if (*line == '#')
		return;

	if (!*line)
		strcpy(line, last_line);
	else
		strcpy(last_line, line);

	name = line;
	while (*line && !isspace(*line))
		++line;

	if (*line) {
		*line = '\0';
		++line;
	}

	for (cmd = commands; *(cmd->name); ++cmd)
		if (!strcmp(name, cmd->name)) {
			cmd->handle(name, line);
			return;
		}

	fprintf(stderr, "%s: unknown command\n", name);
}



struct chunk {
	struct chunk *next, *prev;
	int fd;
	unsigned long size;
	unsigned long start;
};

static struct chunk root = {
	.next = &root,
	.prev = &root,
};

#define for_each(a) for (a = root.next; a != &root; a = a->next)

static struct chunk *chunk_create(const char *prefix);
static void chunk_destroy(struct chunk *chunk);
static void chunk_add(struct chunk *chunk);

static int memparse(char *ptr, char **retptr, unsigned long *ret);


static void cmd_list(char *name, char *line)
{
	struct chunk *chunk;

	(void)name; (void)line;

	for_each(chunk)
		printf("%3d: %p@%p\n", chunk->fd,
		       (void *)chunk->size, (void *)chunk->start);
}


static void __cma_alloc(char *name, char *line, int from);

static void cmd_alloc(char *name, char *line)
{
	__cma_alloc(name, line, 0);
}

static void cmd_alloc_from(char *name, char *line)
{
	__cma_alloc(name, line, 1);
}

static void __cma_alloc(char *name, char *line, int from)
{
	static const char *what[2] = { "dev/kind", "regions" };

	unsigned long size, alignment = 0;
	struct cma_alloc_request req;
	struct chunk *chunk;
	char *spec;
	size_t n;
	int ret;

	SKIP_SPACE(line);
	if (!*line) {
		fprintf(stderr, "%s: expecting %s\n", name, what[from]);
		return;
	}

	for (spec = line; *line && !isspace(*line); ++line)
		/* nothing */;

	if (!*line) {
		fprintf(stderr, "%s: expecting size after %s\n",
			name, what[from]);
		return;
	}

	*line++ = '\0';
	n = line - spec;
	if (n > sizeof req.spec) {
		fprintf(stderr, "%s: %s too long\n", name, what[from]);
		return;
	}

	if (memparse(line, &line, &size) < 0 || !size) {
		fprintf(stderr, "%s: invalid size\n", name);
		return;
	}

	if (*line == '/')
		if (memparse(line, &line, &alignment) < 0) {
			fprintf(stderr, "%s: invalid alignment\n", name);
			return;
		}

	SKIP_SPACE(line);
	if (*line) {
		fprintf(stderr, "%s: unknown argument(s) at the end: %s\n",
			name, line);
		return;
	}


	chunk = chunk_create(name);
	if (!chunk)
		return;

	fprintf(stderr, "%s: allocating %p/%p\n", name,
		(void *)size, (void *)alignment);

	req.magic     = CMA_MAGIC;
	req.type      = from ? CMA_REQ_FROM_REG : CMA_REQ_DEV_KIND;
	req.size      = size;
	req.alignment = alignment;
	req.start     = 0;

	memcpy(req.spec, spec, n);
	memset(req.spec + n, '\0', sizeof req.spec - n);

	ret = ioctl(chunk->fd, IOCTL_CMA_ALLOC, &req);
	if (ret < 0) {
		fprintf(stderr, "%s: cma_alloc: %s\n", name, strerror(errno));
		chunk_destroy(chunk);
	} else {
		chunk_add(chunk);
		chunk->size  = req.size;
		chunk->start = req.start;

		printf("%3d: %p@%p\n", chunk->fd,
		       (void *)chunk->size, (void *)chunk->start);
	}
}


static void cmd_free(char *name, char *line)
{
	struct chunk *chunk;

	SKIP_SPACE(line);

	if (*line) {
		unsigned long num;

		errno = 0;
		num = strtoul(line, &line, 10);

		if (errno || num > INT_MAX) {
			fprintf(stderr, "%s: invalid number\n", name);
			return;
		}

		SKIP_SPACE(line);
		if (*line) {
			fprintf(stderr, "%s: unknown arguments at the end: %s\n",
				name, line);
			return;
		}

		for_each(chunk)
			if (chunk->fd == (int)num)
				goto ok;
		fprintf(stderr, "%s: no chunk %3lu\n", name, num);
		return;

	} else {
		chunk = root.prev;
		if (chunk == &root) {
			fprintf(stderr, "%s: no chunks\n", name);
			return;
		}
	}

ok:
	fprintf(stderr, "%s: freeing %p@%p\n", name,
		(void *)chunk->size, (void *)chunk->start);
	chunk_destroy(chunk);
}


static struct chunk *chunk_create(const char *prefix)
{
	struct chunk *chunk;
	int fd;

	chunk = malloc(sizeof *chunk);
	if (!chunk) {
		fprintf(stderr, "%s: %s\n", prefix, strerror(errno));
		return NULL;
	}

	fd = open("/dev/cma", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "%s: /dev/cma: %s\n", prefix, strerror(errno));
		return NULL;
	}

	chunk->prev = chunk;
	chunk->next = chunk;
	chunk->fd   = fd;
	return chunk;
}

static void chunk_destroy(struct chunk *chunk)
{
	chunk->prev->next = chunk->next;
	chunk->next->prev = chunk->prev;
	close(chunk->fd);
}

static void chunk_add(struct chunk *chunk)
{
	chunk->next = &root;
	chunk->prev = root.prev;
	root.prev->next = chunk;
	root.prev = chunk;
}



static int memparse(char *ptr, char **retptr, unsigned long *ret)
{
	unsigned long val;

	SKIP_SPACE(ptr);

	errno = 0;
	val = strtoul(ptr, &ptr, 0);
	if (errno)
		return -1;

	switch (*ptr) {
	case 'G':
	case 'g':
		val <<= 10;
	case 'M':
	case 'm':
		val <<= 10;
	case 'K':
	case 'k':
		val <<= 10;
		++ptr;
	}

	if (retptr) {
		SKIP_SPACE(ptr);
		*retptr = ptr;
	}

	*ret = val;
	return 0;
}
