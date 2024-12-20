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

#include <mutex>
#include <thread>
#include <vector>

#include "utils/StreamEvents.h"
#include "utils/StreamHandler.h"


class StreamSession {
public:
    explicit StreamSession(
        std::shared_ptr<StreamHandler> streamHandler,
        std::weak_ptr<StreamEventListener> eventListener
    );

    ~StreamSession();

    void cleanupSession();

    void waitForCleanup();

    // Getters
    std::shared_ptr<StreamHandler> getStreamHandler() const { return m_publisherHandler; }

    // Add/remove subscribers
    void addSubscriber(std::shared_ptr<StreamHandler> subscriber);

    void removeSubscriber(std::shared_ptr<StreamHandler> subscriber);

    bool startPublishing();

protected:
    std::atomic<bool> &getRunning() { return m_running; }
    std::atomic<bool> &getIsDisconnecting() { return m_isDisconnecting; }

    std::unique_ptr<std::thread> &getPublisherThread() { return m_publisherThread; }
    std::thread::id getPublisherThreadId() const { return m_publisherThreadId; }

    std::lock_guard<std::mutex> getSubscribersMutex() { return std::lock_guard<std::mutex>(m_subscribersMutex); }
    const std::vector<std::shared_ptr<StreamHandler> > &getSubscribers() const { return m_subscribers; }

    std::atomic<bool> &getCleanupDone() { return m_cleanupDone; }

    void removeAllSubscribers();

private:
    void publisherThread();

    void notifyDisconnect() const;

    std::shared_ptr<StreamHandler> m_publisherHandler;

    std::weak_ptr<StreamEventListener> m_eventListener;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_isDisconnecting{false};

    std::unique_ptr<std::thread> m_publisherThread;
    std::thread::id m_publisherThreadId;

    std::mutex m_subscribersMutex;
    std::vector<std::shared_ptr<StreamHandler> > m_subscribers;

    std::mutex m_cleanupMutex;
    std::condition_variable m_cleanupCV;
    std::atomic<bool> m_cleanupDone{false};

    // TODO: Move to configuration file
    static constexpr int BUFFER_SIZE = 1456;
};
#endif //STREAMSESSION_H
