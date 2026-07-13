// syg::variant::make — a sum type minted at runtime as a syg_type. Cases ride in
// template_args (type args); RAII dispatches on the tag. assert-based; exit 0 passes.
#include "variant.hpp"
#include <cassert>
#include <cstdint>
#include <string_view>
using namespace syg;

int main() {
  const syg_type_t* cases[] = { stage0::generate_value<std::int32_t>(),
                                stage0::generate_value<double>() };
  const syg_type_t* t = variant::make("num", cases);

  // layout: tag at 0, data aligned to the widest case (double => 8), total 16 bytes.
  assert(t->member_count == 2 && t->template_arg_count == 2);
  assert(t->size == 16 && t->align == 8);
  assert(std::string_view{t->members[0].name} == "tag" && t->members[0].offset == 0);
  assert(std::string_view{t->members[1].name} == "data" && t->members[1].offset == 8);
  // cases are TYPE args: addr == nullptr, type == the case.
  assert(t->template_args[0].addr == nullptr && t->template_args[1].type == cases[1]);

  alignas(16) unsigned char buf[16];
  syg_value_t v{ t, buf };
  t->place(v, nullptr);                                 // defaults to case 0 (int32)
  assert(variant::key(v) == 0 && variant::blob(v).type == cases[0]);
  *static_cast<std::int32_t*>(variant::blob(v).data) = 42;
  assert(*static_cast<std::int32_t*>(variant::blob(v).data) == 42);

  variant::set(v, 1);                                   // switch the live case to double
  assert(variant::key(v) == 1 && variant::blob(v).type == cases[1]);
  *static_cast<double*>(variant::blob(v).data) = 3.5;
  assert(*static_cast<double*>(variant::blob(v).data) == 3.5);

  // move: migrate the live case into a fresh instance (move placement-constructs).
  alignas(16) unsigned char buf2[16];
  syg_value_t v2{ t, buf2 };
  t->move(v2, buf);
  assert(variant::key(v2) == 1 && *static_cast<double*>(variant::blob(v2).data) == 3.5);
  t->erase(v2); t->erase(v);

  // identity: the case-set enters params_hash — a different set is a different type,
  // the same set reproduces the id (deterministic; the pointer differs, the id doesn't).
  const syg_type_t* one[] = { stage0::generate_value<std::int32_t>() };
  assert(variant::make("num", one)->id != t->id);
  assert(variant::make("num", cases)->id == t->id);
  return 0;
}
