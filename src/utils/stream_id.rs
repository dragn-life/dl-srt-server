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
use dl_srt_rust::{SrtOptionValue, SrtSocketConnection, SrtSocketOptions};
use std::collections::HashMap;
use std::fmt::{Display, Formatter};
use std::sync::RwLock;
use std::time::{Duration, Instant};

#[derive(Eq, Hash, PartialEq, Debug, Clone)]
pub struct StreamId(String);

impl Display for StreamId {
  fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
    write!(f, "StreamID: {}", self.0)
  }
}

struct CacheEntry {
  valid: bool,
  timestamp: Instant,
}

pub struct StreamIdValidator {
  cache: RwLock<HashMap<String, CacheEntry>>,
  // TODO: Implement API / File validation (i.e. stream validation type, maybe use settings)
  // api_url: String,
  cache_duration: Duration,
}

impl StreamIdValidator {
  pub fn new() -> StreamIdValidator {
    StreamIdValidator {
      cache: RwLock::new(HashMap::new()),
      cache_duration: Duration::from_secs(60),
    }
  }

  fn extract_srt_stream_id(srt_socket: &SrtSocketConnection) -> Result<String, RelayError> {
    // Extract Stream ID from SRT connection
    match srt_socket.get_sock_flag(SrtSocketOptions::SrtOptStreamID) {
      Ok(SrtOptionValue::String(stream_id)) => Ok(stream_id),
      Ok(other) => {
        tracing::error!("Failed to extract stream id: Unexpected value: {:?}", other);
        Err(RelayError::GeneralError)
      }
      Err(e) => {
        tracing::error!("Failed to get stream id: {}", e);
        Err(RelayError::SrtError(e))
      }
    }
  }

  pub async fn validate_srt_stream_id(
    &self,
    srt_socket: &SrtSocketConnection,
  ) -> Result<StreamId, RelayError> {
    let stream_id = match Self::extract_srt_stream_id(srt_socket) {
      Ok(stream_id) => stream_id,
      Err(e) => return Err(e),
    };

    // Check if empty
    if stream_id.is_empty() {
      return Err(RelayError::InvalidStreamId(String::from(
        "Stream ID Missing",
      )));
    }

    // TODO: Caching
    // Check Cache
    // if let Some(is_valid) = self.check_cache(&stream_id).await {
    //   return if is_valid {
    //     Ok(StreamId(stream_id))
    //   } else {
    //     Err(RelayError::InvalidStreamId(stream_id))
    //   };
    // }

    // TODO: Validate via API / File

    // Update Cache
    // self.update_cache(&stream_id, true).await;

    Ok(StreamId(stream_id))
  }

  // async fn check_cache(&self, stream_id: &str) -> Option<bool> {
  //   let cache = self.cache.read().await;
  //   if let Some(entry) = cache.get(stream_id) {
  //     if entry.timestamp.elapsed() < self.cache_duration {
  //       return Some(entry.valid);
  //     }
  //   }
  //   None
  // }
  //
  // async fn update_cache(&self, stream_id: &str, is_valid: bool) {
  //   let mut cache = self.cache.write().await;
  //   cache.insert(
  //     stream_id.to_string(),
  //     CacheEntry {
  //       valid: is_valid,
  //       timestamp: Instant::now(),
  //     },
  //   );
  // }
}
