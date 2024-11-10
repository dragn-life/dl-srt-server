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

use crate::config::Settings;
use crate::core::errors::RelayError;
use crate::net::connection::{ConnectionType, StreamConnection};
use crate::net::listener::SrtListener;
use std::collections::HashMap;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use tokio::sync::{RwLock};

pub struct RelayServer {
  settings: Settings,
  running: Arc<AtomicBool>,
  // TODO: Build out a ConnectionsManager to link incoming streams to outgoing streams
  incoming_stream_connections: Arc<RwLock<HashMap<String, Arc<StreamConnection>>>>,
  outgoing_stream_connections: Arc<RwLock<HashMap<String, Arc<StreamConnection>>>>,
}

impl RelayServer {
  pub fn new() -> Self {
    // TODO: Configure settings in main, pass in here (for possible cli args and/or settings file)
    let settings = Settings::default();
    let running = Arc::new(AtomicBool::new(true));
    RelayServer {
      settings,
      running,
      incoming_stream_connections: Arc::new(RwLock::new(HashMap::new())),
      outgoing_stream_connections: Arc::new(RwLock::new(HashMap::new())),
    }
  }

  pub async fn run(&self) -> Result<(), RelayError> {
    tracing::info!("Starting SRT relay server...");
    tracing::info!("Incoming Streams Listening on: {}", self.settings.input_stream_port);
    tracing::info!("Outgoing Streams Listening on: {}", self.settings.output_stream_port);

    dl_srt_rust::startup_srt()?;

    let incoming_stream_handle = match self.listen_for_input_streams() {
      Ok(handle) => handle,
      Err(err) => {
        tracing::error!("Failed to start incoming stream listener: {}", err);
        return Err(RelayError::GeneralError);
      }
    };

    let outgoing_stream_handle = match self.listen_for_output_streams() {
      Ok(handle) => handle,
      Err(err) => {
        tracing::error!("Failed to start outgoing stream listener: {}", err);
        return Err(RelayError::GeneralError);
      }
    };

    let relay_handle = match self.start_relay_task() {
      Ok(handle) => handle,
      Err(err) => {
        tracing::error!("Failed to start relay task: {}", err);
        return Err(RelayError::GeneralError);
      }
    };

    tokio::select! {
      _ = tokio::signal::ctrl_c() => {
        tracing::info!("Shutting down server...");
        self.running.store(false, Ordering::Relaxed);
      },
      _ = incoming_stream_handle => {
        tracing::info!("Incoming stream listener has stopped");
      },
      _ = outgoing_stream_handle => {
        tracing::info!("Outgoing stream listener has stopped");
      },
      _ = relay_handle => {
        tracing::info!("Relay task has stopped");
      },
    }

    Ok(())
  }

  fn listen_for_input_streams(&self) -> Result<tokio::task::JoinHandle<()>, RelayError> {
    let listener = match SrtListener::new(self.settings.input_stream_port) {
      Ok(listener) => listener,
      Err(err) => {
        tracing::error!("Failed to create listener: {}", err);
        return Err(RelayError::GeneralError);
      }
    };

    let running = Arc::clone(&self.running);
    let connections = Arc::clone(&self.incoming_stream_connections);

    Ok(tokio::spawn(async move {
      while running.load(Ordering::Relaxed) {
        tracing::info!("Waiting for input stream connection...");
        let socket = match listener.accept_connection() {
          Ok(socket) => socket,
          Err(err) => {
            tracing::error!("Failed to accept connection: {}", err);
            continue;
          }
        };
        let connection = match StreamConnection::new(socket, ConnectionType::InputStream) {
          Ok(connection) => Arc::new(connection),
          Err(err) => {
            tracing::error!("Failed to create connection: {}", err);
            continue;
          }
        };

        let id = connection.stream_id.clone();
        tracing::info!("Incoming Stream Connected {}", connection.socket);
        {
          let mut connections = connections.write().await;
          connections.insert(id, connection);
        }
        tracing::info!("Incoming Stream Added to Map");
      }
    }))
  }

