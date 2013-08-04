// Copyright (C) 2009-2012 Jan Niklas Schulze
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef _WEBCONTROL_PLUGIN_H
#define _WEBCONTROL_PLUGIN_H

#include "webcontrol_buffer.h"
#include "webcontrol_request.h"
#include "webcontrol_base.h"

#include "webcontrol.h"

#ifdef HAVE_LIBCONFUSE
 #include <confuse.h>
#endif

//struct webcontrol_core_ctx;

//typedef struct plugin plugin;

typedef struct plugin
{
	buffer* name;

	/* webcontrol core */
	struct webcontrol_core_ctx* webcontrol;

	/* cleanup function */
	void (*plugin_free)(struct plugin* p);

	/* thread initialization */
	int (*thread_init)(thread* t, struct plugin* p);

	/* thread cleanup */
	void (*thread_free)(thread* t, struct plugin* p);

	/* config cb */
	void (*config_file_parsed)(struct plugin* p);

	/* pre dispatch handler */
	handler_status (*pre_handle_request)(request* r);

	/* The actual request handler */	
	handler_status (*handle_request)(request* r, struct plugin* p);
	
	/* post dispatch handler */
	handler_status (*post_handle_request)(request* r, struct plugin* p);

	/* plugin custom data */
	void* data;

	/* dlopen handle */
	void* lib;

	/* plugin options */
	#ifdef HAVE_LIBCONFUSE
	cfg_opt_t* options;
	#endif

} plugin;

void plugins_load_string(struct webcontrol_core_ctx* webcontrol, buffer* plugin_list);
plugin* plugin_load(const char* plugin_name, struct webcontrol_core_ctx* webcontrol);
void plugin_unload(const plugin* plugin);

#endif