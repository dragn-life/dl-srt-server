/*
 * dl_srt_server
 * Copyright (C) 2024 DragN Life LLC (Adam B)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef SRT_SERVER_H
#define SRT_SERVER_H

#include <srt/srt.h>
#include <string>
#include <vector>
#include <atomic>
#include <thread>


class SRTServer {
public:
    SRTServer();

    ~SRTServer();

    // Initialize SRT and create listening sockets
    bool initialize();

    // Start the server (non-blocking)
    bool start();

    // Stop the server and cleanup
    void stop();

private:
    // Main server threads
    void handleIncomingStreams();

    void handleOutgoingStreams();

    // Helper methods
    bool initializeSrt();

    SRTSOCKET createSocket(int port, bool isListener);

    void closeSocket(SRTSOCKET &socket);

    // Server state
    std::atomic<bool> m_running;
    SRTSOCKET m_input_socket;
    SRTSOCKET m_output_socket;
    std::vector<SRTSOCKET> m_output_clients;

    // Threads
    std::thread m_input_thread;
    std::thread m_output_thread;

    // TODO: Move to configuration file
    static constexpr int INPUT_PORT = 5500;
    static constexpr int OUTPUT_PORT = 6000;
    static constexpr int BUFFER_SIZE = 1456; // SRT default packet size
    static constexpr int BACKLOG = 10;
};


#endif SRT_SERVER_H
