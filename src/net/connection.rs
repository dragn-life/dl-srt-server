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
use std::sync::Arc;
use std::sync::atomic::{AtomicBool, Ordering};
use dl_srt_rust::SrtSocketConnection;
use tokio::sync::broadcast;
use crate::config::Settings;
use crate::core::errors::RelayError;
use crate::net::stream::StreamData;

#[derive(Debug, Clone, Copy)]
pub enum ConnectionType {
  InputStream,
  OutputStream,
}

pub struct Connection {
  pub socket: SrtSocketConnection,
  pub connection_type: ConnectionType,
  pub settings: Arc<Settings>,
}

impl Connection {
  pub fn new(socket: SrtSocketConnection, connection_type: ConnectionType, settings: Arc<Settings>) -> Result<Self, RelayError> {
    let connection = Self { socket, connection_type, settings };
    Ok(connection)
  }

  pub async fn handle_connection_data(
    self,
    running: Arc<AtomicBool>,
    tx: broadcast::Sender<StreamData>,
  ) -> Result<(), RelayError> {
    match self.connection_type {
      ConnectionType::InputStream => self.handle_input_stream_data(running, tx).await,
      ConnectionType::OutputStream => self.handle_output_stream_data(running, tx).await,
    }
  }

  async fn handle_input_stream_data(
    self,
    running: Arc<AtomicBool>,
    tx: broadcast::Sender<StreamData>,
  ) -> Result<(), RelayError> {
    while running.load(Ordering::SeqCst) {
      match self.socket.recv(self.settings.buffer_size as i32) {
        Ok(data) => {
          tracing::debug!("Received data from input stream");
          let stream_data = StreamData {
            data,
            timestamp: chrono::Utc::now().timestamp_millis() as u64,
          };
          if let Err(e) = tx.send(stream_data) {
            tracing::error!("Broadcast Error: {:?}", e);
            break;
          }
        }
        Err(e) => {
          tracing::error!("Input Stream Received Error: {:?}", e);
          break;
        }
      }
    }
    Ok(())
  }

  async fn handle_output_stream_data(
    self,
    running: Arc<AtomicBool>,
    tx: broadcast::Sender<StreamData>,
  ) -> Result<(), RelayError> {
    let mut rx = tx.subscribe();

    while running.load(Ordering::SeqCst) {
      match rx.recv().await {
        Ok(stream_data) => {
          tracing::debug!("Sending data to output stream");
          if let Err(e) = self.socket.send(&stream_data.data) {
            tracing::error!("Failed to send to output stream: {:?}", e);
            break;
          }
        }
        Err(e) => {
          tracing::error!("Output Stream Receive Error: {:?}", e);
          break;
        }
      }
    }
    Ok(())
  }
}
