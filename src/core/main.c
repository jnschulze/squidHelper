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

#include "webcontrol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <netinet/in.h>
#include <arpa/inet.h> // inet_pton
#include <pthread.h>
#include <sys/fcntl.h>
#include <unistd.h>

pthread_key_t current_thread_key;

#define STDIN_BUF_CHUNKSIZE 4096
#define DEFAULT_CONFIG_FILE_NAME "webcontrol.conf"

int setnonblock(int fd)
{
    int flags;
    flags = fcntl(fd, F_GETFL);
    flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags);
}

static void stdin_read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    webcontrol_core_ctx *webcontrol = ev_userdata(ev_default_loop(0));

    assert(webcontrol->tmp_line_buf != NULL);
    //assert(webcontrol->tmp_line_buf->capacity > 0);

    buffer* buf = webcontrol->tmp_line_buf;

    char readbuf[STDIN_BUF_CHUNKSIZE];

    while(true)
    {
        ssize_t bytes = read(STDIN_FILENO, readbuf, sizeof(readbuf)-1);

        if(bytes > 0)
        {
            assert(bytes <= sizeof(readbuf)-1);

            readbuf[bytes] = '\0';
            //printf("Read %s\n", readbuf);

            size_t line_start = 0;

            //size_t num_lines = 0;

            for(size_t i=0;i<bytes;++i)
            {
                char c = readbuf[i];
                if(c == '\n')
                {
                    readbuf[i] = '\0';

                    buffer* line;

                    if(webcontrol->tmp_line_buf->len > 0)
                    {
                        //printf("Have remainder '%s'\n", webcontrol->tmp_line_buf->ptr);
                        line = buffer_init_buffer(webcontrol->tmp_line_buf);
                        buffer_append_string(line, readbuf+line_start);
                        buffer_reset(webcontrol->tmp_line_buf);
                    }
                    else
                    {
                        line = buffer_init_string(readbuf+line_start);
                    }

                    //printf("Found line '%s'\n", readbuf+line_start);
                    

                    //printf("HAVE LINE '%s'\n", line->ptr);

                    queue_enqueue(webcontrol->job_queue, line); 

                    //buffer_free(line);

                    line_start = ++i;

                    //num_lines++;
                }
            }

            if(readbuf[line_start] != '\0')
            {
                //fprintf(stdout, "Remainder is %s\n", readbuf+line_start);
                buffer_append_string_len(webcontrol->tmp_line_buf, readbuf+line_start, bytes-line_start);
            }
            else
            {
                //printf("No remainder\n");
                buffer_reset(webcontrol->tmp_line_buf);
                return;
            }
        }
        else if(bytes == 0)
        {
            printf("GOT 0 BYTES. TERMINATE?\n");
            ev_io_stop(loop, watcher);
            return;
        }
        else /* bytes < 0 */
        {
            fprintf(stderr, "GOT ERRNO\n");
            switch(errno)
            {
                case EAGAIN:
                    fprintf(stderr, "Got EAGAIN\n");
                    //ev_io_stop(loop, watcher);
                    return;
                default:
                    fprintf(stderr, "Unknown Errno: %d\n", errno);
                    return;
            }
        }
    }

    //printf("Got %d lines\n", lines);

}

#define CALL_HOOK(NAME) {\
    plugin** plugins = worker->webcontrol->plugins;\
    for(size_t i = 0; i<worker->webcontrol->num_plugins; ++i)\
    {\
        plugin* currentPlugin = plugins[i];\
        if(currentPlugin->NAME != NULL)\
        {\
            currentPlugin->NAME(worker, currentPlugin);\
        }\
    }\
}

#define CALL_HOOK_SIMPLE(NAME) {\
    plugin** plugins = webcontrol->plugins;\
    size_t i;\
    for(i = 0; i<webcontrol->num_plugins; ++i)\
    {\
        plugin* currentPlugin = plugins[i];\
        if(currentPlugin->NAME != NULL)\
        {\
            currentPlugin->NAME(currentPlugin);\
        }\
    }\
}

/*
static inline void send_response(size_t channel, int status, const char* msg)
{
    const char* status_str = (status == 0) ? "OK" : "ERR";

    if(channel == 0)
    {
        if(msg == NULL)
        {
            fputs(status_str, stdout);
            return;
        }

        fprintf(stdout, "%s log=%s\n", status_str, msg);
    }
    else
    {
        if(msg == NULL)
        {
            fprintf(stdout, "%zu %s\n", channel, status_str);
            return;
        }
        fprintf(stdout, "%zu %s log=%s\n", channel, status_str, msg);
    }
}
*/

