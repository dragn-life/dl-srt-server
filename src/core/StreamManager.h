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

class StreamManager {
public:
    StreamManager();

    ~StreamManager();

    // Stream management
    bool addPublisher(SRTSOCKET socket, const std::string &streamId);

    bool addSubscriber(SRTSOCKET socket, const std::string &streamId);

    void removeSession(SRTSOCKET socket);
    void removeStream(const std::string &streamId);

    // Stream validation
    bool validateStreamId(const std::string &streamId);

    std::string extractStreamId(SRTSOCKET socket);

private:
    std::mutex m_sessionsMutex;
    std::unordered_map<std::string, std::shared_ptr<StreamSession> > m_sessionsByStreamId;
    std::unordered_map<SRTSOCKET, std::shared_ptr<StreamSession> > m_sessionsBySocket;
};


#endif //STREAMMANAGER_H
