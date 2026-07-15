#include "env.hpp"

struct stage0_symbols {
};

void load_stage0(content_store registry) {
  inscribe_symbol(registry, "atom");
  inscribe_symbol(registry, "structure");
  inscribe_symbol(registry, "structure");
}
