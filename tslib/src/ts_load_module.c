/*
 *  tslib/src/ts_load_module.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_load_module.c,v 1.2 2002/07/01 23:02:57 dlowder Exp $
 *
 * Close a touchscreen device.
 */
#include "config.h"

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "tslib-private.h"

int ts_load_module(struct tsdev *ts, const char *module, const char *params)
{
	struct tslib_module_info * (*init)(struct tsdev *, const char *);
	struct tslib_module_info *info;
	char fn[1024];
	void *handle;
	int ret;
	char *plugin_directory=NULL;

	memset( fn, 0, 1024 );
	if( (plugin_directory = getenv("TSLIB_PLUGINDIR")) != NULL )
		strcpy(fn,plugin_directory);
	else 
		strcpy(fn, PLUGIN_DIR);

	strcat(fn, "/");
	strcat(fn, module);
	strcat(fn, ".so");
#ifdef DEBUG
	printf("ts_load_module(%s, %s)\n", module, params);
#endif	
	handle = dlopen(fn, RTLD_NOW);
	if (!handle) {
		char *error = dlerror();
		printf("dlopen %s error: %s\n", fn, error);
		return -1;
	}

	init = dlsym(handle, "mod_init");
	if (!init) {
		printf("mod_init error.\n");
		dlclose(handle);
		return -1;
	}

	info = init(ts, params);
	if (!info) {
		printf("init(%s) error.\n", params);
		dlclose(handle);
		return -1;
	}

	info->handle = handle;

	ret = __ts_attach(ts, info);
	if (ret) {
		info->ops->fini(info);
		dlclose(handle);
	}

	return ret;
}
