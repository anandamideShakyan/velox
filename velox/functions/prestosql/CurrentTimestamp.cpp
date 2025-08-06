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

#include <exec/SimpleAggregateAdapter.h>

#include "type/tz/TimeZoneMap.h"
#include "types/TimestampWithTimeZoneType.h"
#include "velox/expression/EvalCtx.h"
#include "velox/expression/VectorFunction.h"

namespace facebook::velox::functions {
namespace {
class CurrentTimestampFunction : public exec::VectorFunction {
public:
  bool isDeterministic() const {
    return false;
  }

  void apply(
      const SelectivityVector& rows,
      std::vector<VectorPtr>& /*args*/,
      const TypePtr& /*outputType*/,
      exec::EvalCtx& context,
      VectorPtr& result) const override {

    const auto& config = context.execCtx()->queryCtx()->queryConfig();
    auto timeZoneKey = tz::getTimeZoneID(config.sessionTimezone(), /*failOnError=*/true);

    Timestamp now = Timestamp::now();
    int64_t packed = pack(now, timeZoneKey);

    exec::out_type<TimestampWithTimezone> value(packed);

    // Create a ConstantVector that repeats the value across all rows
    result = std::make_shared<ConstantVector<int64_t>>(
        context.pool(),
        rows.size(),               // number of rows
        false,                     // isNull
        TIMESTAMP_WITH_TIME_ZONE(), // Velox type
        std::move(packed));                    // the actual value
  }

  static std::vector<std::shared_ptr<exec::FunctionSignature>> signatures() {
    return {
      exec::FunctionSignatureBuilder()
          .returnType("timestamp with time zone")
          .build()};
  }
};
} // namespace

// Declare and register
VELOX_DECLARE_VECTOR_FUNCTION_WITH_METADATA(
    udf_current_timestamp,
    CurrentTimestampFunction::signatures(),
    exec::VectorFunctionMetadataBuilder()
        .deterministic(false)
        .defaultNullBehavior(false)
        .build(),
    std::make_unique<CurrentTimestampFunction>());

void registerCurrentTimestampFunction(const std::string& name) {
  VELOX_REGISTER_VECTOR_FUNCTION(udf_current_timestamp, name);
}
} // namespace facebook::velox::functions
