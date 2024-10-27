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

#include "StreamSession.h"

StreamSession::StreamSession(SRTSOCKET socket, const std::string &streamId, DisconnectCallback onDisconnect)
    : m_socket(socket), m_streamId(streamId), m_onDisconnect(std::move(onDisconnect)) {
}

StreamSession::~StreamSession() {
    m_isDisconnecting = true;
    stopPublishing();
    removeAllSubscribers();
    closeSocket(m_socket);
}

bool StreamSession::addSubscriber(SRTSOCKET socket) {
    std::lock_guard<std::mutex> lock(m_subscribersMutex);
    m_subscribers.push_back(socket);
    std::cout << "Added subscriber to stream " << m_streamId << std::endl;
    return true;
}

void StreamSession::removeSubscriber(SRTSOCKET socket) {
    std::lock_guard<std::mutex> lock(m_subscribersMutex);
    m_subscribers.erase(
        std::remove(m_subscribers.begin(), m_subscribers.end(), socket),
        m_subscribers.end()
    );
    closeSocket(socket);
}

void StreamSession::removeAllSubscribers() {
    std::lock_guard<std::mutex> lock(m_subscribersMutex);
    for (auto &subscriber: m_subscribers) {
        closeSocket(subscriber);
    }
    m_subscribers.clear();
}

bool StreamSession::startPublishing() {
    if (m_running.exchange(true)) {
        std::cerr << "Already publishing stream " << m_streamId << std::endl;
        return false;
    }

    m_publisherThread = std::make_unique<std::thread>(&StreamSession::publisherThread, this);
    return true;
}

void StreamSession::stopPublishing() {
    if (m_running.exchange(false)) {
        if (m_publisherThread && m_publisherThread->joinable()) {
            m_publisherThread->join();
        }
    }
}

void StreamSession::publisherThread() {
    std::vector<char> buffer(BUFFER_SIZE);

    while (m_running.load(std::memory_order_relaxed)) {
        // Check if we're disconnecting before any socket operations
        if (m_isDisconnecting.load(std::memory_order_acquire)) {
            break;
        }

        int bytesReceived = srt_recv(m_socket, buffer.data(), BUFFER_SIZE);
        if (bytesReceived == SRT_ERROR) {
            if (!m_running.load(std::memory_order_acquire)) {
                break;
            }
            std::cerr << "Failed to receive data from publisher: " << srt_getlasterror_str() << std::endl;
            handleDisconnect();
            break;
        }

        // Make a copy of subscribers to avoid a long lock
        std::vector<SRTSOCKET> currentSubscribers; {
            std::lock_guard<std::mutex> lock(m_subscribersMutex);
            currentSubscribers = m_subscribers;
        }

        // If no subscribers, continue receiving (but not sending)
        if (currentSubscribers.empty()) {
            continue;
        }

        std::vector<SRTSOCKET> failedSubscribers;
        for (const auto &subscriber: currentSubscribers) {
            if (m_isDisconnecting.load(std::memory_order_acquire)) {
                break;
            }
            int bytesSent = srt_send(subscriber, buffer.data(), bytesReceived);
            if (bytesSent == SRT_ERROR) {
                std::cerr << "Failed to send data to subscriber: " << srt_getlasterror_str() << std::endl;
                failedSubscribers.push_back(subscriber);
            }
        }

        // Remove failed subscribers
        if (!failedSubscribers.empty()) {
            std::lock_guard<std::mutex> lock(m_subscribersMutex);
            for (const auto &failedSubscriber: failedSubscribers) {
                closeSocket(failedSubscriber);
                m_subscribers.erase(
                    std::remove(m_subscribers.begin(), m_subscribers.end(), failedSubscriber),
                    m_subscribers.end()
                );
            }
        }
    }
}

void StreamSession::handleDisconnect() {
    try {
        if (!m_isDisconnecting.exchange(true)) {
            stopPublishing();
            removeAllSubscribers();
            if (m_onDisconnect) {
                m_onDisconnect(m_streamId);
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception in disconnect handler: " << e.what() << std::endl;
    }
}

void StreamSession::closeSocket(SRTSOCKET socket) {
    if (socket != SRT_INVALID_SOCK) {
        srt_close(socket);
    }
}
