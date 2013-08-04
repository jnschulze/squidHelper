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

#include "webcontrol_txtfile.h"
#include "webcontrol_base.h"
#include "webcontrol_plugin.h"
#include "webcontrol_buffer.h"
#include <string.h>
#include <arpa/inet.h> // inet_pton

typedef struct host_entry
{
    ipaddr address;
    handler_status flag;
    struct host_entry* next;
} host_entry;

typedef struct
{
    ev_stat* file_stat;
    char* filename;
    host_entry* entry_chain;
    pthread_mutex_t mtx;
} plugin_data;

static void clear_entries(plugin_data* pd)
{
    while(pd->entry_chain)
    {
        host_entry* oldnext = pd->entry_chain->next;
        free(pd->entry_chain);
        pd->entry_chain = oldnext;
    }
}

static void print_entries(const plugin_data* pd)
{
    host_entry* current = pd->entry_chain;
    
    while(current)
    {
        DEBUG("IP Address: %s, Flag: %d", current->address.str, current->flag);
        current = current->next;
    }
}

static host_entry* parse_line(char* line)
{
    char* pch;
    char* tmp;
    host_entry* entry = malloc(sizeof(host_entry));
    entry->next = NULL;
    
    pch = strtok_r(line, " ", &tmp);
    
    //DEBUG("Got IP Address String '%s'", pch);
    
    #ifdef HAVE_IPV6
    if(inet_pton(AF_INET6, pch, &((struct sockaddr_in6*)&entry->address.sockaddr)->sin6_addr))
    {
        //DEBUG("Got valid IPv6 address");
        entry->address.sockaddr.ss_family = AF_INET6;
        goto resolved_addr;
    }
    #endif
    
    if(!inet_pton(AF_INET, pch, &((struct sockaddr_in*)&entry->address.sockaddr)->sin_addr))
    {
        //DEBUG("Invalid IP address");
        free(entry);
        return NULL;
    }
    
    entry->address.sockaddr.ss_family = AF_INET;
    
    resolved_addr:
    
    pch = strtok_r(NULL, "\r\n", &tmp);
    strtoupper(pch);
    
    //entry->flag = str4_cmp(pch, 'D', 'E', 'N', 'Y') ? HANDLER_DENY : HANDLER_ALLOW;
    entry->flag = strcmp(pch, "ALLOW") == 0 ? HANDLER_ALLOW : HANDLER_DENY;
    
    return entry;
}

static void parse_file(plugin_data* pd)
{
    //TRACE("parse_file()");
    
    FILE* handle;
    
    char line[LINE_MAX];
    
    if((handle = fopen(pd->filename, "r")) == NULL)
    {
        printf("ERROR\n");
        return;
    }
    
    pthread_mutex_lock(&pd->mtx);
    clear_entries(pd);
    
    while (fgets(line, LINE_MAX, handle) != NULL)
    {
        host_entry* current_entry;
        if((current_entry = parse_line(line)) != NULL)
        {
            current_entry->next = pd->entry_chain;
            pd->entry_chain = current_entry;
        }
    }
    
    pthread_mutex_unlock(&pd->mtx);
    
    fclose(handle);
    
    //print_entries(pd);
}

static void file_change_cb (struct ev_loop *loop, ev_stat *w, int revents)
{
    DEBUG("File changed.");
    
    /*
    if (w->attr.st_nlink)
    {
        DEBUG("current size  %ld\n", (long)w->attr.st_size);
        DEBUG("current atime %ld\n", (long)w->attr.st_mtime);
        DEBUG("current mtime %ld\n", (long)w->attr.st_mtime);
    }
    */
    
    parse_file((plugin_data*)w->data);
}

inline int compare_addresses(const ipaddr* a, const ipaddr* b)
{
    if(a->sockaddr.ss_family != b->sockaddr.ss_family) return -1;
    
    switch(a->sockaddr.ss_family)
    {
        case AF_INET:      
            return ((struct sockaddr_in*)&a->sockaddr)->sin_addr.s_addr == ((struct sockaddr_in*)&b->sockaddr)->sin_addr.s_addr;
        case AF_INET6:
            return memcmp(((struct sockaddr_in6*)&a->sockaddr)->sin6_addr.s6_addr, ((struct sockaddr_in6*)&b->sockaddr)->sin6_addr.s6_addr, 16) == 0;
        default:
            return -1;
    }
}

static handler_status webcontrol_txtfile_handle_request(request* request, plugin* p)
{
    //DEBUG("HANDLE REQUEST!");
    
    plugin_data* pd = (plugin_data*)p->data;
    
    handler_status status = HANDLER_GO_ON;
    
    pthread_mutex_lock(&pd->mtx);
    
    host_entry* entry = pd->entry_chain;
    
    while(entry)
    {   
        if(compare_addresses(&request->ip_addr, &entry->address) == 1)
        {
            //DEBUG("Found Record %s with flag %d", request->ip_addr.str, entry->flag);
            
            status = entry->flag;
            break;
        }
        
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&pd->mtx);
    
    return status;
}

void webcontrol_txtfile_process_config(plugin* p)
{
    parse_file((plugin_data*)p->data);
}

static void webcontrol_txtfile_free(plugin* p)
{
    plugin_data* pd = (plugin_data*)p->data;
    
    ev_stat_stop(p->webcontrol->loop, pd->file_stat);
    free(pd->file_stat);
    
    pthread_mutex_lock(&pd->mtx);
    clear_entries(pd);
    pthread_mutex_unlock(&pd->mtx);
    
    pthread_mutex_destroy(&pd->mtx);
}

void webcontrol_mod_txtfile_plugin_init(plugin* p)
{
    p->name = buffer_init_string("txtfile");
    p->plugin_free = webcontrol_txtfile_free;
    p->handle_request = webcontrol_txtfile_handle_request;
    p->config_file_parsed = webcontrol_txtfile_process_config;
    
	plugin_data* pd = (plugin_data*)malloc(sizeof(plugin_data));
	p->data = pd;
	
	cfg_opt_t plugin_opts[] =
	{
		CFG_SIMPLE_STR("file", &pd->filename),
		CFG_END()
	};

    pd->filename = strdup(WEBCONTROL_TXTFILE_DEFAULT_FILENAME);

	p->options = malloc(sizeof(plugin_opts));
	memcpy(p->options, plugin_opts, sizeof(plugin_opts));
	
    pd->entry_chain = NULL;
    
    pthread_mutex_init(&pd->mtx, NULL);
	
    pd->file_stat = malloc(sizeof(ev_stat));
    pd->file_stat->data = pd;
	
	ev_stat_init(pd->file_stat, file_change_cb, pd->filename, 0.);
    ev_stat_start(p->webcontrol->loop, pd->file_stat);
}

