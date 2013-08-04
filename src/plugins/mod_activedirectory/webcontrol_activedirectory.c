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

#include "webcontrol_activedirectory.h"
#include "webcontrol_base.h"
#include "webcontrol_plugin.h"
#include "webcontrol_buffer.h"
#include "webcontrol_netutil.h"

#include <stdio.h>
#include <ldap.h>
#include <stdlib.h>
#include <sys/time.h>

typedef struct
{
	char* uri;
	char* bind_dn;
	char* bind_password;
	char* basedn;
	char* filter;
	size_t filter_len;
	char* attribute_name;
} plugin_config;

pthread_key_t ldap_key;

static void ldap_key_destruct(void *value)
{
	TRACE("ldap_key_destruct");

	ldap_unbind_ext((LDAP*)value, NULL, NULL);
    pthread_setspecific(ldap_key, NULL);
}

static int webcontrol_activedirectory_ldap_open(LDAP* ldap_con, const plugin_config* pc)
{
	int msgid;
	int status;

	if(pc->uri == NULL)
	{
		ERROR("No LDAP URI specified.");
		return -1;
	}

	if((status = ldap_initialize(&ldap_con, pc->uri)) != LDAP_SUCCESS)
	{
		ERROR("ldap_initialize() failed: %s", ldap_err2string(status));
		return -1;
	}

	//struct berval* servercredp;

	if(pc->bind_password != NULL)
	{
		struct berval passwd;
		passwd.bv_val = pc->bind_password;
		passwd.bv_len = strlen(pc->bind_password);

		//status = ldap_sasl_bind_s(ldap_con, pc->bind_dn, LDAP_SASL_SIMPLE, &passwd, NULL, NULL, &servercredp);
		status = ldap_sasl_bind(ldap_con, pc->bind_dn, LDAP_SASL_SIMPLE, &passwd, NULL, NULL, &msgid);
	}
	else
	{
		//status = ldap_sasl_bind_s(ldap_con, pc->bind_dn, LDAP_SASL_SIMPLE, NULL, NULL, NULL, &servercredp);
		status = ldap_sasl_bind(ldap_con, pc->bind_dn, LDAP_SASL_SIMPLE, NULL, NULL, NULL, &msgid);
	}

	if(status != LDAP_SUCCESS)
	{
		ERROR("ldap_sasl_bind() failed: %s", ldap_err2string(status));
		return -1;
	}

	struct timeval tv;
	tv.tv_usec = 0;
	tv.tv_sec = 10;

	LDAPMessage* result;

	if((status = ldap_result(ldap_con, msgid, 0, &tv, &result)) != LDAP_RES_BIND)
	{
		ERROR("ldap_result() failed");
		return -1;
	}

	ldap_msgfree(result);

	/* Store the connection as thread local */
	pthread_setspecific(ldap_key, ldap_con);

	return 0;
}

static int webcontrol_activedirectory_thread_init(thread* thread, plugin* p)
{
	DEBUG("Webcontrol AD Plugin Thread Init");

	plugin_config* pc = (plugin_config*)p->data;

	LDAP* ldap_con = NULL;
	if(webcontrol_activedirectory_ldap_open(ldap_con, pc) != 0)
	{
		DEBUG("Error establishing LDAP connection");
		return -1;
	}


	return 0;
}

static void webcontrol_activedirectory_thread_free(thread* thread, plugin* p)
{
	DEBUG("Webcontrol AD Plugin Thread Cleanup");

	//LDAP* ldap_con = pthread_getspecific(ldap_key);

	//ldap_unbind_ext( ldap_con, NULL, NULL );
}


