// Temporary link check — verifies OpenXR loader compiles and links. Remove after item 07.
#include <openxr/openxr.h>

int main() {
    uint32_t count = 0;
    xrEnumerateApiLayerProperties(0, &count, nullptr);
    return 0;
}
