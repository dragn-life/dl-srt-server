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

#include "core/SRTServer.h"
#include <iostream>
#include <csignal>

std::unique_ptr<SRTServer> srtServer;

void signalHandler(int) {
    if (srtServer) {
        std::cout << "Stopping server" << std::endl;
        srtServer->stop();
    }
}

int main() {
    // Setup signal handling
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    srtServer = std::make_unique<SRTServer>();

    if (!srtServer->initialize()) {
        std::cerr << "Failed to initialize SRT Server" << std::endl;
        return EXIT_FAILURE;
    }

    if (!srtServer->start()) {
        std::cerr << "Failed to start SRT Server" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "SRT Server started. Press Ctrl+C to stop." << std::endl;

    // Wait for signal
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
