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

#include "SRTServer.h"
#include <iostream>


SRTServer::SRTServer()
    : m_streamManager(std::make_unique<StreamManager>()) {
}

SRTServer::~SRTServer() {
    stop();
}


bool SRTServer::initialize() {
    if (!initializeSrt()) {
        std::cerr << "Failed to initialize SRT" << std::endl;
        return false;
    }

    m_publisherSocket = createSocket(PUBLISHER_PORT);
    if (m_publisherSocket == SRT_INVALID_SOCK) {
        std::cerr << "Failed to create publisher socket" << std::endl;
        return false;
    }

    m_subscriberSocket = createSocket(SUBSCRIBER_PORT);
    if (m_subscriberSocket == SRT_INVALID_SOCK) {
        std::cerr << "Failed to create subscriber socket" << std::endl;
        srt_close(m_publisherSocket);
        return false;
    }

    std::cout << "SRT Server initialized" << std::endl;
    std::cout << "Publisher port: " << PUBLISHER_PORT << std::endl;
    std::cout << "Subscriber port: " << SUBSCRIBER_PORT << std::endl;

    return true;
}

bool SRTServer::start() {
    if (m_running.exchange(true)) {
        std::cerr << "Server is already running" << std::endl;
        return false;
    }

    m_publisherThread = std::make_unique<std::thread>(&SRTServer::handleConnections, this,
                                                      m_publisherSocket, true);
    m_subscriberThread = std::make_unique<std::thread>(&SRTServer::handleConnections, this,
                                                       m_subscriberSocket, false);

    return true;
}

void SRTServer::stop() {
    if (!m_running.exchange(false)) {
        return;
    }

    srt_close(m_publisherSocket);
    srt_close(m_subscriberSocket);

    if (m_publisherThread && m_publisherThread->joinable()) {
        m_publisherThread->join();
    }
    if (m_subscriberThread && m_subscriberThread->joinable()) {
        m_subscriberThread->join();
    }

    m_streamManager.reset();
    srt_cleanup();
}

void SRTServer::handleConnections(SRTSOCKET listener, bool isPublisher) {
    while (m_running.load(std::memory_order_acquire)) {
        std::shared_ptr<SRTHandler> streamConnection = std::make_shared<SRTHandler>();

        if (!streamConnection->connect(listener)) {
            std::cerr << "Failed to accept incoming connection: " << streamConnection->getLastErrorMessage() <<
                    std::endl;
            continue;
        }

        if (!m_streamManager->validateStreamId(streamConnection->getStreamId())) {
            std::cerr << "Invalid stream ID: " << streamConnection->getStreamId() << std::endl;
            streamConnection->disconnect();
            continue;
        }

        bool success = isPublisher
                           ? m_streamManager->onPublisherConnected(streamConnection)
                           : m_streamManager->onSubscriberConnected(streamConnection);

        if (!success) {
            std::cerr << "Failed to add client to stream manager" << std::endl;
            streamConnection->disconnect();
        }
    }
}

bool SRTServer::initializeSrt() {
    if (srt_startup() == SRT_ERROR) {
        std::cerr << "Failed to initialize SRT" << std::endl;
        return false;
    }
    return true;
}

SRTSOCKET SRTServer::createSocket(int port) {
    SRTSOCKET sock = srt_create_socket();
    if (sock == SRT_INVALID_SOCK) {
        std::cerr << "Failed to create SRT socket" << srt_getlasterror_str() << std::endl;
        return SRT_INVALID_SOCK;
    }

    // Set SRT Options
    int yes = 1;
    srt_setsockopt(sock, 0, SRTO_RCVSYN, &yes, sizeof(yes));
    srt_setsockopt(sock, 0, SRTO_REUSEADDR, &yes, sizeof(yes));


    sockaddr_in socketAddress;
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_port = htons(port);
    socketAddress.sin_addr.s_addr = INADDR_ANY;


    if (srt_bind(sock, (sockaddr *) &socketAddress, sizeof(socketAddress)) == SRT_ERROR) {
        std::cerr << "Failed to bind to socket port " << port << ": " << srt_getlasterror_str() << std::endl;
        srt_close(sock);
        return SRT_INVALID_SOCK;
    }

    if (srt_listen(sock, BACKLOG) == SRT_ERROR) {
        std::cerr << "Failed to listen on socket port " << port << ": " << srt_getlasterror_str() << std::endl;
        srt_close(sock);
        return SRT_INVALID_SOCK;
    }

    return sock;
}
