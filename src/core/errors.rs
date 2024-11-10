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
use thiserror::Error;

// TODO: WIP: Add SRT error, fine tune specific errors
#[derive(Error, Debug)]
pub enum RelayError {
  // TODO: Add SRT error
  // #[error("SRT error: {0}")]
  // SrtError(#[from] dl_srt_rust::Error),
  #[error("Broadcast channel error")]
  BroadcastError,
  #[error("IO error: {0}")]
  IoError(#[from] std::io::Error),
  #[error("Failed to bind on port: {0}")]
  SocketBindError(u16),
  #[error("Failed to accept connection")]
  SocketAcceptError,
  #[error("General Error")]
  GeneralError,
}

