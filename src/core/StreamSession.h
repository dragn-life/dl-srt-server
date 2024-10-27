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

#ifndef STREAMSESSION_H
#define STREAMSESSION_H

#include <functional>
#include <mutex>
#include <srt/srt.h>
#include <string>
#include <thread>
#include <vector>


class StreamSession {
public:
    using DisconnectCallback = std::function<void(const std::string &)>;

    StreamSession(SRTSOCKET socket, const std::string &streamId, DisconnectCallback onDisconnect);

    ~StreamSession();

    // Getters
    SRTSOCKET getSocket() const { return m_socket; }
    std::string getStreamId() const { return m_streamId; }

    // Add/remove subscribers
    bool addSubscriber(SRTSOCKET socket);

    void removeSubscriber(SRTSOCKET socket);

    void removeAllSubscribers();

    // Start/stop publishing
    bool startPublishing();

    void stopPublishing();

private:
    void publisherThread();

    void closeSocket(SRTSOCKET socket);

    void handleDisconnect();

    const SRTSOCKET m_socket;
    const std::string m_streamId;
    DisconnectCallback m_onDisconnect;

    std::atomic<bool> m_running{false};
    std::unique_ptr<std::thread> m_publisherThread;

    std::mutex m_subscribersMutex;
    std::vector<SRTSOCKET> m_subscribers;
    std::atomic<bool> m_isDisconnecting{false};

    // TODO: Move to configuration file
    static constexpr int BUFFER_SIZE = 1456;
};


#endif //STREAMSESSION_H
