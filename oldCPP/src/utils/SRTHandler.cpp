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

#include "SRTHandler.h"

bool SRTHandler::connect(SRTSOCKET listeningSocket) {
    m_socket = srt_accept(listeningSocket, nullptr, nullptr);

    if (m_socket == SRT_INVALID_SOCK) {
        return false;
    }
    m_streamId = extractStreamId();
    return true;
}


bool SRTHandler::disconnect() {
    return srt_close(m_socket) >= 0;
}

int SRTHandler::receive(char *buffer, int len) {
    return srt_recv(m_socket, buffer, len);
}

int SRTHandler::send(const char *buffer, int len) {
    return srt_send(m_socket, buffer, len);
}

bool SRTHandler::isConnected() const {
    return srt_getsockstate(m_socket) == SRTS_CONNECTED;
}

std::string SRTHandler::getLastErrorMessage() const {
    return srt_getlasterror_str();
}

std::string SRTHandler::extractStreamId() const {
    char streamId[512];
    int streamIdLen = 512;

    if (srt_getsockflag(m_socket, SRTO_STREAMID, &streamId, &streamIdLen) == SRT_ERROR) {
        return "";
    }

    return std::string(streamId, streamIdLen);
}