static void send_response(size_t channel, int status, const char* msg)
{
    const char* status_str = (status == 0) ? "OK" : "ERR";

    if(msg == NULL)
        fprintf(stdout, "%zu %s\n", channel, status_str);
    else
        fprintf(stdout, "%zu %s log=%s\n", channel, status_str, msg);

    fflush(stdout);
}

static void* worker(void *arg)
{
    thread* this_worker = (thread*)arg;
    thread* worker = this_worker;//legacy

    pthread_setspecific(current_thread_key, this_worker);

    //CALL_HOOK(thread_init);
    plugin** plugins = worker->webcontrol->plugins;
    size_t i;
    for(i = 0; i<worker->webcontrol->num_plugins; ++i)
    {
        plugin* currentPlugin = plugins[i];
        if(currentPlugin->thread_init != NULL)
        {
            if(currentPlugin->thread_init(worker, currentPlugin) != 0)
            {
                ERROR("Calling the thread init hook failed. Terminating thread.");
                goto terminate;
            }
        }
    }

    while(1)
    {
        buffer* line = (buffer*)queue_dequeue(worker->webcontrol->job_queue);

        if(line == NULL) /* If we get a null pointer, terminate this thread */
        {
            DEBUG("Empty. Will Terminate Thread");
            break;
        }

        //DEBUG("GOT LINE %s", line->ptr);

        request* req = malloc(sizeof(request));
        req->channel = 0; // no channel
        req->response.result = -1;
        req->skip_post_handle_request = 0;
        //request->ipAddress = malloc(sizeof(WEBCONTROL_IPADDR));

        bzero(req->ip_addr.str, WEBCONTROL_IPADDRSTRLEN);
        memset(&req->ip_addr.sockaddr, 0, sizeof(struct sockaddr_storage));

        #if 0
        #define LINE_FORMAT "%u %" STR(WEBCONTROL_IPADDRSTRLEN) "s"
        printf("LINE FORMAT %s\n", LINE_FORMAT);
        sscanf(line->ptr, LINE_FORMAT, &request->channel, &request->ipAddressStr);
        #endif

        #ifndef HAVE_IPV6
        sscanf(line->ptr, "%zu %15s", &req->channel, req->ip_addr.str);
        #else
        sscanf(line->ptr, "%zu %45s", &req->channel, req->ip_addr.str);
        if(inet_pton(AF_INET6, req->ip_addr.str, &((struct sockaddr_in6*)&req->ip_addr.sockaddr)->sin6_addr))
        {
            DEBUG("Got valid IPv6 address");
            req->ip_addr.sockaddr.ss_family = AF_INET6;
            goto resolved_addr;
        }

        DEBUG("%s is no valid IPv6 address. Let's check whether it's IPv4", req->ip_addr.str);
        #endif

        if(!inet_pton(AF_INET, req->ip_addr.str, &((struct sockaddr_in*)&req->ip_addr.sockaddr)->sin_addr))
        {
            pthread_mutex_lock(&worker->webcontrol->stdout_lock);
            send_response(req->channel, -1, "Valid IP address expected");
            pthread_mutex_unlock(&worker->webcontrol->stdout_lock);
            goto endrequest;
        }

        req->ip_addr.sockaddr.ss_family = AF_INET;

        resolved_addr:

        req->ip_addr.str_len = strlen(req->ip_addr.str);

        plugin** plugins = worker->webcontrol->plugins;
        for(size_t i = 0; i<worker->webcontrol->num_plugins; ++i)
        {
            //printf("CALLING REQUEST HANDLER %d\n", i+1);
            plugin* currentPlugin = worker->webcontrol->plugins[i];
            if(currentPlugin->handle_request != NULL)
            {
                handler_status status = currentPlugin->handle_request(req, currentPlugin);

                DEBUG("Status is %d", status);

                req->response.result = status;

                if(status != HANDLER_GO_ON)
                {
                    DEBUG("Handler got result");
                    break;
                }
            }
        }

        switch(req->response.result)
        {
            case HANDLER_ALLOW:
                DEBUG("SEND RESPONSE OK %zu", req->channel);
                send_response(req->channel, 0, NULL);
                break;
            case HANDLER_DENY:
                DEBUG("SEND RESPONSE ERR %zu", req->channel);
                send_response(req->channel, -1, "DENIED");
                break;
            default:
                send_response(req->channel, worker->webcontrol->deny_by_default ? -1 : 0, "UNKNOWN ERR");
                goto endrequest;
        }

        if(!req->skip_post_handle_request)
        {
            plugin** plugins = worker->webcontrol->plugins;
            for(size_t i = 0; i<worker->webcontrol->num_plugins; ++i)
            {
                plugin* currentPlugin = plugins[i];
                if(currentPlugin->post_handle_request != NULL)
                {
                    currentPlugin->post_handle_request(req, currentPlugin);
                }
            }
        }

        endrequest:
        DEBUG("ENDREQUEST channel=%zu", req->channel);

        free(req);
        buffer_free(line);
    }

    terminate:

    DEBUG("WORKER THREAD TERMINATED\n");

    thread_msg* msg = malloc(sizeof(thread_msg));
    msg->signal = 0;
    msg->data = NULL;

    msg->thread = this_worker;
    msg->signal = WORKER_TERMINATE;

    queue_enqueue(worker->webcontrol->thread_signal_queue, msg);
    //ev_async_send(EV_DEFAULT_UC_ &worker->webcontrol->thread_sig);
    ev_async_send(EV_DEFAULT_UC_ worker->webcontrol->ev_thread_signal_watcher);

    CALL_HOOK(thread_free);

    // exit
    pthread_exit(0);
}

