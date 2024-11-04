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

use std::process;
use std::thread;
use std::time::Duration;
use std::sync::{Arc, Mutex};
use std::collections::VecDeque;

use dl_srt_rust::{SrtSocketConnection, startup_srt, cleanup_srt, SrtOptionValue};
use dl_srt_rust::SrtSocketOptions::{SrtOptRCVSYN, SrtOptReuseAddr};

const PUBLISHER_PORT: u16 = 5500;
const SUBSCRIBER_PORT: u16 = 6000;
const BUFFER_SIZE: i32 = 1500; // Standard MTU size

// Structure to hold our connection lists
struct ConnectionLists {
  publishers: VecDeque<SrtSocketConnection>,
  subscribers: VecDeque<SrtSocketConnection>,
}

fn make_listener_socket(port: u16) -> Result<SrtSocketConnection, String> {
  let connection = SrtSocketConnection::new()
    .map_err(|e| format!("Failed to create listener socket: {}", e))?;

  // Set listener options
  connection.set_sock_opt(0, SrtOptRCVSYN, SrtOptionValue::Bool(true))
    .map_err(|e| format!("Failed to set socket options: {}", e))?;
  connection.set_sock_opt(0, SrtOptReuseAddr, SrtOptionValue::Bool(true))
    .map_err(|e| format!("Failed to set socket options: {}", e))?;

  // Bind and listen
  connection.bind(port)
    .map_err(|e| format!("Failed to bind listener socket: {}", e))?;

  connection.listen(2)
    .map_err(|e| format!("Failed to start listening: {}", e))?;

  Ok(connection)
}

fn handle_connections(
  listener: SrtSocketConnection,
  is_publisher: bool,
  connections: Arc<Mutex<ConnectionLists>>,
) {
  loop {
    match listener.accept() {
      Ok(connection) => {
        println!("{} connected.",
                 if is_publisher { "Publisher" } else { "Subscriber" },
        );

        let mut lists = connections.lock().unwrap();
        if is_publisher {
          lists.publishers.push_back(connection);
        } else {
          lists.subscribers.push_back(connection);
        }

        println!("Current connections - Publishers: {}, Subscribers: {}",
                 lists.publishers.len(),
                 lists.subscribers.len()
        );
      }
      Err(e) => {
        // eprintln!("Failed to accept connection: {}", e);
        thread::sleep(Duration::from_secs(1));
      }
    }
  }
}

fn relay_data(connections: Arc<Mutex<ConnectionLists>>) {
  let mut buffer = Vec::new();

  loop {
    // Lock the connections list to check for available publishers and subscribers
    let mut lists = connections.lock().unwrap();

    if lists.publishers.is_empty() || lists.subscribers.is_empty() {
      drop(lists);  // Release the lock before sleeping
      thread::sleep(Duration::from_millis(100));
      continue;
    }

    // Get the first publisher
    if let Some(publisher) = lists.publishers.front() {
      match publisher.recv(BUFFER_SIZE) {
        Ok(data) => {
          // buffer = data;
          // println!("Received {} bytes from publisher", buffer.len());

          // Send to all subscribers
          lists.subscribers.retain(|subscriber| {
            match subscriber.send(&buffer) {
              Ok(_) => {
                // println!("Forwarded data to subscriber");
                true  // Keep this subscriber
              }
              Err(e) => {
                eprintln!("Failed to forward data to subscriber: {}. Removing subscriber.", e);
                false  // Remove this subscriber
              }
            }
          });
        }
        Err(e) => {
          eprintln!("Error receiving data from publisher: {}. Removing publisher.", e);
          lists.publishers.pop_front();
        }
      }
    }

    drop(lists);  // Release the lock
    thread::sleep(Duration::from_millis(10));
  }
}

fn main() {
  println!("Starting SRT Relay");
  println!("Publisher port: {}", PUBLISHER_PORT);
  println!("Subscriber port: {}", SUBSCRIBER_PORT);

  // Initialize SRT
  if let Err(e) = startup_srt() {
    eprintln!("Failed to start SRT: {}", e);
    process::exit(1);
  }

  // Create the listener socketets
  let publisher_listener = match make_listener_socket(PUBLISHER_PORT) {
    Ok(socket) => socket,
    Err(e) => {
      eprintln!("{}", e);
      cleanup_srt().unwrap_or_default();
      process::exit(1);
    }
  };

  let subscriber_listener = match make_listener_socket(SUBSCRIBER_PORT) {
    Ok(socket) => socket,
    Err(e) => {
      eprintln!("{}", e);
      cleanup_srt().unwrap_or_default();
      process::exit(1);
    }
  };


  // Create shared connection lists
  let connections = Arc::new(Mutex::new(ConnectionLists {
    publishers: VecDeque::new(),
    subscribers: VecDeque::new(),
  }));

  // Spawn publisher listener thread
  let publisher_connections = Arc::clone(&connections);
  let publisher_thread = thread::spawn(move || {
    handle_connections(publisher_listener, true, publisher_connections);
  });

  // Spawn subscriber listener thread
  let subscriber_connections = Arc::clone(&connections);
  let subscriber_thread = thread::spawn(move || {
    handle_connections(subscriber_listener, false, subscriber_connections);
  });

  // Start relay thread
  let relay_connections = Arc::clone(&connections);
  let relay_thread = thread::spawn(move || {
    relay_data(relay_connections);
  });

  println!("Server running. Press Ctrl+C to stop...");

  // Wait for threads (they'll run indefinitely unless there's an error)
  publisher_thread.join().unwrap();
  subscriber_thread.join().unwrap();
  relay_thread.join().unwrap();

  // Cleanup
  println!("Shutting down...");
  cleanup_srt().unwrap_or_default();
}