  fn listen_for_output_streams(&self) -> Result<tokio::task::JoinHandle<()>, RelayError> {
    let listener = match SrtListener::new(self.settings.output_stream_port) {
      Ok(listener) => listener,
      Err(err) => {
        tracing::error!("Failed to create listener: {}", err);
        return Err(RelayError::GeneralError);
      }
    };

    let running = Arc::clone(&self.running);
    let connections = Arc::clone(&self.outgoing_stream_connections);

    Ok(tokio::spawn(async move {
      while running.load(Ordering::Relaxed) {
        tracing::info!("Waiting for output stream connection...");
        let socket = match listener.accept_connection() {
          Ok(socket) => socket,
          Err(err) => {
            tracing::error!("Failed to accept connection: {}", err);
            continue;
          }
        };
        let connection = match StreamConnection::new(socket, ConnectionType::OutputStream) {
          Ok(connection) => Arc::new(connection),
          Err(err) => {
            tracing::error!("Failed to create connection: {}", err);
            continue;
          }
        };

        let id = connection.stream_id.clone();
        tracing::info!("Outgoing Stream Connected {}", connection.socket);
        {
          let mut connections = connections.write().await;
          connections.insert(id, connection);
        }
        tracing::info!("Outgoing Stream Added to Map");
      }
    }))
  }

  fn start_relay_task(&self) -> Result<tokio::task::JoinHandle<()>, RelayError> {
    tracing::info!("Starting Relay Task...");
    let buffer_size = self.settings.buffer_size;
    let in_connections = Arc::clone(&self.incoming_stream_connections);
    let out_connections = Arc::clone(&self.outgoing_stream_connections);
    let running = Arc::clone(&self.running);
    Ok(tokio::spawn(async move {
      tracing::debug!("Relay Task Spawned...");
      while running.load(Ordering::Relaxed) {
        handle_relay(buffer_size as i32, &in_connections, &out_connections).await;
      }
    }))
  }

  // TODO: Proper Shutdown of server
  pub fn shutdown(&self) {
    self.running.store(false, Ordering::SeqCst);
    dl_srt_rust::cleanup_srt().unwrap_or_default();
  }
}

// TODO: Split into separate threads per stream connection
async fn handle_relay(buffer_size: i32, in_connections: &Arc<RwLock<HashMap<String, Arc<StreamConnection>>>>, out_connections: &Arc<RwLock<HashMap<String, Arc<StreamConnection>>>>) {
  // let incoming_stream_connections = in_connections.read().await;

  let incoming_stream;
  {
    // Try read for debugging deadlocks
    let incoming_stream_connections = match in_connections.try_read() {
      Ok(guard) => guard,
      Err(_err) => {
        tracing::error!("Relay Failed to read incoming connections: {}", _err);
        return;
      }
    };

    // Get first incoming stream
    incoming_stream = match incoming_stream_connections.get("abc") {
      Some(connection) => Some((connection.stream_id.clone(), Arc::clone(connection))),
      None => None,
    };
  }
  if incoming_stream.is_none() {
    return;
  }
  // tracing::debug!("Incoming Stream Found...");
  let (_, incoming_connection) = incoming_stream.unwrap();

  let incoming_socket = &incoming_connection.socket;

  let incoming_data = match incoming_socket.recv(buffer_size) {
    Ok(data) => data,
    Err(err) => {
      // Remove incoming stream connection
      // TODO: WIP will detect if socket is closed and remove connection
      {
        let mut incoming_stream_connections = in_connections.write().await;
        incoming_stream_connections.remove(&incoming_connection.stream_id);
      }
      tracing::error!("Failed to receive data from incoming stream: {}", err);
      return;
    }
  };

  let outgoing_stream;
  {
    let outgoing_stream_connections = out_connections.read().await;
    outgoing_stream = match outgoing_stream_connections.get("abc") {
      Some(connection) => Some((connection.stream_id.clone(), Arc::clone(connection))),
      None => None,
    };
  }

  if outgoing_stream.is_none() {
    return;
  }
  let (_, out_connection) = outgoing_stream.unwrap();
  let outgoing_socket = &out_connection.socket;
  match outgoing_socket.send(&incoming_data) {
    Ok(_) => {
      // tracing::debug!("Successful Relay");
    }
    Err(_err) => {
      // TODO: WIP will detect if socket is closed and remove connection
      {
        let mut outgoing_stream_connections = out_connections.write().await;
        outgoing_stream_connections.remove(&out_connection.stream_id);
      }
      // tracing::error!("Failed to relay data...");
    }
  };
}