static void sigint_cb (struct ev_loop *loop, struct ev_signal *w, int revents)
{
    switch(w->signum)
    {
        case SIGINT:
            DEBUG("Received SIGINT, shutting down");
            break;
        case SIGTERM:
            DEBUG("Received SIGTERM, shutting down");
            break;
        default:
            DEBUG("Received Unknown Signal: %d", w->signum);
    }

    ev_unloop (loop, EVUNLOOP_ALL);

    DEBUG("EXIT\n");
}

static void thread_sig_cb(EV_P_ ev_async *w, int revents)
{
    webcontrol_core_ctx *webcontrol = ev_userdata(ev_default_loop(0));

    thread_msg* msg = (thread_msg*)queue_dequeue(webcontrol->thread_signal_queue);

    assert(msg);
    assert(msg->thread);

    DEBUG("Main thread received signal by thread #%zu", msg->thread->number);

    switch(msg->signal)
    {
        case WORKER_TERMINATE:
            DEBUG("worker thread #%zu terminated", msg->thread->number);

            if(--webcontrol->active_workers == 0)
            {
                DEBUG("All workers died. Shutting down.");
                ev_unloop (webcontrol->loop, EVUNLOOP_ALL);
            }

        default:
            break;
    }

    free(msg);
}


//int init_config(const buffer* config_file, webcontrol_core_ctx* webcontrol)
int init_config(const char* config_file, webcontrol_core_ctx* webcontrol)
{
    #define CONFIG_SLOTS_BLOCKSIZE 32 // must be big enough to store core options

    size_t num_opts = 0;

    // num slots needs to be big enough to hold core server args as well as terminator
    size_t num_slots = CONFIG_SLOTS_BLOCKSIZE;

    char* logfile = NULL;
    char* default_behavior = NULL;

    /*
    cfg_opt_t opts[] = {
        CFG_SIMPLE_BOOL("verbose", &verbose),
        CFG_SIMPLE_STR("server", &server),
        CFG_SIMPLE_STR("user", &username),
        CFG_SIMPLE_INT("debug", &debug),
        CFG_SIMPLE_FLOAT("delay", &delay),
        CFG_END()
    };
    */

    //cfg_opt_t* opts = calloc(sizeof(cfg_opt_t), num_slots);
    webcontrol->opts = calloc(sizeof(cfg_opt_t), num_slots);

    webcontrol->opts[num_opts++] = (cfg_opt_t)CFG_SIMPLE_STR("logfile", &logfile);
    webcontrol->opts[num_opts++] = (cfg_opt_t)CFG_SIMPLE_INT("loglevel", &webcontrol->loglevel);
    webcontrol->opts[num_opts++] = (cfg_opt_t)CFG_SIMPLE_STR("default_behavior", &default_behavior);


    size_t i;
    for(i=0;i<webcontrol->num_plugins;++i)
    {
        plugin* plugin = webcontrol->plugins[i];

        if(plugin->options != NULL && plugin->name != NULL)
        {
            DEBUG("Slots %zu / %zu", num_opts+1, num_slots);

            if(num_slots == num_opts+1) /* +1 because the final CFG_END needs space as well */
            {
                num_slots += CONFIG_SLOTS_BLOCKSIZE;
                webcontrol->opts = realloc(webcontrol->opts, sizeof(cfg_opt_t) * num_slots);
            }

            DEBUG("Adding Config Namespace %s to slot %zu of %zu", plugin->name->ptr, num_opts+1, num_slots);
            webcontrol->opts[num_opts++] = (cfg_opt_t)CFG_SEC(plugin->name->ptr, plugin->options, CFGF_NONE);
        }
    }

    webcontrol->opts[num_opts++] = (cfg_opt_t)CFG_END();

    webcontrol->cfg = cfg_init(webcontrol->opts, 0);

    //int result = cfg_parse(webcontrol->cfg, webcontrol->config_file->ptr);
    int result = cfg_parse(webcontrol->cfg, config_file);

    switch(result)
    {
        case CFG_FILE_ERROR:
            DEBUG("warning: configuration file '%s' could not be read: %s\n",
                    config_file, strerror(errno));
            DEBUG("continuing with default values...\n\n");
        case CFG_SUCCESS:
            break;
        case CFG_PARSE_ERROR:
            return -1;
    }

    webcontrol->deny_by_default = (strcmp(default_behavior, "deny") == 0) ? 1 : 0;

    if((webcontrol->logfile = fopen(logfile, "w")) == NULL)
    {
        ERROR("Unable to open logfile %s", logfile);
    }

    CALL_HOOK_SIMPLE(config_file_parsed);

    return 0;
}

