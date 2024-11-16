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
use tokio::sync::OnceCell;

#[derive(Debug, Clone)]
pub struct Settings {
  pub input_stream_port: u16,
  pub output_stream_port: u16,
  pub buffer_size: usize,
  pub max_connections: usize,
  pub latency_ms: u32,
}

// Static global settings
static SETTINGS: OnceCell<Settings> = OnceCell::const_new();

impl Settings {
  pub fn init(
    input_stream_port: u16,
    output_stream_port: u16,
    buffer_size: usize,
    max_connections: usize,
    latency_ms: u32,
  ) {
    let settings = Settings {
      input_stream_port,
      output_stream_port,
      buffer_size,
      max_connections,
      latency_ms,
    };
    SETTINGS
      .set(settings)
      .expect("Settings already initialized");
  }

  pub fn get() -> &'static Settings {
    SETTINGS.get().expect("Settings not initialized")
  }
}

// TODO: Implement a way to load settings from a file or via Command line Arguments
