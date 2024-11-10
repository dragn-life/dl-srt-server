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
use std::fmt;
use std::fmt::{Display, Formatter};
use crate::core::errors::RelayError;
use dl_srt_rust::{SrtSocketConnection};

#[derive(Debug, Clone, Copy)]
pub enum ConnectionType {
  InputStream,
  OutputStream,
}
impl Display for ConnectionType {
  fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
    match self {
      ConnectionType::InputStream => write!(f, "Input Stream"),
      ConnectionType::OutputStream => write!(f, "Output Stream"),
    }
  }
}

pub struct StreamConnection {
  pub socket: SrtSocketConnection,
  pub connection_type: ConnectionType,
  pub stream_id: String,
}

impl Display for StreamConnection {
  fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
    write!(f, "Stream ID: {}, Connection Type: {}, Socket Connection: {}", self.stream_id, self.connection_type, self.socket)
  }
}

impl StreamConnection {
  pub fn new(socket: SrtSocketConnection, connection_type: ConnectionType) -> Result<Self, RelayError> {
    // let stream_id = match socket.get_sock_flag(SrtOptStreamID) {
    //   Ok(SrtOptionValue::String(stream_id)) => stream_id,
    //   _ => {
    //     tracing::error!("Failed to get stream id");
    //     // Return fake stream id
    //     // TODO: Fix this - this is a temporary fix, getting stream id failing atm
    //     "abc".to_string()
    //   }
    // };
    let stream_id = "abc".to_string();
    Ok(Self {
      socket,
      connection_type,
      stream_id,
    })
  }
}
