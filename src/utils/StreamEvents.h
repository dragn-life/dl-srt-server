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

#ifndef STREAMEVENTS_H
#define STREAMEVENTS_H

#include <memory>
#include <string>

class StreamHandler;

enum class StreamEventType {
    PublisherDisconnected,
};

struct StreamEvent {
    StreamEventType type;
    std::string streamId;
    std::shared_ptr<StreamHandler> handler;
};


class StreamEventListener {
public:
    virtual ~StreamEventListener() = default;

    virtual void onStreamEvent(const StreamEvent &event) = 0;
};


#endif //STREAMEVENTS_H