static handler_status webcontrol_activedirectory_handle_request(request* request, plugin* plugin)
{
	DEBUG("webcontrol_activedirectory_handle_request()");

	plugin_config* config = (plugin_config*)plugin->data;

	char hostname[HOST_NAME_MAX+1];
	bzero(hostname,HOST_NAME_MAX);

	if(webcontrol_rdns_lookup(&request->ip_addr, hostname, HOST_NAME_MAX) != 0)
	{
		DEBUG("Unable to lookup hostname");
		return HANDLER_GO_ON;
	}

	DEBUG("Looked up hostname: '%s'", hostname);

	size_t hostname_len = strlen(hostname);
	size_t filter_len = hostname_len + config->filter_len - 2; // -2 for the replaced %s

	buffer* assembled_filter = buffer_init();
    buffer_prepare_copy(assembled_filter, filter_len);

	sprintf(assembled_filter->ptr, config->filter, hostname);

	assembled_filter->len = filter_len;

	//assert(assembled_filter->len == strlen(assembled_filter->ptr));

	DEBUG("FILTER TEMPLATE LEN is %zu", config->filter_len);
	DEBUG("HOSTNAME LEN is %zu", hostname_len);
	DEBUG("ASSEMBLED FILTER IS %s", assembled_filter->ptr);

	char* attribs[2] = {config->attribute_name ? config->attribute_name : WEBCONTROL_ACTIVEDIRECTORY_ATTRIBUTENAME, NULL};

	LDAP* ldap_con = pthread_getspecific(ldap_key);

	LDAPMessage* result;
	int msgid;
	LDAPMessage* entry;
	BerElement *ber;
	char* attr = NULL;
	struct berval** values;
	//int timeout = 0;

	//int rc = ldap_search(ldap_con, config->basedn, LDAP_SCOPE_SUBTREE, assembled_filter->ptr, attribs, 0, &msgid);
	int rc = ldap_search_ext(ldap_con, config->basedn, LDAP_SCOPE_SUBTREE, assembled_filter->ptr, attribs, 0, NULL, NULL, NULL, LDAP_NO_LIMIT, &msgid);

	if (rc != LDAP_SUCCESS)
	{
		DEBUG("LDAP search error: %s", ldap_err2string(rc));

		buffer_free(assembled_filter);

		return HANDLER_GO_ON;
	}

	rc = ldap_result(ldap_con, msgid, LDAP_MSG_ONE, NULL, &result);

	if(rc != LDAP_RES_SEARCH_ENTRY)
	{
		DEBUG("ldap_result() failed: %s %d", ldap_err2string(rc), rc);

		ldap_msgfree(result);
		buffer_free(assembled_filter);

		return HANDLER_GO_ON;
	}

	//DEBUG("LDAP return code is %d %d %d", rc, LDAP_RES_SEARCH_RESULT, LDAP_RES_SEARCH_ENTRY);

	if((entry = ldap_first_entry(ldap_con, result)) == NULL)
	{
		DEBUG("ldap_first_entry() failed");

		ldap_msgfree(result);
		buffer_free(assembled_filter);

		return HANDLER_GO_ON;
	}

	handler_status status = HANDLER_GO_ON;

	if((attr = ldap_first_attribute(ldap_con, entry, &ber)) != NULL)
	{
		//DEBUG("Got attribute");

		if((values = ldap_get_values_len(ldap_con, entry, attr)) != NULL)
		{
			DEBUG("Got value");

			status = str4_cmp(values[0]->bv_val, 'T', 'R', 'U', 'E') ? HANDLER_ALLOW : HANDLER_DENY;
			ldap_value_free_len(values);
		}

		ber_free(ber,0);
		ldap_memfree(attr);
	}

	buffer_free(assembled_filter);

	DEBUG("STATUS is %d", status);

	return status;
}

static void webcontrol_activedirectory_plugin_free(plugin* p)
{
	DEBUG("webcontrol_activedirectory_plugin_free()");

	free(p->data);
}

void webcontrol_activedirectory_processconfig(plugin* p)
{
	assert(p->webcontrol);
	assert(p->webcontrol->cfg);
	assert(p->data);

	plugin_config* pd = (plugin_config*)p->data;

	if(pd->filter != NULL)
	{
		pd->filter_len = strlen(pd->filter);
	}
}

static void config_values_init(plugin_config* data)
{
	data->uri = NULL;
	data->bind_dn = NULL;
	data->bind_password = NULL;
	data->basedn = NULL;
	data->filter = NULL;
	data->filter_len = 0;
	data->attribute_name = NULL;
}

void webcontrol_mod_activedirectory_plugin_init(plugin* p)
{
	//p->init = webcontrol_activedirectory_init;

	p->name = buffer_init_string("ActiveDirectory");

	plugin_config* data = (plugin_config*)malloc(sizeof(plugin_config));
	p->data = data;

	config_values_init(data);

	//data->filter = buffer_init();

	cfg_opt_t plugin_opts[] =
	{
		CFG_SIMPLE_STR("uri", &data->uri),
		CFG_SIMPLE_STR("bind_dn", &data->bind_dn),
		CFG_SIMPLE_STR("bind_password", &data->bind_password),
		CFG_SIMPLE_STR("basedn", &data->basedn),
		CFG_SIMPLE_STR("filter", &data->filter),
		CFG_SIMPLE_STR("attribute_name", &data->attribute_name),
		CFG_END()
	};

	//DEBUG("SIZEOF is %d", sizeof(plugin_opts));

	p->options = malloc(sizeof(plugin_opts));
	memcpy(p->options, plugin_opts, sizeof(plugin_opts));

	p->config_file_parsed = webcontrol_activedirectory_processconfig;

	p->plugin_free = webcontrol_activedirectory_plugin_free;

	p->thread_init = webcontrol_activedirectory_thread_init;
	p->thread_free = webcontrol_activedirectory_thread_free;


	
	p->handle_request = webcontrol_activedirectory_handle_request;

	pthread_key_create(&ldap_key, &ldap_key_destruct);
}

