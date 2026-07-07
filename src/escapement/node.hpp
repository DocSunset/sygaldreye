#include <cstdint>
#include <utility>
#include <type_traits>
#include "cell.hpp"

namespace syg::esc {

struct node {
  cell in_count;
  cell out_count;
  void (*fn)(cell** in, cell* out);   // inputs by pointer (any cells); outputs written from out
};

template <auto Fn, class R, class... Args>
node describe_(R (*)(Args...)) {
  cell outs = [] {
    if constexpr (std::is_void_v<R>) return cell(0);
    else                             return cell(sizeof(R) / sizeof(cell));
  }();
  return {
    cell(sizeof...(Args)),
    outs,
    [](cell** in, [[maybe_unused]] cell* out) {
      [=]<std::size_t... I>(std::index_sequence<I...>) {
        if constexpr (std::is_void_v<R>)
          Fn(*reinterpret_cast<Args*>(in[I])...);
        else
          *reinterpret_cast<R*>(out) = Fn(*reinterpret_cast<Args*>(in[I])...);
      }(std::index_sequence_for<Args...>{});
    }
  };
}

template <auto Fn> node describe() { return describe_<Fn>(Fn); }

}
