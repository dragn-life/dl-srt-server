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

#include "MockSRTHandler.h"

MockSRTHandler::MockSRTHandler(std::string streamId) {
    EXPECT_CALL(*this, getStreamId())
            .WillRepeatedly(testing::Return(streamId));
}

void MockSRTHandler::expectReceivingData(const char *data, int len) {
    EXPECT_CALL(*this, receive(testing::_, testing::_))
            .WillRepeatedly(testing::DoAll(
                testing::Invoke([data, len](char *buffer, int bufferLen) {
                    // Simulate receiving some data
                    memcpy(buffer, data, len);
                    return len;
                }),
                testing::Return(len)
            ));
}

void MockSRTHandler::expectSendingData(const char *expectedData, int expectedLen) {
    EXPECT_CALL(*this, send(testing::_, testing::_))
            .WillRepeatedly(testing::DoAll(
                testing::Invoke([expectedData, expectedLen](const char *buffer, int len) {
                    // Verify that the subscriber received the data
                    EXPECT_EQ(len, expectedLen);
                    EXPECT_EQ(memcmp(buffer, expectedData, len), 0);
                    return len;
                }),
                testing::Return(expectedLen)
            ));
}
