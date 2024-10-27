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

#ifndef STREAMMANAGER_H
#define STREAMMANAGER_H

#include <unordered_map>

#include "StreamSession.h"
#include "utils/StreamHandler.h"

class StreamManager {
public:
    StreamManager();

    ~StreamManager();

    // Stream management
    bool onPublisherConnected(std::shared_ptr<StreamHandler> publisherHandler);

    bool onSubscriberConnected(std::shared_ptr<StreamHandler> subscriber);

    void removePublishingStream(std::shared_ptr<StreamHandler> connection);

    // Stream validation
    bool validateStreamId(const std::string &streamId);

protected:
    std::mutex &getSessionsMutex() { return m_sessionsMutex; }

    std::unordered_map<std::string, std::shared_ptr<StreamSession> > &getSessionsByStreamId() {
        return m_sessionsByStreamId;
    }

private:
    std::mutex m_sessionsMutex;
    std::unordered_map<std::string, std::shared_ptr<StreamSession> > m_sessionsByStreamId;
};


#endif //STREAMMANAGER_H
