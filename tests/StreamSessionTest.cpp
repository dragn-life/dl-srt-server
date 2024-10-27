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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <utils/SRTHandler.h>

#include <core/StreamSession.h>

#include "MockSRTHandler.h"


class StreamSessionTestHelper : public StreamSession {
public:
    using StreamSession::StreamSession;

    // Expose the protected members for testing
    std::atomic<bool> &getRunning() { return StreamSession::getRunning(); }
    std::atomic<bool> &getIsDisconnecting() { return StreamSession::getIsDisconnecting(); }

    std::unique_ptr<std::thread> &getPublisherThread() { return StreamSession::getPublisherThread(); }
    std::thread::id getPublisherThreadId() const { return StreamSession::getPublisherThreadId(); }

    std::lock_guard<std::mutex> getSubscribersMutex() { return StreamSession::getSubscribersMutex(); }

    const std::vector<std::shared_ptr<StreamHandler> > &getSubscribers() const {
        return StreamSession::getSubscribers();
    }

    void cleanupSession() {
        StreamSession::cleanupSession();
    }
};

class StreamSessionTest : public ::testing::Test {
protected:
    std::shared_ptr<MockSRTHandler> publisherHandler;
    std::shared_ptr<MockSRTHandler> subscriberHandler;

    
    void SetUp() override {
        publisherHandler = std::make_shared<MockSRTHandler>("test-stream-id");
        subscriberHandler = std::make_shared<MockSRTHandler>("test-stream-id");
    }

    void TearDown() override {
    }
};

TEST_F(StreamSessionTest, AddSubscriberSuccess) {
    auto onDisconnect = [](const std::string &) {
    };
    StreamSessionTestHelper session(publisherHandler, onDisconnect);

    session.addSubscriber(subscriberHandler);

    // Verify that the subscriber was added
    std::vector<std::shared_ptr<StreamHandler> > subscribers; {
        std::lock_guard<std::mutex> lock(session.getSubscribersMutex());
        subscribers = session.getSubscribers();
    }
    EXPECT_EQ(subscribers.size(), 1);
    EXPECT_EQ(subscribers[0], subscriberHandler);
}

TEST_F(StreamSessionTest, RemoveSubscriberSuccess) {
    auto onDisconnect = [](const std::string &) {
    };
    StreamSessionTestHelper session(publisherHandler, onDisconnect);

    session.addSubscriber(subscriberHandler);
    session.removeSubscriber(subscriberHandler);

    // Verify that the subscriber was removed
    std::vector<std::shared_ptr<StreamHandler> > subscribers; {
        std::lock_guard<std::mutex> lock(session.getSubscribersMutex());
        subscribers = session.getSubscribers();
    }
    EXPECT_TRUE(subscribers.empty());
}

TEST_F(StreamSessionTest, PublisherShutdownCleansUpThreads) {
    auto onDisconnect = [](const std::string &) {
    };

    publisherHandler->expectReceivingData("test data", 9);

    EXPECT_CALL(*publisherHandler, disconnect()).Times(1);

    StreamSessionTestHelper session(publisherHandler, onDisconnect);

    EXPECT_TRUE(session.startPublishing());

    // Let the publisher thread run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify Session is running
    EXPECT_TRUE(session.getRunning());

    // Verify that the publisher thread is running
    EXPECT_NE(session.getPublisherThreadId(), std::thread::id());

    session.cleanupSession();

    // Wait for the publisher thread to join
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify that the publisher thread has stopped
    EXPECT_FALSE(session.getRunning());

    // Verify that the publisher thread is running
    EXPECT_NE(session.getPublisherThreadId(), std::thread::id());

    // Verify that the publisher thread has joined
    EXPECT_EQ(session.getPublisherThread(), nullptr);
}

TEST_F(StreamSessionTest, PublisherShutdownCleansUpSubscribers) {
    auto onDisconnect = [](const std::string &) {
    };

    publisherHandler->expectReceivingData("test data", 9);

    StreamSessionTestHelper session(publisherHandler, onDisconnect);

    EXPECT_TRUE(session.startPublishing());

    // Let the publisher thread run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify Session is running
    EXPECT_TRUE(session.getRunning());

    session.addSubscriber(subscriberHandler);

    EXPECT_CALL(*publisherHandler, disconnect()).Times(1);
    EXPECT_CALL(*subscriberHandler, disconnect()).Times(1);

    // Verify that the subscriber was added
    std::vector<std::shared_ptr<StreamHandler> > subscribers; {
        std::lock_guard<std::mutex> lock(session.getSubscribersMutex());
        subscribers = session.getSubscribers();
    }
    EXPECT_EQ(subscribers.size(), 1);
    EXPECT_EQ(subscribers[0], subscriberHandler);

    session.cleanupSession();

    // Verify that the subscriber was removed
    subscribers.clear();
    {
        std::lock_guard<std::mutex> lock(session.getSubscribersMutex());
        subscribers = session.getSubscribers();
    }
    EXPECT_TRUE(subscribers.empty());
}

TEST_F(StreamSessionTest, SubscribersReceivesData) {
    auto onDisconnect = [](const std::string &) {
    };

    StreamSessionTestHelper session(publisherHandler, onDisconnect);

    const char testData[] = "some test data";
    const char testDataSend[] = "some test data";

    publisherHandler->expectReceivingData(testData, strlen(testData));
    subscriberHandler->expectSendingData(testDataSend, strlen(testDataSend));

    EXPECT_TRUE(session.startPublishing());

    session.addSubscriber(subscriberHandler);

    // Wait for data to be sent and received
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_CALL(*publisherHandler, disconnect()).Times(1);
    EXPECT_CALL(*subscriberHandler, disconnect()).Times(1);

    session.cleanupSession();
}
