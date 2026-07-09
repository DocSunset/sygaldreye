# The escapement is header-only; each test compiles a header's worked example and
# runs it (assert-based — exit 0 is a pass). They need C++26; the component layer
# also needs GCC 16 static reflection (-freflection), whose splices trip -Wpedantic,
# so it is dropped here. Tests are held to a looser bar than the rest of the tree.

foreach(t node plan crown)
  add_executable(esc_${t}_test ${t}.test.cpp)
  set_target_properties(esc_${t}_test PROPERTIES CXX_STANDARD 26)
  target_compile_options(esc_${t}_test PRIVATE -std=c++26 -Wno-pedantic)
  add_test(NAME esc_${t} COMMAND esc_${t}_test)
endforeach()

add_executable(esc_component_test component.test.cpp)
set_target_properties(esc_component_test PROPERTIES CXX_STANDARD 26)
target_compile_options(esc_component_test PRIVATE -std=c++26 -freflection -Wno-pedantic)
add_test(NAME esc_component COMMAND esc_component_test)
