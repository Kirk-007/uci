/*
 * libuci - Library for the Unified Configuration Interface
 * Copyright (C) 2008 Felix Fietkau <nbd@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * This file contains some common code for the uci library
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "uci.h"
#include "err.h"

static const char *uci_confdir = UCI_CONFDIR;
static const char *uci_savedir = UCI_SAVEDIR;

static const char *uci_errstr[] = {
	[UCI_OK] =            "Success",
	[UCI_ERR_MEM] =       "Out of memory",
	[UCI_ERR_INVAL] =     "Invalid argument",
	[UCI_ERR_NOTFOUND] =  "Entry not found",
	[UCI_ERR_IO] =        "I/O error",
	[UCI_ERR_PARSE] =     "Parse error",
	[UCI_ERR_DUPLICATE] = "Duplicate entry",
	[UCI_ERR_UNKNOWN] =   "Unknown error",
};

#include "util.c"
#include "list.c"
#include "file.c"

/* exported functions */
struct uci_context *uci_alloc_context(void)
{
	struct uci_context *ctx;

	ctx = (struct uci_context *) malloc(sizeof(struct uci_context));
	memset(ctx, 0, sizeof(struct uci_context));
	uci_list_init(&ctx->root);
	uci_list_init(&ctx->history_path);
	ctx->flags = UCI_FLAG_STRICT;

	ctx->confdir = (char *) uci_confdir;
	ctx->savedir = (char *) uci_savedir;

	return ctx;
}

void uci_free_context(struct uci_context *ctx)
{
	struct uci_element *e, *tmp;

	if (ctx->confdir != uci_confdir)
		free(ctx->confdir);
	if (ctx->savedir != uci_savedir)
		free(ctx->savedir);

	UCI_TRAP_SAVE(ctx, ignore);
	uci_cleanup(ctx);
	uci_foreach_element_safe(&ctx->root, tmp, e) {
		struct uci_package *p = uci_to_package(e);
		uci_free_package(&p);
	}
	uci_foreach_element_safe(&ctx->history_path, tmp, e) {
		uci_free_element(e);
	}
	free(ctx);
	UCI_TRAP_RESTORE(ctx);

ignore:
	return;
}

int uci_add_history_path(struct uci_context *ctx, const char *dir)
{
	struct uci_element *e;

	UCI_HANDLE_ERR(ctx);
	UCI_ASSERT(ctx, dir != NULL);
	e = uci_alloc_generic(ctx, UCI_TYPE_PATH, dir, sizeof(struct uci_element));
	uci_list_add(&ctx->history_path, &e->list);

	return 0;
}

int uci_set_confdir(struct uci_context *ctx, const char *dir)
{
	char *cdir;

	UCI_HANDLE_ERR(ctx);
	UCI_ASSERT(ctx, dir != NULL);

	cdir = uci_strdup(ctx, dir);
	if (ctx->confdir != uci_confdir)
		free(ctx->confdir);
	ctx->confdir = cdir;
	return 0;
}

int uci_set_savedir(struct uci_context *ctx, const char *dir)
{
	char *sdir;

	UCI_HANDLE_ERR(ctx);
	UCI_ASSERT(ctx, dir != NULL);

	sdir = uci_strdup(ctx, dir);
	if (ctx->savedir != uci_savedir)
		free(ctx->savedir);
	ctx->savedir = sdir;
	return 0;
}

int uci_cleanup(struct uci_context *ctx)
{
	UCI_HANDLE_ERR(ctx);
	uci_file_cleanup(ctx);
	return 0;
}

void uci_perror(struct uci_context *ctx, const char *prefix)
{
	int err;

	if (!ctx)
		err = UCI_ERR_INVAL;
	else
		err = ctx->errno;

	if ((err < 0) || (err >= UCI_ERR_LAST))
		err = UCI_ERR_UNKNOWN;

	if (prefix)
		fprintf(stderr, "%s: ", prefix);
	if (ctx->func)
		fprintf(stderr, "%s: ", ctx->func);

	switch (err) {
	case UCI_ERR_PARSE:
		if (ctx->pctx) {
			fprintf(stderr, "%s (%s) at line %d, byte %d\n", uci_errstr[err], (ctx->pctx->reason ? ctx->pctx->reason : "unknown"), ctx->pctx->line, ctx->pctx->byte);
			break;
		}
		/* fall through */
	default:
		fprintf(stderr, "%s\n", uci_errstr[err]);
		break;
	}
}


