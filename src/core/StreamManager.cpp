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

bool StreamManager::onPublisherConnected(std::shared_ptr<StreamHandler> publisherHandler) {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);

    // Check if Stream ID already exists
    if (m_sessionsByStreamId.find(publisherHandler->getStreamId()) != m_sessionsByStreamId.end()) {
        std::cerr << "Stream ID " << publisherHandler->getStreamId() << " already exists" << std::endl;
        return false;
    }

    // Create a disconnect callback
    auto onPublisherDisconnected = [this](const std::shared_ptr<StreamHandler> publisherHandler) {
        removePublishingStream(publisherHandler);
    };

    auto session = std::make_shared<StreamSession>(publisherHandler, onPublisherDisconnected);
    m_sessionsByStreamId[publisherHandler->getStreamId()] = session;

    // Start publishing
    if (!session->startPublishing()) {
        // If already publishing, remove the session
        m_sessionsByStreamId.erase(publisherHandler->getStreamId());
        return false;
    }

    std::cout << "Added publisher to stream " << publisherHandler->getStreamId() << std::endl;
    return true;
}

void StreamManager::removePublishingStream(std::shared_ptr<StreamHandler> publisherHandler) {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);

    auto it = m_sessionsByStreamId.find(publisherHandler->getStreamId());
    if (it != m_sessionsByStreamId.end()) {
        auto sessionToCleanup = it->second;
        m_sessionsByStreamId.erase(it);
    }
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
    return true;
}

bool StreamManager::validateStreamId(const std::string &streamId) {
    if (streamId.empty()) {
        std::cerr << "Stream ID cannot be empty" << std::endl;
        return false;
    }
    // TODO: Validate stream ID via callback api
    return true;
}
