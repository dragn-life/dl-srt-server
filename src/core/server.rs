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
use std::sync::Arc;
use std::time::Duration;
use tokio::sync::{broadcast, RwLock};
use tokio::task::JoinHandle;

pub struct RelayServer {
    settings: Settings,
    shutdown_tx: broadcast::Sender<()>,
    tasks: Vec<JoinHandle<()>>,
    // TODO: Handle blocking on tokio select for incoming and outgoing streams, tmp fix works
    srt_listeners: Vec<Arc<SrtListener>>,
    // TODO: Build out a ConnectionsManager to link incoming streams to outgoing streams
    incoming_stream_connections: Arc<RwLock<HashMap<String, Arc<StreamConnection>>>>,
    outgoing_stream_connections: Arc<RwLock<HashMap<String, Arc<StreamConnection>>>>,
}

impl RelayServer {
    pub fn new() -> Self {
        // TODO: Configure settings in main, pass in here (for possible cli args and/or settings file)
        let settings = Settings::default();

        // Broadcast channel for shutdown signal
        let (shutdown_tx, _) = broadcast::channel(1);

        RelayServer {
            settings,
            shutdown_tx,
            tasks: Vec::new(),
            srt_listeners: Vec::new(),
            incoming_stream_connections: Arc::new(RwLock::new(HashMap::new())),
            outgoing_stream_connections: Arc::new(RwLock::new(HashMap::new())),
        }
    }

    pub async fn run(&mut self) -> Result<(), RelayError> {
        tracing::info!("Starting SRT relay server...");
        tracing::info!(
            "Incoming Streams Listening on: {}",
            self.settings.input_stream_port
        );
        tracing::info!(
            "Outgoing Streams Listening on: {}",
            self.settings.output_stream_port
        );

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

        self.tasks.push(incoming_stream_handle);
        self.tasks.push(outgoing_stream_handle);
        self.tasks.push(relay_handle);

        tokio::select! {
          _ = tokio::signal::ctrl_c() => {
            tracing::info!("Shutting down server...");
            self.shutdown_tx.send(()).expect("Failed to send shutdown signal");

            // Wait for all tasks to finish
            self.shutdown_tasks().await;
            tracing::info!("Server shutdown complete");
          },
        }

        Ok(())
    }

    fn listen_for_input_streams(&mut self) -> Result<JoinHandle<()>, RelayError> {
        let mut shutdown_rx = self.shutdown_tx.subscribe();

        let listener = match SrtListener::new(self.settings.input_stream_port) {
            Ok(listener) => Arc::new(listener),
            Err(err) => {
                tracing::error!("Failed to create listener: {}", err);
                return Err(RelayError::GeneralError);
            }
        };
        self.srt_listeners.push(Arc::clone(&listener));

        let connections = Arc::clone(&self.incoming_stream_connections);

        Ok(tokio::spawn(async move {
            loop {
                tokio::select! {
                  _ = shutdown_rx.recv() => {
                    tracing::info!("Shutting down incoming stream listener...");

                    // Cleanup connections
                    let mut connections = connections.write().await;
                    connections.clear();

                    tracing::info!("Incoming stream listener shutdown complete");
                    break;
                  }
                  accept_result = async {
                    tracing::info!("Waiting for input stream connection...");
                    // TODO: Handle blocking this by running this in its own blocking thread in SrtListener
                    listener.accept_connection()
                  } => {
                    match accept_result {
                      Ok(socket) => {
                        match StreamConnection::new(socket, ConnectionType::InputStream) {
                          Ok(connection) => {
                            let connection = Arc::new(connection);
                            let id = connection.stream_id.clone();

                            tracing::info!("Incoming Stream Connected {}", connection.socket);

                            {
                              let mut connections = connections.write().await;
                              connections.insert(id, connection);
                            }
                            tracing::info!("Incoming Stream Added to Map");
                          },
                          Err(err) => {
                            tracing::error!("Failed to create connection: {}", err);
                            continue;
                          }
                        }
                      }
                      Err(err) => {
                        // TODO: False errors when shutting down
                        tracing::error!("Failed to accept connection: {}", err);

                        // Add a delay before retrying
                        tokio::time::sleep(Duration::from_secs(1)).await;
                        continue;
                      }
                    }
                  }
                }
            }

            tracing::info!("Incoming stream listener task complete");
        }))
    }

