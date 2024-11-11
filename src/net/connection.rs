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
use crate::core::errors::RelayError;
use dl_srt_rust::errors::SrtError;
use dl_srt_rust::{SrtOptionValue, SrtSocketConnection, SrtSocketOptions};
use std::fmt;
use std::fmt::{Display, Formatter};

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
    write!(
      f,
      "Stream ID: {}, Connection Type: {}, Socket Connection: {}",
      self.stream_id, self.connection_type, self.socket
    )
  }
}

impl StreamConnection {
  pub fn new(
    socket: SrtSocketConnection,
    connection_type: ConnectionType,
  ) -> Result<Self, RelayError> {
    tracing::debug!("{} stream connected", connection_type);
    let stream_id = match socket.get_sock_flag(SrtSocketOptions::SrtOptStreamID) {
      Ok(SrtOptionValue::String(stream_id)) => stream_id,
      Ok(other) => {
        tracing::error!("Failed to get stream id: Unexpected value: {:?}", other);
        return Err(RelayError::GeneralError);
      }
      Err(e) => {
        tracing::error!("Failed to get stream id: {}", e);
        return Err(RelayError::SrtError(e));
      }
    };
    tracing::debug!("{} Stream ID: {}", connection_type, stream_id);
    Ok(Self {
      socket,
      connection_type,
      stream_id,
    })
  }
}
