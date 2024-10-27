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

StreamManager::StreamManager() = default;

StreamManager::~StreamManager() = default;

bool StreamManager::addPublisher(SRTSOCKET socket, const std::string &streamId) {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);

    // Check if Stream ID already exists
    if (m_sessionsByStreamId.find(streamId) != m_sessionsByStreamId.end()) {
        std::cerr << "Stream ID " << streamId << " already exists" << std::endl;
        return false;
    }

    // Create a disconnect callback
    auto onDisconnect = [this](const std::string &streamId) {
        removeStream(streamId);
    };

    auto session = std::make_shared<StreamSession>(socket, streamId, onDisconnect);
    m_sessionsByStreamId[streamId] = session;
    m_sessionsBySocket[socket] = session;

    // Start publishing
    if (!session->startPublishing()) {
        // If already publishing, remove the session
        m_sessionsByStreamId.erase(streamId);
        m_sessionsBySocket.erase(socket);
        return false;
    }

    std::cout << "Added publisher to stream " << streamId << std::endl;
    return true;
}

bool StreamManager::addSubscriber(SRTSOCKET socket, const std::string &streamId) {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);

    auto it = m_sessionsByStreamId.find(streamId);
    if (it == m_sessionsByStreamId.end()) {
        std::cerr << "Stream Session for ID " << streamId << " not found" << std::endl;
        return false;
    }
    auto streamSession = it->second;

    if (!streamSession->addSubscriber(socket)) {
        return false;
    }

    m_sessionsBySocket[socket] = streamSession;
    return true;
}

void StreamManager::removeSession(SRTSOCKET socket) {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);

    auto it = m_sessionsBySocket.find(socket);
    if (it == m_sessionsBySocket.end()) {
        return;
    }
    auto streamSession = it->second;
    const std::string &streamId = streamSession->getStreamId();

    if (streamSession->getSocket() == socket) {
        // This is a publisher - remove the entire stream
        m_sessionsByStreamId.erase(streamId);
        m_sessionsBySocket.erase(socket);
        std::cout << "Publisher disconnected, removed stream " << streamId << std::endl;
    } else {
        // This is a subscriber
        streamSession->removeSubscriber(socket);
        m_sessionsBySocket.erase(socket);
        std::cout << "Subscriber disconnected from stream " << streamId << std::endl;
    }
}

void StreamManager::removeStream(const std::string &streamId) {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);

    auto it = m_sessionsByStreamId.find(streamId);
    if (it == m_sessionsByStreamId.end()) {
        return;
    }
    auto streamSession = it->second;

    // First remove all subscribers
    streamSession->removeAllSubscribers();

    // Remove all socket mappings for this session
    auto socket_it = m_sessionsBySocket.begin();
    while (socket_it != m_sessionsBySocket.end()) {
        if (socket_it->second == streamSession) {
            socket_it = m_sessionsBySocket.erase(socket_it);
        } else {
            ++socket_it;
        }
    }

    // Finally remove the session itself
    m_sessionsByStreamId.erase(it);

    std::cout << "Removed stream " << streamId << std::endl;
}

bool StreamManager::validateStreamId(const std::string &streamId) {
    if (streamId.empty()) {
        std::cerr << "Stream ID cannot be empty" << std::endl;
        return false;
    }
    // TODO: Validate stream ID via callback api
    return true;
}

std::string StreamManager::extractStreamId(SRTSOCKET socket) {
    char streamId[512];
    int streamIdLen = 512;

    if (srt_getsockflag(socket, SRTO_STREAMID, &streamId, &streamIdLen) == SRT_ERROR) {
        std::cerr << "Failed to extract stream ID from socket" << std::endl;
        return "";
    }
    return std::string(streamId, streamIdLen);
}