int main(int argc, char** argv)
{
    char c;
    size_t num_threads = 2;
    buffer* plugin_list = NULL;
    //buffer* config_file = NULL;
    char config_file[PATH_MAX];

    while ((c = getopt (argc, argv, "c:t:p:h")) != -1)
    {
        switch (c)
        {
            case 'c':
                //config_file = buffer_init_string(optarg);
                strcpy(config_file, optarg);
                break;
            case 't':
                sscanf(optarg, "%zu", &num_threads);
                break;
            case 'p':
                plugin_list = buffer_init_string(optarg);
                break;
            case 'h':
            default:
                fprintf(stderr, "WebControl Squid Helper\n\nUsage:\n"
                                "-c foo.conf                      - load config file \"foo.conf\"\n"                
                                "-p \"plugin1,plugin2,...,pluginN\" - load specified plugins\n"
                                "-t 4                             - start 4 threads\n");
                exit(-1);
        }
    }
    
    /* Probably the most important thing */
    setvbuf(stdout, NULL, _IONBF, 0);

    setnonblock(STDIN_FILENO);

    webcontrol_core_ctx *webcontrol = malloc(sizeof(webcontrol_core_ctx));
    webcontrol->request_counter = 0;

    pthread_mutex_init(&webcontrol->stdout_lock, NULL);

    pthread_key_create(&current_thread_key, NULL);

    webcontrol->tmp_line_buf = buffer_init();

    webcontrol->plugins = calloc(sizeof(plugin), 10);
    webcontrol->num_plugins = 0;

    webcontrol->active_workers = 0;

    if(config_file == NULL)
    {
        //config_file = buffer_init_string(DEFAULT_CONFIG_FILE_NAME);
        strcpy(config_file, DEFAULT_CONFIG_FILE_NAME);
    }

    DEBUG("Starting WebControl with %zu Threads...\n", num_threads);
    DEBUG("Using config file %s\n", config_file);

    if(plugin_list)
    {
        DEBUG("Loading plugins %s\n", plugin_list->ptr);
    }
    
    webcontrol->loop = ev_default_loop(0);
    ev_set_userdata(webcontrol->loop, webcontrol);

    plugins_load_string(webcontrol, plugin_list);

    if(init_config(config_file, webcontrol) == -1)
    {
        DEBUG("CONFIG FILE ERROR\n");
        exit(-1);
    }

    //webcontrol_core_ctx *data = calloc(sizeof(webcontrol_core_ctx), 1);

    queue_t inQueue = QUEUE_INITIALIZER(webcontrol->job_queue_buffer);
    webcontrol->job_queue = &inQueue;

    queue_t signalQueue = QUEUE_INITIALIZER(webcontrol->thread_signal_queue_buffer);
    webcontrol->thread_signal_queue = &signalQueue;

    thread* workers = (thread*)calloc(sizeof(thread), num_threads);
    size_t i;



    #if 0
    ev_io_init(&webcontrol->io_stdin_watcher, stdin_read_cb, 0, EV_READ);
    ev_io_start(webcontrol->loop, &webcontrol->io_stdin_watcher);

    struct ev_signal signal_watcher;
    ev_signal_init (&signal_watcher, sigint_cb, SIGINT);
    ev_signal_start (webcontrol->loop, &signal_watcher);
    ev_signal_init (&signal_watcher, sigint_cb, SIGTERM);
    ev_signal_start (webcontrol->loop, &signal_watcher);

    ev_async_init(&webcontrol->thread_sig, thread_sig_cb);
    ev_async_start(webcontrol->loop, &webcontrol->thread_sig);

    //ev_io_init(&webcontrol->io_stdout_watcher, stdout_write_cb, 0, EV_WRITE);
    //ev_io_start(webcontrol->loop, &webcontrol->io_stdout_watcher);
    #endif

    DEBUG("haaha");


    webcontrol->ev_stdin_watcher = malloc(sizeof(ev_io));
    webcontrol->ev_signal_watcher = malloc(sizeof(ev_signal));
    webcontrol->ev_thread_signal_watcher = malloc(sizeof(ev_async));

    /* Handle stdin read */
    ev_io_init(webcontrol->ev_stdin_watcher, stdin_read_cb, 0, EV_READ);
    ev_io_start(webcontrol->loop, webcontrol->ev_stdin_watcher);

    DEBUG("haaha");


    /* Handle SIGINT */
    ev_signal_init (webcontrol->ev_signal_watcher, sigint_cb, SIGINT);
    ev_signal_start (webcontrol->loop, webcontrol->ev_signal_watcher);

    /* Handle SIGTERM */
    ev_signal_init (webcontrol->ev_signal_watcher, sigint_cb, SIGTERM);
    ev_signal_start (webcontrol->loop, webcontrol->ev_signal_watcher);

    /* Handle async thread signals */
    ev_async_init(webcontrol->ev_thread_signal_watcher, thread_sig_cb);
    ev_async_start(webcontrol->loop, webcontrol->ev_thread_signal_watcher);

    //pthread_setspecific(webcontrol_global_ctx_key, webcontrol);

    for (size_t i=0; i<num_threads; ++i)
    {
        workers[i].webcontrol = webcontrol;
        workers[i].number = i+1;
        pthread_create(&workers[i].pthread, NULL, (void*)worker, &workers[i]);
    }

    webcontrol->active_workers = num_threads;

    ev_loop(webcontrol->loop, 0);

    //return;

    //return;

    DEBUG("EVENT LOOP TERMINATED\n");

    INFO("Handled %zu requests\n", webcontrol->request_counter);

    for(i=0; i<num_threads; ++i)
    {
        DEBUG("Cancelling Thread\n");
        //pthread_cancel(workers[i].pthread);

        // Place an empty line for every thread to terminate in the queue
        queue_enqueue(webcontrol->job_queue, NULL);
    }

    DEBUG("Waiting for workers to join\n");

    for (i = 0; i < num_threads; ++ i)
    {
        pthread_join(workers[i].pthread, NULL);
        DEBUG("WORKER JOINED\n");
    }


    for(i = 0; i < webcontrol->num_plugins; ++i)
    {
        plugin* currentPlugin = webcontrol->plugins[i];
        if(currentPlugin->plugin_free != NULL)
        {
            DEBUG("Call plugin free\n");
            currentPlugin->plugin_free(currentPlugin);

            plugin_unload(currentPlugin);

            //free(currentPlugin);
        }
    }

    ev_loop_destroy(webcontrol->loop);

    if(plugin_list)
    {
        buffer_free(plugin_list);
    }

    //buffer_free(webcontrol->line_buf);

    if(webcontrol->logfile)
    {
        fclose(webcontrol->logfile);
    }   

    free(webcontrol->plugins);
    cfg_free(webcontrol->cfg);
    //free(webcontrol->opts);
    //buffer_free(config_file);
    free(webcontrol);
    free(workers);

    fflush(stdout);
    fflush(stderr);

    return 0;
}
