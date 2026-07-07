// clause: floor — kernel native over ugens (generated registration references
// this TU's symbol; omitting the object is a loud link error — SZ-2)
#include "crown.hpp"

#include "native_ports.hpp"

#include <cstring>

#include "ugens/ugens.hpp"

namespace syg::nodes {
namespace {
void vca_no_num(void*, const char*, double) {}
void vca_no_text(void*, const char*, const char*) {}
}  // namespace

extern const syg::crown::native_type vca_native;
const syg::crown::native_type vca_native{
    "vca", [] { return static_cast<void*>(nullptr); }, [](void*) {},
    vca_no_num, vca_no_text, vca_process, nullptr,
    syg::generated::vca_in_ports(), syg::generated::vca_out_ports()};

}  // namespace syg::nodes
