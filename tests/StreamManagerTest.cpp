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

#include <core/StreamManager.h>

#include "MockSRTHandler.h"

class StreamManagerTestHelper : public StreamManager {
    using StreamManager::StreamManager;

public:
    // Expose protected members for testing
    std::mutex &getSessionsMutex() { return StreamManager::getSessionsMutex(); }

    std::unordered_map<std::string, std::shared_ptr<StreamSession> > &getSessionsByStreamId() {
        return StreamManager::getSessionsByStreamId();
    }
};

class StreamManagerTest : public ::testing::Test {
protected:
    std::shared_ptr<StreamManagerTestHelper> streamManager;

    std::shared_ptr<MockSRTHandler> publisherAHandler;
    std::shared_ptr<MockSRTHandler> subscriberAHandler;

    std::shared_ptr<MockSRTHandler> publisherBHandler;
    std::shared_ptr<MockSRTHandler> subscriberBHandler;

    void SetUp() override {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
        streamManager = std::make_shared<StreamManagerTestHelper>();
        publisherAHandler = std::make_shared<MockSRTHandler>("test-stream-A");
        subscriberAHandler = std::make_shared<MockSRTHandler>("test-stream-A");

        publisherBHandler = std::make_shared<MockSRTHandler>("test-stream-B");
        subscriberBHandler = std::make_shared<MockSRTHandler>("test-stream-B");
    }

    void TearDown() override {
        streamManager.reset();
        publisherAHandler.reset();
        subscriberAHandler.reset();
        publisherBHandler.reset();
        subscriberBHandler.reset();
        _CrtDumpMemoryLeaks();
    }
};

TEST_F(StreamManagerTest, OnPublisherConnectedAddsStream) {
    publisherAHandler->expectReceivingData("Some Test Data", 14);

    EXPECT_TRUE(streamManager->getSessionsByStreamId().empty());

    EXPECT_TRUE(streamManager->onPublisherConnected(publisherAHandler));

    // Verify that the stream was added
    std::unordered_map<std::string, std::shared_ptr<StreamSession> > sessionsByStreamId;
    {
        std::lock_guard<std::mutex> lock(streamManager->getSessionsMutex());
        sessionsByStreamId = streamManager->getSessionsByStreamId();
    }

    EXPECT_TRUE(sessionsByStreamId.find(publisherAHandler->getStreamId()) != sessionsByStreamId.end());
    // Verify Counts:
    EXPECT_EQ(sessionsByStreamId.size(), 1);
}

TEST_F(StreamManagerTest, RemoveStreamRemovesStream) {
    publisherAHandler->expectReceivingData("Some Test Data", 14);

    EXPECT_TRUE(streamManager->getSessionsByStreamId().empty());

    EXPECT_TRUE(streamManager->onPublisherConnected(publisherAHandler));

    streamManager->removePublishingStream(publisherAHandler);

    // Wait for the publisher to clean up
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify that the stream session was removed
    std::unordered_map<std::string, std::shared_ptr<StreamSession> > sessionsByStreamId;
    {
        std::lock_guard<std::mutex> lock(streamManager->getSessionsMutex());
    }
    EXPECT_TRUE(sessionsByStreamId.empty());
}

// TODO: WIP: Fix publisher disconnection crash
TEST_F(StreamManagerTest, OnPublisherDisconnectRemovesStream) {
    publisherAHandler->expectReceivingDataDisconnects();

    // Stream should Disconnect when no data is received
    EXPECT_CALL(*publisherAHandler, disconnect()).Times(1);

    EXPECT_TRUE(streamManager->onPublisherConnected(publisherAHandler));

    // Wait for the publisher to clean up
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify that the stream session was removed
    std::unordered_map<std::string, std::shared_ptr<StreamSession> > sessionsByStreamId;
    {
        std::lock_guard<std::mutex> lock(streamManager->getSessionsMutex());
    }
    EXPECT_TRUE(sessionsByStreamId.empty());
}
