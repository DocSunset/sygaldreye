// clause: scaffolding (dissolves: PKG-8.1) — the dac NODE is a passive
// boundary by design (the executor owns device machinery — ADR-015/PKG-1);
// what still pends is the REAL device output the audio package's executor
// drives. That machinery arrives with environment observation / device
// splice-in (PKG-8.1); until then the harness reads this sink's buffer.
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
    dac_process, nullptr, syg::generated::dac_in_ports(), syg::generated::dac_out_ports(),
    false, false, nullptr, nullptr, nullptr, nullptr, false, nullptr,
    /*resource_holder=*/true};

}  // namespace syg::nodes
