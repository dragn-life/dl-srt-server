# DL-SRT-Server

## Description
DL-SRT-Server is a specialized SRT (Secure Reliable Transport) server implementation focused on efficient stream multiplexing and secure stream management. Built using [Haivision's libsrt](https://github.com/Haivision/srt), this server provides a robust solution for handling multiple SRT streams with authentication and validation capabilities.

This server is used in with [My Stream Pal](https://mystreampal.com) for low-latency IRL re-streaming from mobile devices to your local OBS setup.

## Status
🚧 **Currently Under Development** 🚧

## Key Features
- SRT stream multiplexing support using [Haivision SRT Library (libsrt)](https://github.com/Haivision/srt)
- Stream validation callback mechanism
- Support for multiple concurrent streams
- Low latency re-streaming

## Use Case
The primary use case is to facilitate high-quality, low-latency streaming from mobile devices to [OBS (Open Broadcaster Software)](https://obsproject.com/):
- Streamers can use [Moblin](https://github.com/eerimoq/moblin) to stream via SRT protocol with unique stream IDs
- The server multiplexes these streams and manages authentication
- OBS can connect to specific streams using the stream ID
- Perfect for:
    - IRL Streaming on [Twitch](https://www.twitch.tv/)
    - Multi-camera sources in OBS

## Architecture
```
Mobile Streams                  Cloud                      OBS Clients
                                   
┌─────────────────┐                                     ┌─────────────────┐
│  Moblin Client  │                                     │   OBS Client    │
│  Stream ID: 1   │─┐                                 ┌─│ Listening to 1  │
└─────────────────┘ │                                 │ └─────────────────┘
                    │    ┌──────────────────┐         │
┌─────────────────┐ ├────│  DL-SRT-Server   │─────────┤ ┌─────────────────┐
│  Moblin Client  │─┤    │    IP:PORT       │         ├─│   OBS Client    │
│  Stream ID: 2   │ │    │                  │         │ │ Listening to 2  │
└─────────────────┘ │    │   ┌──────────┐   │         │ └─────────────────┘
                    │    │   │   Auth   │   │         │
┌─────────────────┐ │    │   │  System  │   │         │ ┌─────────────────┐
│  Moblin Client  │─┘    │   └──────────┘   │         └─│   OBS Client    │
│  Stream ID: 3   │      └──────────────────┘           │ Listening to 3  │
└─────────────────┘                                     └─────────────────┘
```
## License
This project is licensed under the GNU Affero General Public License v3.0 (AGPL-3.0). See the [LICENSE](LICENSE) file for details.
