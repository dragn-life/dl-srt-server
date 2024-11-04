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
use tokio::sync::broadcast;
use crate::config::Settings;
use crate::core::errors::RelayError;
use crate::net::connection::ConnectionType;
use crate::net::listener;
use crate::net::stream::StreamData;

pub struct RelayServer {
  settings: Settings,
  running: Arc<AtomicBool>,
  tx: broadcast::Sender<StreamData>,
}

impl RelayServer {
  pub fn new() -> Self {
    let settings = Settings::default();
    let running = Arc::new(AtomicBool::new(true));
    let (tx, _) = broadcast::channel(settings.max_connections);
    RelayServer {
      settings,
      running,
      tx,
    }
  }

  pub async fn run(&self) -> Result<(), RelayError> {
    println!("Starting SRT relay server...");
    println!("Incoming Streams Listening on: {}", self.settings.input_stream_port);
    println!("Outgoing Streams Listening on: {}", self.settings.output_stream_port);

    dl_srt_rust::startup_srt()?;

    let incoming_stream_listener = listener::SrtListener::new(self.settings.input_stream_port, ConnectionType::InputStream, self.settings.clone())?;
    let outgoing_stream_listener = listener::SrtListener::new(self.settings.output_stream_port, ConnectionType::OutputStream, self.settings.clone())?;

    // Spawn listener tasks
    let incoming_stream_handle = incoming_stream_listener.start_listening(self.running.clone(), self.tx.clone());
    let outgoing_stream_handle = outgoing_stream_listener.start_listening(self.running.clone(), self.tx.clone());

    tokio::select! {
      _ = tokio::signal::ctrl_c() => {
        println!("Shutting down server...");
        self.running.store(false, Ordering::SeqCst);
      },
      _ = incoming_stream_handle => {
        println!("Incoming stream listener has stopped");
      },
      _ = outgoing_stream_handle => {
        println!("Outgoing stream listener has stopped");
      }
    }

    Ok(())
  }

  pub fn shutdown(&self) {
    self.running.store(false, Ordering::SeqCst);
    dl_srt_rust::cleanup_srt().unwrap_or_default();
  }
}