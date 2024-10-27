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

#ifndef SRTHANDLER_H
#define SRTHANDLER_H

#include <srt/srt.h>

#include "StreamHandler.h"

class SRTHandler : public StreamHandler {
public:
    explicit SRTHandler() {
    }

    std::string extractStreamId() const;

    bool connect(SRTSOCKET listeningSocket);

    bool disconnect() override;

    int receive(char *buffer, int len) override;

    int send(const char *buffer, int len) override;

    bool isConnected() const override;

    std::string getStreamId() const override { return m_streamId; };

    std::string getLastErrorMessage() const override;

private:
    SRTSOCKET m_socket;
    std::string m_streamId;
};


#endif //SRTHANDLER_H
