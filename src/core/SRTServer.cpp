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
    : m_running(false)
      , m_input_socket(SRT_INVALID_SOCK)
      , m_output_socket(SRT_INVALID_SOCK) {
}

SRTServer::~SRTServer() {
    stop();
}

bool SRTServer::initialize() {
    if (!initializeSrt()) {
        std::cerr << "Failed to initialize SRT" << std::endl;
        return false;
    }

    m_input_socket = createSocket(INPUT_PORT, true);
    if (m_input_socket == SRT_INVALID_SOCK) {
        std::cerr << "Failed to create input socket" << std::endl;
        return false;
    }

    m_output_socket = createSocket(OUTPUT_PORT, true);
    if (m_output_socket == SRT_INVALID_SOCK) {
        std::cerr << "Failed to create output socket" << std::endl;
        closeSocket(m_input_socket);
        return false;
    }

    std::cout << "SRT Server initialized" << std::endl;
    std::cout << "Input port: " << INPUT_PORT << std::endl;
    std::cout << "Output port: " << OUTPUT_PORT << std::endl;

    return true;
}

bool SRTServer::start() {
    if (m_running.exchange(true)) {
        std::cerr << "Server is already running" << std::endl;
        return false;
    }

    try {
        m_input_thread = std::thread(&SRTServer::handleIncomingStreams, this);
        m_output_thread = std::thread(&SRTServer::handleOutgoingStreams, this);
    } catch (...) {
        // If thread creation fails, reset the running flag
        m_running.store(false);
        throw;
    }

    return true;
}

void SRTServer::stop() {
    if (!m_running.exchange(false)) {
        return;
    }

    // Close all client connections
    for (auto &client: m_output_clients) {
        closeSocket(client);
    }

    m_output_clients.clear();

    // Close server sockets
    closeSocket(m_input_socket);
    closeSocket(m_output_socket);

    // Wait for threads to finish
    if (m_input_thread.joinable()) {
        m_input_thread.join();
    }

    if (m_output_thread.joinable()) {
        m_output_thread.join();
    }

    srt_cleanup();
}

bool SRTServer::initializeSrt() {
    if (srt_startup() < 0) {
        std::cerr << "Failed to initialize SRT" << std::endl;
        return false;
    }
    return true;
}

SRTSOCKET SRTServer::createSocket(int port, bool isListener) {
    SRTSOCKET sock = srt_create_socket();
    if (sock == SRT_INVALID_SOCK) {
        std::cerr << "Failed to create SRT socket" << srt_getlasterror_str() << std::endl;
        return SRT_INVALID_SOCK;
    }

    // Set SRT Options
    int yes = 1;
    srt_setsockopt(sock, 0, SRTO_RCVSYN, &yes, sizeof(yes));

    if (isListener) {
        srt_setsockopt(sock, 0, SRTO_REUSEADDR, &yes, sizeof(yes));
    }

    sockaddr_in socketAddress;
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_port = htons(port);
    socketAddress.sin_addr.s_addr = INADDR_ANY;

    if (isListener) {
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
    }

    return sock;
}

void SRTServer::closeSocket(SRTSOCKET &sock) {
    if (sock != SRT_INVALID_SOCK) {
        srt_close(sock);
        sock = SRT_INVALID_SOCK;
    }
}

void SRTServer::handleIncomingStreams() {
    std::vector<char> buffer(BUFFER_SIZE);

    while (m_running) {
        SRTSOCKET clientSocket = srt_accept(m_input_socket, nullptr, nullptr);
        if (clientSocket == SRT_INVALID_SOCK) {
            std::cerr << "Failed to accept incoming connection: " << srt_getlasterror_str() << std::endl;
            continue;
        }

        std::cout << "Accepted incoming connection" << std::endl;

        // Redistribute to all output clients
        for (auto it = m_output_clients.begin(); it != m_output_clients.end();) {
            int bytesReceived = srt_recv(clientSocket, buffer.data(), BUFFER_SIZE);
            if (bytesReceived == SRT_ERROR) {
                std::cerr << "Failed to receive data from client: " << srt_getlasterror_str() << std::endl;
                break;
            }

            // Redistribute to all output clients
            for (auto it = m_output_clients.begin(); it != m_output_clients.end();) {
                int bytesSent = srt_send(*it, buffer.data(), bytesReceived);
                if (bytesSent == SRT_ERROR) {
                    std::cerr << "Failed to send data to client: " << srt_getlasterror_str() << std::endl;
                    closeSocket(*it);
                    it = m_output_clients.erase(it);
                } else {
                    ++it;
                }
            }
        }
        closeSocket(clientSocket);
    }
}

void SRTServer::handleOutgoingStreams() {
    while (m_running) {
        SRTSOCKET clientSocket = srt_accept(m_output_socket, nullptr, nullptr);
        if (clientSocket == SRT_INVALID_SOCK) {
            std::cerr << "Failed to accept outgoing connection: " << srt_getlasterror_str() << std::endl;
            continue;
        }

        std::cout << "Accepted outgoing connection" << std::endl;
        m_output_clients.push_back(clientSocket);
    }
}

