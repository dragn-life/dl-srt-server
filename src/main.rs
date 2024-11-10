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

use dl_srt_server::core::server::RelayServer;
use dl_srt_server::monitoring::init_monitoring;
use tokio;

#[tokio::main]
async fn main() {
  init_monitoring();

  let mut server = RelayServer::new();

  if let Err(e) = server.run().await {
    tracing::error!("Server Error: {}", e);
  }
}
