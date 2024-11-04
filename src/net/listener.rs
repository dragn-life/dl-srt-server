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
use dl_srt_rust::{SrtOptionValue, SrtSocketConnection};
use dl_srt_rust::SrtSocketOptions::{SrtOptRCVSYN, SrtOptReuseAddr};
use tokio::sync::broadcast;
use crate::config::Settings;
use crate::core::errors::RelayError;
use crate::net::connection::{Connection, ConnectionType};
use crate::net::stream::StreamData;

#[derive(Clone)]
pub struct SrtListener {
  socket: Arc<SrtSocketConnection>,
  connection_type: ConnectionType,
  settings: Arc<Settings>,
}

impl SrtListener {
  pub fn new(port: u16, connection_type: ConnectionType, settings: Settings) -> Result<Self, RelayError> {
    let socket = SrtSocketConnection::new()?;

    socket.set_sock_opt(0, SrtOptRCVSYN, SrtOptionValue::Bool(true))?;
    socket.set_sock_opt(0, SrtOptReuseAddr, SrtOptionValue::Bool(true))?;

    socket.bind(port)?;
    socket.listen(2)?;

    println!("Created {:?}, Listening on port: {port}", connection_type);

    Ok(Self {
      socket: Arc::new(socket),
      connection_type,
      settings: Arc::new(settings),
    })
  }

  pub fn start_listening(
    &self,
    running: Arc<AtomicBool>,
    tx: broadcast::Sender<StreamData>,
  ) -> tokio::task::JoinHandle<()> {
    let listener = self.clone();

    tokio::spawn(async move {
      if let Err(e) = listener.accept_connections(running, tx).await {
        eprintln!("Error accepting connections: {:?}", e);
      }
    })
  }

  async fn accept_connections(
    &self,
    running: Arc<AtomicBool>,
    tx: broadcast::Sender<StreamData>,
  ) -> Result<(), RelayError> {
    while running.load(Ordering::SeqCst) {
      match self.connection_type {
        ConnectionType::InputStream => {
          tracing::info!("Waiting for input stream connection...");
        }
        ConnectionType::OutputStream => {
          tracing::info!("Waiting for output stream connection...");
        }
      };

      match self.socket.accept() {
        Ok(socket) => {
          // TODO: Get Peer IP address
          println!("{:?} connected", self.connection_type);

          let connection = Connection::new(
            socket,
            self.connection_type,
            Arc::clone(&self.settings),
          )?;

          let running = Arc::clone(&running);
          let tx = tx.clone();

          tokio::spawn(async move {
            if let Err(e) = connection.handle_connection_data(running, tx).await {
              tracing::error!("Error handling connection: {:?}", e);
            }
          });
        }
        Err(e) => {
          tracing::error!("Error accepting connection: {:?}", e);
          tokio::time::sleep(tokio::time::Duration::from_secs(1)).await;
        }
      }
    }
    Ok(())
  }
}
