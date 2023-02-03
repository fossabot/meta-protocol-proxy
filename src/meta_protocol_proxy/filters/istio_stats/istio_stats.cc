#include "src/meta_protocol_proxy/filters/istio_stats/istio_stats.h"

#include <memory>
#include <string>
#include <vector>

#include "envoy/stats/scope.h"

#include "source/common/stats/symbol_table.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace IstioStats {

IstioStats::IstioStats(Server::Configuration::FactoryContext& context,
                       envoy::config::core::v3::TrafficDirection traffic_direction)
    : scope_(context.scope()), pool_(context.scope().symbolTable()), stat_namespace_(pool_.add(CustomStatNamespace)),
      requests_total_(pool_.add("istio_requests_total")),
      request_duration_milliseconds_(pool_.add("istio_request_duration_milliseconds")),
      request_bytes_(pool_.add("istio_request_bytes")),
      response_bytes_(pool_.add("istio_response_bytes")), empty_(pool_.add("")),
      unknown_(pool_.add("unknown")), source_(pool_.add("source")),
      destination_(pool_.add("destination")), latest_(pool_.add("latest")),
      http_(pool_.add("http")), grpc_(pool_.add("grpc")), tcp_(pool_.add("tcp")),
      mutual_tls_(pool_.add("mutual_tls")), none_(pool_.add("none")),
      reporter_(pool_.add("reporter")), source_workload_(pool_.add("source_workload")),
      source_workload_namespace_(pool_.add("source_workload_namespace")),
      source_principal_(pool_.add("source_principal")), source_app_(pool_.add("source_app")),
      source_version_(pool_.add("source_version")),
      source_canonical_service_(pool_.add("source_canonical_service")),
      source_canonical_revision_(pool_.add("source_canonical_revision")),
      source_cluster_(pool_.add("source_cluster")),
      destination_workload_(pool_.add("destination_workload")),
      destination_workload_namespace_(pool_.add("destination_workload_namespace")),
      destination_principal_(pool_.add("destination_principal")),
      destination_app_(pool_.add("destination_app")),
      destination_version_(pool_.add("destination_version")),
      destination_service_(pool_.add("destination_service")),
      destination_service_name_(pool_.add("destination_service_name")),
      destination_service_namespace_(pool_.add("destination_service_namespace")),
      destination_canonical_service_(pool_.add("destination_canonical_service")),
      destination_canonical_revision_(pool_.add("destination_canonical_revision")),
      destination_cluster_(pool_.add("destination_cluster")),
      request_protocol_(pool_.add("request_protocol")),
      response_flags_(pool_.add("response_flags")),
      connection_security_policy_(pool_.add("connection_security_policy")),
      response_code_(pool_.add("response_code")) {
  traffic_direction_ = traffic_direction;
  local_node_info_ = Wasm::Common::extractEmptyNodeFlatBuffer();
  if (context.localInfo().node().has_metadata()) {
    local_node_info_ =
        Wasm::Common::extractNodeFlatBufferFromStruct(context.localInfo().node().metadata());
  }
}

// Returns a string view stored in a flatbuffers string.
static inline absl::string_view GetFromFbStringView(const flatbuffers::String* str) {
  return str ? absl::string_view(str->c_str(), str->size()) : absl::string_view();
}

void IstioStats::incCounter(const ::Wasm::Common::FlatNode&) {
  Stats::StatNameTagVector tags;
  tags.reserve(25);
  const auto& local_node = *flatbuffers::GetRoot<Wasm::Common::FlatNode>(local_node_info_.data());

  if (traffic_direction_ == envoy::config::core::v3::TrafficDirection::INBOUND) {
    tags.push_back({reporter_, destination_});
  } else {
    tags.push_back({reporter_, source_});
    auto sourceWorkLoad = GetFromFbStringView(local_node.workload_name());
    tags.push_back(
        {source_workload_, !sourceWorkLoad.empty() ? pool_.add(sourceWorkLoad) : unknown_});
    auto sourceNamespace = GetFromFbStringView(local_node.namespace_());
    tags.push_back({source_workload_namespace_,
                    !sourceNamespace.empty() ? pool_.add(sourceNamespace) : unknown_});
  }
  Stats::Utility::counterFromStatNames(scope_, {stat_namespace_, requests_total_}, tags).inc();
}

void IstioStats::recordHistogram(const Stats::ElementVec& names, Stats::Histogram::Unit unit,
                                 uint64_t sample) {
  Stats::Utility::histogramFromElements(scope_, names, unit).recordValue(sample);
}

} // namespace IstioStats
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy