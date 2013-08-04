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

#include "webcontrol_memcached.h"
#include "webcontrol_base.h"
#include "webcontrol_plugin.h"
#include <libmemcached/memcached.h>
#include <string.h>

static pthread_key_t memcached_tls_key;

typedef struct
{
	memcached_server_list_st servers;
} plugin_config;

static int webcontrol_memcached_thread_init(thread* thread, plugin* plugin)
{
	TRACE("webcontrol_memcached_thread_init()");

	plugin_config* pc = (plugin_config*)plugin->data;

	memcached_st* memc;

	if((memc = memcached_create(NULL)) == NULL)
	{
		ERROR("memcached_create() failed");
		return -1;
	}

	memcached_return rc;

	if(pc->servers)
	{
		rc = memcached_server_push(memc, pc->servers);
	}
	else
	{
		INFO("No memcached servers configured, using default servers");
		rc = memcached_server_push(memc, memcached_servers_parse(WEBCONTROL_MEMCACHED_DEFAULT_SERVERS));
	}

	if(rc != MEMCACHED_SUCCESS)
	{
		ERROR("Memcached error while pushing servers: %s", memcached_strerror(memc, rc));
		return -1;
	}

	// Store memcached as thread-local
	pthread_setspecific(memcached_tls_key, memc);

	return 0;
}

#if 0
static void webcontrol_memcached_thread_free(thread_t* thread)
{
	TRACE("webcontrol_memcached_thread_free()");

	memcached_st* mc = pthread_getspecific(memcached_tls_key);
	memcached_free(mc);
}
#endif

static void memcached_tls_key_free(void* value)
{
	TRACE("memcached_tls_key_free()");

	if(value)
	{
		memcached_free((memcached_st*)value);
	}
    
    pthread_setspecific(memcached_tls_key, NULL);
}

#if 0
static inline buffer* webcontrol_memcached_generate_key(const request* request)
{
	TRACE("webcontrol_memcached_generate_key");

	buffer* buf = buffer_init_string("webcontrol_host_");
	buffer_append_string_len(buf, request->ip_addr_str, request->ip_addr_strlen);
	return buf;
}
#endif

static inline void webcontrol_memcached_generate_key(const request* request, char* key, size_t* key_len)
{
	size_t len = strlen(WEBCONTROL_MEMCACHED_DEFAULT_KEY_PREFIX);
	memcpy(key, WEBCONTROL_MEMCACHED_DEFAULT_KEY_PREFIX, len);

	memcpy(key+len, request->ip_addr.str, request->ip_addr.str_len);
	len += request->ip_addr.str_len;

    *key_len = len;
}

static handler_status webcontrol_memcached_handle_request(request* request, plugin* p)
{
	TRACE("webcontrol_memcached_handle_request()");

	memcached_st *memc = pthread_getspecific(memcached_tls_key);

	handler_status status;

	//buffer* key = webcontrol_memcached_generate_key(request);
	char key[MEMCACHED_MAX_KEY];
	bzero(key, MEMCACHED_MAX_KEY);
	size_t key_len;
	webcontrol_memcached_generate_key(request, key, &key_len);

	size_t value_length;
	uint32_t flags;
	memcached_return error;

	//DEBUG("Going to Lookup key '%s'", key->ptr);
	DEBUG("Going to Lookup key '%s'", key);

	//char* value = memcached_get(memc, key->ptr, key->len-1, &value_length, &flags, &error);
	char* value = memcached_get(memc, key, key_len, &value_length, &flags, &error);

	if(value != NULL)
	{
		DEBUG("memcached_get returned '%s'", value);

		status = str4_cmp(value, 'T', 'R', 'U', 'E') ? HANDLER_ALLOW : HANDLER_DENY;

		/* This ensures that the memcached save handler isn't called */
		request->skip_post_handle_request = 1;

		free(value);
	}
	else
	{
		DEBUG("memcached_get failed: %s", memcached_strerror(memc, error));
		status = HANDLER_GO_ON;
	}

	//buffer_free(key);

	return status;
}

