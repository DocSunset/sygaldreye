// clause: scaffolding — a no-op sink standing where the audio package's real
// dac (machinery) arrives at rung 8; the harness reads its input buffer
#include "crown.hpp"

#include "native_ports.hpp"

namespace syg::nodes {
namespace {

void dac_process(void*, const float* const*, float* const*, int) noexcept {}

}  // namespace

extern const syg::crown::native_type dac_native;
const syg::crown::native_type dac_native{
    "dac", [] { return static_cast<void*>(nullptr); }, [](void*) {},
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    dac_process, nullptr, syg::generated::dac_in_ports, syg::generated::dac_out_ports};

}  // namespace syg::nodes
