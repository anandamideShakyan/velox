/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "TraceConfig.h"
#include "velox/common/file/File.h"
#include "velox/common/file/FileSystems.h"
#include "velox/serializers/PrestoSerializer.h"
#include "velox/vector/VectorStream.h"

namespace facebook::velox::exec {
class Operator;
}

namespace facebook::velox::exec::trace {

/// Used to serialize and write the input vectors from a given operator into a
/// file.
class OperatorTraceWriter {
 public:
  /// 'traceOp' is the operator to trace. 'traceDir' specifies the trace
  /// directory for the operator.
  explicit OperatorTraceWriter(
      Operator* traceOp,
      std::string traceDir,
      memory::MemoryPool* pool,
      UpdateAndCheckTraceLimitCB updateAndCheckTraceLimitCB);

  /// Serializes rows and writes out each batch.
  void write(const RowVectorPtr& rows);

  /// Closes the data file and writes out the data summary.
  void finish();

 private:
  // Flushes the trace data summaries to the disk.
  //
  // TODO: add more summaries such as number of rows etc.
  void writeSummary() const;

  Operator* const traceOp_;
  const std::string traceDir_;
  // TODO: make 'useLosslessTimestamp' configuerable.
  const serializer::presto::PrestoVectorSerde::PrestoOptions options_ = {
      true,
      common::CompressionKind::CompressionKind_ZSTD,
      /*nullsFirst=*/true};
  const std::shared_ptr<filesystems::FileSystem> fs_;
  memory::MemoryPool* const pool_;
  const UpdateAndCheckTraceLimitCB updateAndCheckTraceLimitCB_;

  std::unique_ptr<WriteFile> traceFile_;
  TypePtr dataType_;
  std::unique_ptr<VectorStreamGroup> batch_;
  bool limitExceeded_{false};
  bool finished_{false};
};

} // namespace facebook::velox::exec::trace