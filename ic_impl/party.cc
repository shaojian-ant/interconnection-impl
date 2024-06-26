// Copyright 2023 Ant Group Co., Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ic_impl/party.h"

#include "spdlog/spdlog.h"

#include "ic_impl/context.h"
#include "ic_impl/factory.h"

namespace ic_impl {

Party::Party(std::shared_ptr<IcContext> ctx) : ctx_(std::move(ctx)) {}

Party::~Party() = default;

void Party::Run() {
  int32_t self_rank = ctx_->lctx->Rank();
  if (ctx_->version == 1) {  // send request from rank 0 to rank 1
    YACL_THROW("handshake version 1 has been discarded");
  } else {  // send requests from rank 1 ~ n-1 to rank 0
    auto factory = CreateHandlerFactory();
    YACL_ENFORCE(factory);
    auto handler_v2 = factory->CreateAlgoV2Handler(ctx_);

    int32_t recv_rank = 0;
    if (self_rank == recv_rank) {
      handler_v2->PassiveRun();
    } else {
      handler_v2->ActiveRun(recv_rank);
    }
  }
}

std::unique_ptr<AlgoHandlerFactory> Party::CreateHandlerFactory() {
  if (ctx_->algo == org::interconnection::v2::ALGO_TYPE_ECDH_PSI) {
    YACL_ENFORCE(!ctx_->protocol_families.empty());
    YACL_ENFORCE(ctx_->protocol_families.at(0) ==
                 org::interconnection::v2::PROTOCOL_FAMILY_ECC);
    SPDLOG_INFO("run ECDH-PSI");
    return std::make_unique<PsiV2HandlerFactory>();
  } else if (ctx_->algo == org::interconnection::v2::ALGO_TYPE_SS_LR) {
    YACL_ENFORCE(!ctx_->protocol_families.empty());
    YACL_ENFORCE(ctx_->protocol_families.at(0) ==
                 org::interconnection::v2::PROTOCOL_FAMILY_SS);
    SPDLOG_INFO("run SS-LR");
    return std::make_unique<LrHandlerFactory>();
  }

  SPDLOG_ERROR("Create algo handler failed");
  return nullptr;
}

}  // namespace ic_impl
