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

#include "StreamManager.h"

#include <iostream>

StreamManager::StreamManager() = default;

StreamManager::~StreamManager() = default;

bool StreamManager::onPublisherConnected(std::shared_ptr<StreamHandler> publisher) {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);

    // Check if Stream ID already exists
    if (m_sessionsByStreamId.find(publisher->getStreamId()) != m_sessionsByStreamId.end()) {
        std::cerr << "Stream ID " << publisher->getStreamId() << " already exists" << std::endl;
        return false;
    }

    // Create a disconnect callback
    auto onDisconnect = [this](const std::string &streamId) {
        removeStream(streamId);
    };

    auto session = std::make_shared<StreamSession>(publisher, onDisconnect);
    m_sessionsByStreamId[publisher->getStreamId()] = session;
    m_sessionsByConnection[publisher] = session;

    // Start publishing
    if (!session->startPublishing()) {
        // If already publishing, remove the session
        m_sessionsByStreamId.erase(publisher->getStreamId());
        m_sessionsByConnection.erase(publisher);
        return false;
    }

    std::cout << "Added publisher to stream " << publisher->getStreamId() << std::endl;
    return true;
}

bool StreamManager::onSubscriberConnected(std::shared_ptr<StreamHandler> subscriber) {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);

    auto it = m_sessionsByStreamId.find(subscriber->getStreamId());
    if (it == m_sessionsByStreamId.end()) {
        std::cerr << "Stream Session for ID " << subscriber->getStreamId() << " not found" << std::endl;
        return false;
    }
    auto streamSession = it->second;

    streamSession->addSubscriber(subscriber);
    m_sessionsByConnection[subscriber] = streamSession;
    return true;
}

void StreamManager::removeSession(std::shared_ptr<StreamHandler> connection) {
    std::shared_ptr<StreamSession> sessionToCleanup;

    bool isPublisher = false;
    std::string streamId; {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);

        auto it = m_sessionsByConnection.find(connection);
        if (it != m_sessionsByConnection.end()) {
            auto connectionToCleanup = it->first;
            sessionToCleanup = it->second;
            streamId = connection->getStreamId();
            isPublisher = connectionToCleanup == connection;

            // Remove the connection mapping
            m_sessionsByConnection.erase(it);

            // If this is a publisher, remove the entire stream map
            if (isPublisher) {
                m_sessionsByStreamId.erase(streamId);
            }
        }
    }

    // Cleanup outside the lock
    if (sessionToCleanup) {
        if (isPublisher) {
            std::cout << "Publisher disconnected, removed stream " << streamId << std::endl;
        } else {
            // This is a subscriber
            sessionToCleanup->removeSubscriber(connection);
            std::cout << "Subscriber disconnected from stream " << streamId << std::endl;
        }
    }
}

void StreamManager::removeStream(const std::string &streamId) {
    std::shared_ptr<StreamSession> sessionToCleanup; {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);

        auto it = m_sessionsByStreamId.find(streamId);
        if (it != m_sessionsByStreamId.end()) {
            sessionToCleanup = it->second;

            // Remove all connection mappings for this session
            auto connection_it = m_sessionsByConnection.begin();
            while (connection_it != m_sessionsByConnection.end()) {
                if (connection_it->second == sessionToCleanup) {
                    connection_it = m_sessionsByConnection.erase(connection_it);
                } else {
                    ++connection_it;
                }
            }

            // Finally remove the session itself
            m_sessionsByStreamId.erase(it);
        }
    }
}

bool StreamManager::validateStreamId(const std::string &streamId) {
    if (streamId.empty()) {
        std::cerr << "Stream ID cannot be empty" << std::endl;
        return false;
    }
    // TODO: Validate stream ID via callback api
    return true;
}
