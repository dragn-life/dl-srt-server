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

#ifndef STREAMHANDLER_H
#define STREAMHANDLER_H
#include <string>

static const int STREAM_ERROR = -1;

class StreamHandler {
public:
    virtual ~StreamHandler() = default;

    virtual bool disconnect() = 0;

    virtual int receive(char *buffer, int len) = 0;

    virtual int send(const char *buffer, int len) = 0;

    virtual bool isConnected() const = 0;

    virtual std::string getStreamId() const = 0;

    virtual std::string getLastErrorMessage() const = 0;
};


#endif //STREAMHANDLER_H
