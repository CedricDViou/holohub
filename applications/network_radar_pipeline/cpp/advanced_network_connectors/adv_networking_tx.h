/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "common.h"
#include "advanced_network/common.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>

using namespace holoscan::advanced_network;

namespace holoscan::ops {

class AdvConnectorOpTx : public Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(AdvConnectorOpTx)

  AdvConnectorOpTx() = default;

  ~AdvConnectorOpTx() {
    // TODO: bytes / packets sent
    HOLOSCAN_LOG_INFO("Finished transmitter!");
    if (packets_buf) { delete packets_buf; }
  }

  void setup(OperatorSpec& spec) override;
  void populate_dummy_headers(UDPIPV4Pkt& pkt);
  void initialize() override;
  void compute(InputContext& op_input,
               OutputContext& op_output,
               ExecutionContext& context) override;

 private:
  static constexpr int num_concurrent  = 4;   // Number of concurrent batches processing
  static constexpr int MAX_ANO_BATCHES = 10;  // Batches from ANO for one app batch
  static constexpr uint16_t queue_id   = 0;

  Status set_cpu_hdr(holoscan::advanced_network::BurstParams *msg, const int pkt_idx);
  void populate_packets(uint8_t **out_ptr,
                        complex_t *rf_data,
                        uint16_t waveform_id,
                        uint16_t channel_idx,
                        uint16_t offset,
                        cudaStream_t stream);

  struct TxMsg {
    holoscan::advanced_network::BurstParams *msg;
    uint16_t waveform_id;
    uint16_t channel_id;
    cudaEvent_t evt;
  };
  std::queue<TxMsg> out_q;

  Parameter<holoscan::advanced_network::NetworkConfig> cfg_;

  // Radar settings
  Parameter<uint16_t> num_pulses_;
  Parameter<uint16_t> num_samples_;
  Parameter<uint16_t> waveform_length_;
  Parameter<uint16_t> num_channels_;

  // Networking settings
  Parameter<bool> split_boundary_;
  Parameter<uint16_t> samples_per_packet_;
  Parameter<uint16_t> udp_src_port_;
  Parameter<uint16_t> udp_dst_port_;
  Parameter<std::string> ip_src_addr_;
  Parameter<std::string> ip_dst_addr_;
  Parameter<std::string> eth_dst_addr_;
  Parameter<std::string> interface_name_;  // Port name from advanced_network config to poll on
  int port_id_;                            // Port ID to poll on
  uint16_t payload_size_;
  uint32_t batch_size_;
  int hds_;
  bool gpu_direct_;
  std::string mgr_;

  char eth_dst_[6];
  uint32_t ip_src_;
  uint32_t ip_dst_;
  UDPIPV4Pkt pkt;
  void* pkt_header_;

  // Concurrent batch structures
  std::array<cudaStream_t, num_concurrent> streams_;
  std::array<cudaEvent_t, num_concurrent> events_;
  int cur_idx = 0;

  // Memory buffers
  uint8_t *mem_buf_h_;
  RFPacket *packets_buf;
  std::array<uint8_t **, num_concurrent> gpu_bufs;

  index_t samples_per_pkt;
  index_t pkt_per_pulse;
  index_t num_packets_buf;
  size_t buf_stride;
  size_t buf_size;
};  // AdvConnectorOpTx

}  // namespace holoscan::ops
