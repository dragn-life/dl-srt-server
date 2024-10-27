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

#ifndef MOCKSRTHANDLER_H
#define MOCKSRTHANDLER_H

#include <gmock/gmock.h>
#include <utils/SRTHandler.h>


class MockSRTHandler : public SRTHandler {
public:
    explicit MockSRTHandler(std::string streamId);

    MOCK_METHOD(std::string, getStreamId, (), (const, override));
    MOCK_METHOD(int, receive, (char *, int), (override));
    MOCK_METHOD(int, send, (const char *, int), (override));
    MOCK_METHOD(bool, disconnect, (), (override));

    void expectReceivingData(const char *data, int len);
    void expectSendingData(const char *data, int len);
};

#endif //MOCKSRTHANDLER_H
