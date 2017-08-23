/*
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#ifndef __VPP_INSPECT_H__
#define __VPP_INSPECT_H__

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <uv.h>

namespace VPP
{
    /**
     * A means to inspect the state VPP has built, in total, and per-client
     * To use do:
     *   socat - UNIX-CONNECT:/path/to/sock/in/opflex.conf
     * and follow the instructions
     */
    class inspect
    {
    public:
        /**
         * Constructor
         */
        inspect(const std::string &sockname);

        /**
         * Destructor to tidyup socket resources
         */
        ~inspect();

        /**
         * inspect command handler Handler
         */
        class command_handler
        {
        public:
            command_handler() = default;
            virtual ~command_handler() = default;

            /**
             * Show each object
             */
            virtual void show(std::ostream &os) = 0;
        };

        /**
         * Register a command handler for inspection
         */
        static void register_handler(const std::vector<std::string> &cmds,
                                     const std::string &help,
                                     command_handler *ch);
    private:
        /**
         * Call operator for running in the thread
         */
        static void run(void* ctx);

        /**
         * A write request
         */
        struct write_req_t
        {
            write_req_t(std::ostringstream &output);
            ~write_req_t();

            uv_write_t req;
            uv_buf_t buf;
        };

        /**
         * Write a ostream to the client
         */
        static void do_write(uv_stream_t *client, std::ostringstream &output);

        /**
         * Called on creation of a new connection
         */
        static void on_connection(uv_stream_t* server,
                                  int status);

        /**
         * Call when data is written
         */
        static void on_write(uv_write_t *req, int status);

        /**
         * Called when data is read
         */
        static void on_read(uv_stream_t *client,
                            ssize_t nread,
                            const uv_buf_t *buf);

        /**
         * Called to allocate buffer space for data to be read
         */
        static void on_alloc_buffer(uv_handle_t *handle,
                                    size_t size,
                                    uv_buf_t *buf);

        /**
         * Called to cleanup the thread and socket during destruction
         */
        static void on_cleanup(uv_async_t* handle);

        /**
         * Async handle so we can wakeup the loop
         */
        uv_async_t m_async;

        /**
         * The libuv loop
         */
        uv_loop_t m_server_loop;

        /**
         * The libuv thread context in which we run the loop
         */
        uv_thread_t m_server_thread;

        /**
         * The inspect unix domain socket name, from the config file
         */
        std::string m_sock_name;

        /**
         * command handler list
         */
        static std::unique_ptr<std::map<std::string, command_handler*>> m_cmd_handlers;
        /**
         * help handler list
         */
        static std::unique_ptr<std::deque<std::pair<std::vector<std::string>, std::string>>> m_help_handlers;
    };
};

#endif
