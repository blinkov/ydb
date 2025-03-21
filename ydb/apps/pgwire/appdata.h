#pragma once
#include <ydb/library/actors/core/actorsystem.h>
#include <ydb/library/actors/core/actor.h>
#include <library/cpp/monlib/metrics/metric_registry.h>

namespace NPGW {

struct TAppData {
    std::shared_ptr<NMonitoring::TMetricRegistry> MetricRegistry;
};

inline TAppData* AppData() {
    return NActors::TActivationContext::ActorSystem()->template AppData<TAppData>();
}

}