    fn listen_for_output_streams(&mut self) -> Result<JoinHandle<()>, RelayError> {
        let mut shutdown_rx = self.shutdown_tx.subscribe();

        let listener = match SrtListener::new(self.settings.output_stream_port) {
            Ok(listener) => Arc::new(listener),
            Err(err) => {
                tracing::error!("Failed to create listener: {}", err);
                return Err(RelayError::GeneralError);
            }
        };

        self.srt_listeners.push(Arc::clone(&listener));

        let connections = Arc::clone(&self.outgoing_stream_connections);

        Ok(tokio::spawn(async move {
            loop {
                tokio::select! {
                  _ = shutdown_rx.recv() => {
                    tracing::info!("Shutting down outgoing stream listener...");

                    // Cleanup connections
                    let mut connections = connections.write().await;
                    connections.clear();

                    tracing::info!("Outgoing stream listener shutdown complete");
                    break;
                  }
                  accept_result = async {
                    tracing::info!("Waiting for outgoing stream connection...");
                    listener.accept_connection()
                  } => {
                    match accept_result {
                      Ok(socket) => {
                        match StreamConnection::new(socket, ConnectionType::OutputStream) {
                          Ok(connection) => {
                            let connection = Arc::new(connection);
                            let id = connection.stream_id.clone();

                            tracing::info!("Outgoing Stream Connected {}", connection.socket);

                            {
                              let mut connections = connections.write().await;
                              connections.insert(id, connection);
                            }
                            tracing::info!("Outgoing Stream Added to Map");
                          },
                          Err(err) => {
                            tracing::error!("Failed to create connection: {}", err);
                            continue;
                          }
                        }
                      }
                      Err(err) => {
                        // TODO: False errors when shutting down
                        tracing::error!("Failed to accept connection: {}", err);

                        // Add a delay before retrying
                        tokio::time::sleep(Duration::from_secs(1)).await;
                        continue;
                      }
                    }
                  }
                }
            }

            tracing::info!("Incoming stream listener task complete");
        }))
    }

    fn start_relay_task(&self) -> Result<JoinHandle<()>, RelayError> {
        tracing::info!("Starting Relay Task...");
        let buffer_size = self.settings.buffer_size;
        let in_connections = Arc::clone(&self.incoming_stream_connections);
        let out_connections = Arc::clone(&self.outgoing_stream_connections);
        let mut shutdown_rx = self.shutdown_tx.subscribe();

        Ok(tokio::spawn(async move {
            tracing::debug!("Relay Task Spawned...");
            loop {
                tokio::select! {
                  Ok(_) = shutdown_rx.recv() => {
                    tracing::info!("Shutting down relay task...");
                    break;
                  }
                  _ = async {
                    handle_relay(buffer_size as i32, &in_connections, &out_connections).await;
                  } => {
                    // no-op, loop continues
                    continue;
                  }
                }
            }
        }))
    }

    async fn shutdown_tasks(&mut self) {
        // Close all SRT Listner sockets
        for listener in &self.srt_listeners {
            listener.close();
        }

        match dl_srt_rust::cleanup_srt() {
            Ok(_) => {
                tracing::info!("SRT cleanup successful");
            }
            Err(err) => {
                tracing::error!("Failed to cleanup SRT: {}", err);
            }
        }

        tokio::select! {
         _ = futures::future::join_all(&mut self.tasks) => {
           tracing::info!("All tasks have completed");
         },
          _ = tokio::time::sleep(Duration::from_secs(5)) => {
            tracing::warn!("Timeout waiting for tasks to complete");
            for task in &self.tasks {
              task.abort();
            }
          }
        }
    }
}

// TODO: Split into separate threads per stream connection
async fn handle_relay(
    buffer_size: i32,
    in_connections: &Arc<RwLock<HashMap<String, Arc<StreamConnection>>>>,
    out_connections: &Arc<RwLock<HashMap<String, Arc<StreamConnection>>>>,
) {
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