/**
 * Stores the lookup result in the cache
 */
static handler_status webcontrol_memcached_post_handle_request(request* request)
{
	DEBUG("webcontrol_memcached_post_handle_request()");

	memcached_st* memc = pthread_getspecific(memcached_tls_key);

	//buffer* key = webcontrol_memcached_generate_key(request);
	char key[MEMCACHED_MAX_KEY];
	bzero(key, MEMCACHED_MAX_KEY);
	size_t key_len;
	
    DEBUG("OK!!");
	webcontrol_memcached_generate_key(request, key, &key_len);
    DEBUG("OK2!!");

	const char* value;
	size_t value_len;
	memcached_return_t error;

	if(request->response.result == HANDLER_ALLOW)
	{
		value = "TRUE";
		value_len = 4;
	}
	else
	{
		value = "FALSE";
		value_len = 5;
	}

	//DEBUG("Setting memcache key '%s' of len %zu to '%s'", key->ptr, key->len, value);
	DEBUG("Setting memcache key '%s' of len %zu to '%s'", key, key_len, value);

	//if((error = memcached_set(memc, key->ptr, key->len-1, value, value_len, (time_t)0, (uint32_t)0)) != MEMCACHED_SUCCESS)
	if((error = memcached_set(memc, key, key_len, value, value_len, (time_t)0, (uint32_t)0)) != MEMCACHED_SUCCESS)
	{
		DEBUG("Memcached Error: %s", memcached_strerror(memc, error));
	}

	//buffer_free(key);

	return HANDLER_OK;
}

static void webcontrol_memcached_processconfig(plugin* p)
{
	assert(p->webcontrol);
	assert(p->webcontrol->cfg);
	assert(p->data);

	plugin_config* pc = (plugin_config*)p->data;
	pc->servers = NULL;

	cfg_t* section = cfg_getsec(p->webcontrol->cfg, "memcached");
	if(section)
	{
		size_t numServers = cfg_size(section, "servers");

		for(size_t i=0;i<numServers;++i)
		{
			char* server = cfg_getnstr(section, "servers", i);
			DEBUG("Server String is: %s", server);

			char* host = NULL;
			char* port = NULL;

			host = strtok(server, ":");
			port = strtok(NULL, ":");

			in_port_t port_numeric = 0;

			if(port != NULL)
			{
				sscanf(port, "%hu", &port_numeric);
			}

			memcached_return_t error;
			pc->servers = memcached_server_list_append(pc->servers, host, port_numeric, &error);


			free(server);
		}

		DEBUG("Server List Count %d", memcached_server_list_count(pc->servers));
	}
}

static cfg_opt_t plugin_opts[] = {
	CFG_STR_LIST("servers", "{127.0.0.1}", CFGF_NONE),
	CFG_END()
};

static void webcontrol_memcached_free(plugin* p)
{
	DEBUG("FREEEE");

	assert(p->data);

	plugin_config* pc = (plugin_config*)p->data;

	memcached_server_list_free(pc->servers);
	pc->servers = NULL;

	free(pc);
	p->data = NULL;
}

void webcontrol_mod_memcached_plugin_init(plugin* p)
{
	plugin_config* pc = (plugin_config*)malloc(sizeof(plugin_config));
	p->data = pc;

	p->name = buffer_init_string("memcached");
	p->options = plugin_opts;
	p->config_file_parsed = webcontrol_memcached_processconfig;

	p->plugin_free = webcontrol_memcached_free;

	p->thread_init = webcontrol_memcached_thread_init;
	//p->thread_free = webcontrol_memcached_thread_free;

	p->handle_request = webcontrol_memcached_handle_request;
	p->post_handle_request = webcontrol_memcached_post_handle_request;

	//pthread_key_create(&memcached_tls_key, NULL);
	pthread_key_create(&memcached_tls_key, memcached_tls_key_free);
}

