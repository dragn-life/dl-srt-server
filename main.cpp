#include <iostream>
#include <cstring>
#include <cstdlib>

#include <srt/srt.h>

// Stub for validating stream ID
bool validate_stream_id(const std::string& stream_id) {
    // TODO: Ping API to validate stream ID
    std::cout << "Validating stream ID: " << stream_id << std::endl;
    return true;
}

int main() {
    // Initialize SRT Library
    if (srt_startup() != 0) {
        std::cerr << "Failed to initialize SRT library" << std::endl;
        return EXIT_FAILURE;
    }

    // Create SRT Socket
    SRTSOCKET srt_socket = srt_create_socket();
    if (srt_socket == SRT_ERROR) {
        std::cerr << "Failed to create SRT socket" << std::endl;
        srt_cleanup();
        return EXIT_FAILURE;
    }

    // Bind to port 5500
    // TODO: Read from configuration file
    sockaddr_in srt_address;
    memset(&srt_address, 0, sizeof(srt_address));
    srt_address.sin_family = AF_INET;
    srt_address.sin_port = htons(5500);
    srt_address.sin_addr.s_addr = INADDR_ANY;

    if (srt_bind(srt_socket, (sockaddr*)&srt_address, sizeof(srt_address)) == SRT_ERROR) {
        std::cerr << "Failed to bind to port 5500" << std::endl;
        srt_close(srt_socket);
        srt_cleanup();
        return EXIT_FAILURE;
    }

    // Listen for Connections
    if (srt_listen(srt_socket, 5) == SRT_ERROR) {
        std::cerr << "Failed to listen for connections" << std::endl;
        srt_close(srt_socket);
        srt_cleanup();
        return EXIT_FAILURE;
    }

    std::cout << "Listening for connections on port 5500" << std::endl;

    //while (true) {
    //}

    // Close SRT Socket
    srt_close(srt_socket);
    srt_cleanup();

    return 0;
}
