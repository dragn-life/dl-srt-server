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

#include "StreamManager.h"
#include <atomic>
#include <thread>
#include "utils/SRTHandler.h"


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
    void handleConnections(SRTSOCKET listener, bool isPublisher);

    bool initializeSrt();

    SRTSOCKET createSocket(int port);

    // Server state
    std::atomic<bool> m_running{false};
    SRTSOCKET m_publisherSocket;
    SRTSOCKET m_subscriberSocket;

    std::shared_ptr<StreamManager> m_streamManager;
    std::unique_ptr<std::thread> m_publisherThread;
    std::unique_ptr<std::thread> m_subscriberThread;

    // TODO: Move to configuration file
    static constexpr int PUBLISHER_PORT = 5500;
    static constexpr int SUBSCRIBER_PORT = 6000;
    static constexpr int BACKLOG = 10;
};


#endif SRT_SERVER_H
