// SPDX-License-Identifier: MIT
#include <cstdio>

namespace demo {

int add(int a, int b) {
  return a + b;
}

struct Point {
  int x;
  int y;
};

}  // namespace demo

int main() {
  demo::Point p{1, 2};
  std::printf("%d\n", demo::add(p.x, p.y));
  return 0;
}
