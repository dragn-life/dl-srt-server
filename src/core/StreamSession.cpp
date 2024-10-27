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

#include <iostream>

StreamSession::StreamSession(
    std::shared_ptr<StreamHandler> streamHandler,
    DisconnectCallback onDisconnect)
    : m_streamHandler(std::move(streamHandler)),
      m_onDisconnect(std::move(onDisconnect)) {
}

StreamSession::~StreamSession() {
    cleanupSession();
}

void StreamSession::cleanupSession() {
    if (m_isDisconnecting.exchange(true)) {
        return;
    }
    m_running = false;

    try {
        removeAllSubscribers();

        // Only try to join if we're not in the publisher thread
        if (m_publisherThread && m_publisherThread->joinable() && m_publisherThreadId != std::this_thread::get_id()) {
            m_publisherThread->join();
            m_publisherThread.reset();
        }

        m_streamHandler->disconnect();
    } catch (const std::exception &e) {
        std::cerr << "Exception in StreamSession cleanup: " << e.what() << std::endl;
    }
}

void StreamSession::addSubscriber(std::shared_ptr<StreamHandler> subscriber) {
    std::lock_guard<std::mutex> lock(m_subscribersMutex);
    m_subscribers.push_back(subscriber);
    std::cout << "Added subscriber to stream " << m_streamHandler->getStreamId() << std::endl;
}

void StreamSession::removeSubscriber(std::shared_ptr<StreamHandler> subscriber) {
    std::lock_guard<std::mutex> lock(m_subscribersMutex);
    m_subscribers.erase(
        std::remove(m_subscribers.begin(), m_subscribers.end(), subscriber),
        m_subscribers.end()
    );
    subscriber->disconnect();
}

void StreamSession::removeAllSubscribers() {
    std::cout << "Removing all subscribers from stream " << m_streamHandler->getStreamId() << std::endl;
    std::lock_guard<std::mutex> lock(m_subscribersMutex);
    for (auto &subscriber: m_subscribers) {
        subscriber->disconnect();
    }
    m_subscribers.clear();
}

bool StreamSession::startPublishing() {
    if (m_running.exchange(true)) {
        std::cerr << "Already publishing stream " << m_streamHandler->getStreamId() << std::endl;
        return false;
    }

    m_publisherThread = std::make_unique<std::thread>(&StreamSession::publisherThread, this);
    return true;
}

void StreamSession::publisherThread() {
    m_publisherThreadId = std::this_thread::get_id();
    std::vector<char> buffer(BUFFER_SIZE);

    while (m_running.load(std::memory_order_relaxed)) {
        // Check if we're disconnecting before any socket operations
        if (m_isDisconnecting.load(std::memory_order_acquire)) {
            break;
        }

        int bytesReceived = m_streamHandler->receive(buffer.data(), BUFFER_SIZE);
        if (bytesReceived == STREAM_ERROR) {
            if (!m_running.load(std::memory_order_acquire)) {
                break;
            }
            std::cerr << "Failed to receive data from publisher: " << m_streamHandler->getLastErrorMessage() <<
                    std::endl;

            // Notify the disconnect handler
            if (m_onDisconnect) {
                m_onDisconnect(m_streamHandler->getStreamId());
            }
            break;
        }

        // Make a copy of subscribers to avoid a long lock
        std::vector<std::shared_ptr<StreamHandler> > currentSubscribers; {
            std::lock_guard<std::mutex> lock(m_subscribersMutex);
            currentSubscribers = m_subscribers;
        }

        // If no subscribers, continue receiving (but not sending)
        if (currentSubscribers.empty()) {
            continue;
        }

        std::vector<std::shared_ptr<StreamHandler> > failedSubscribers;
        for (const auto &subscriber: currentSubscribers) {
            if (m_isDisconnecting.load(std::memory_order_acquire)) {
                break;
            }
            int bytesSent = subscriber->send(buffer.data(), bytesReceived);
            if (bytesSent == STREAM_ERROR) {
                std::cerr << "Failed to send data to subscriber: " << m_streamHandler->getLastErrorMessage() <<
                        std::endl;
                failedSubscribers.push_back(subscriber);
            }
        }

        // Remove failed subscribers
        if (!failedSubscribers.empty()) {
            std::lock_guard<std::mutex> lock(m_subscribersMutex);
            for (const auto &failedSubscriber: failedSubscribers) {
                failedSubscriber->disconnect();
                m_subscribers.erase(
                    std::remove(m_subscribers.begin(), m_subscribers.end(), failedSubscriber),
                    m_subscribers.end()
                );
            }
        }
    }
}
