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

#include "webcontrol_plugin.h"
#include "webcontrol_buffer.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

//struct webcontrol_core_ctx;

#include "webcontrol.h"

void plugins_load_string(webcontrol_core_ctx* webcontrol, buffer* plugins)
{
	if(!plugins) return;

	char *pch = strtok (plugins->ptr, ",");
	while (pch != NULL)
	{
        DEBUG("PLUGIN NAME IS %s", pch);
	    
		plugin* plugin = plugin_load(pch, webcontrol);

		if(plugin != NULL)
		{
			webcontrol->plugins[webcontrol->num_plugins++] = plugin;
			
		}
		else
		{
			DEBUG("Failed to load plugin %s\n", pch);
		}

		pch = strtok (NULL, ",");
	}
}

plugin* plugin_load(const char* name, webcontrol_core_ctx* webcontrol)
{
	DEBUG("LOAD PLUGIN %s\n", name);

	void* handle;
	void (*init_func)(void*);
	char* error;

	buffer* filename = buffer_init_string(MODULE_DIR);
	buffer_append_string(filename, "/mod_");
	buffer_append_string(filename, name);
	buffer_append_string(filename, ".so");

	DEBUG("FILENAME is %s\n", filename->ptr);

	if(!(handle = dlopen(filename->ptr, RTLD_NOW|RTLD_GLOBAL)))
	{
        fprintf(stderr, "%s\n", dlerror());
		buffer_free(filename);
		return NULL;
	}

	buffer* symbolname = buffer_init_string("webcontrol_mod_");
	buffer_append_string(symbolname, name);
	buffer_append_string(symbolname, "_plugin_init");

	DEBUG("SYMBOLNAME is %s\n", symbolname->ptr);

	*(void **) (&init_func) = dlsym(handle, symbolname->ptr);

	//if ((error = dlerror()) != NULL)
	if(init_func == NULL)
	{
       // fprintf(stderr, "%s\n", error);
       // exit(EXIT_FAILURE);
		fprintf(stderr, "%s\n", dlerror());

		buffer_free(filename);
		buffer_free(symbolname);

		dlclose(handle);

		return NULL;
    }

	plugin* plgn = (plugin*)malloc(sizeof(plugin));
    plgn->webcontrol = webcontrol;
	plgn->options = NULL;
	plgn->data = NULL;
	plgn->name = NULL;
	plgn->lib = handle;

	plgn->plugin_free = NULL;
	plgn->thread_init = NULL;
	plgn->thread_free = NULL;
	plgn->config_file_parsed = NULL;
	plgn->pre_handle_request = NULL;
	plgn->handle_request = NULL;
	plgn->post_handle_request = NULL;

	init_func(plgn);

	buffer_free(filename);
	buffer_free(symbolname);

	return plgn;
}

void plugin_unload(const plugin* p)
{
	assert(p);

	if(p->name)
	{
		buffer_free(p->name);
	}

	#ifndef WEBCONTROL_STATIC
	dlclose(p->lib);
	#endif

	free(p);
}
