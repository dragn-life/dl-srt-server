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
use dl_srt_rust::SrtSocketOptions::{SrtOptRCVSYN, SrtOptReuseAddr};
use dl_srt_rust::{SrtOptionValue, SrtSocketConnection};
use std::sync::Arc;

#[derive(Clone)]
pub struct SrtListener {
  socket: Arc<SrtSocketConnection>,
}

impl SrtListener {
  pub fn new(port: u16) -> Result<Self, RelayError> {
    let socket = SrtSocketConnection::new()?;

    socket.set_sock_opt(0, SrtOptRCVSYN, SrtOptionValue::Bool(true))?;
    socket.set_sock_opt(0, SrtOptReuseAddr, SrtOptionValue::Bool(true))?;

    if let Err(e) = socket.bind(port) {
      tracing::error!("Failed to bind on port: {}", e);
      return Err(RelayError::SrtError(e));
    }
    socket.listen(2)?;

    Ok(Self {
      socket: Arc::new(socket),
    })
  }

  // TODO: Implement blocking accept via spawning a thread with "isListening" flag or enum states
  pub fn accept_connection(&self) -> Result<SrtSocketConnection, RelayError> {
    match self.socket.accept() {
      Ok(socket) => Ok(socket),
      Err(e) => {
        tracing::error!("Failed to accept connection: {}", e);
        Err(RelayError::SrtError(e))
      }
    }
  }

  // TODO: Implement "isDisconnected" flag or enum states
  pub fn close(&self) {
    match self.socket.close() {
      Ok(_) => (),
      Err(e) => tracing::warn!("Failed to close listener: {}", e),
    }
  }
}
