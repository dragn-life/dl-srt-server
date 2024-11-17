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
use dl_srt_rust::SrtSocketConnection;
use tokio::task::JoinHandle;

#[derive(Debug)]
pub struct StreamManager {
  incoming_stream: SrtSocketConnection,
  outgoing_streams: Vec<SrtSocketConnection>,
  // incoming_handle: JoinHandle<()>,
  outgoing_handles: Vec<JoinHandle<()>>,
}

impl StreamManager {
  pub fn new(incoming_stream: SrtSocketConnection) -> StreamManager {
    StreamManager {
      incoming_stream,
      outgoing_streams: Vec::new(),
      outgoing_handles: Vec::new(),
    }
  }

  pub fn run_stream_relay(self) {}
}
