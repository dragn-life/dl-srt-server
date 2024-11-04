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

#[derive(Debug, Clone)]
pub struct Settings {
  pub input_stream_port: u16,
  pub output_stream_port: u16,
  pub buffer_size: usize,
  pub max_connections: usize,
  pub latency_ms: u32,
}

impl Default for Settings {
  fn default() -> Self {
    Self {
      input_stream_port: 5500,
      output_stream_port: 6000,
      buffer_size: 1456, // Default SRT Buffer Size
      max_connections: 100,
      latency_ms: 20,
    }
  }
}

// TODO: Implement a way to load settings from a file or via Command line Arguments
