find_package(Boost REQUIRED)
add_executable(syg_generator generate.cpp)
target_include_directories(syg_generator PRIVATE ../authoring ../nodes)
target_link_libraries(syg_generator PRIVATE Boost::headers nlohmann_json::nlohmann_json)

set(GEN_DIR ${CMAKE_BINARY_DIR}/generated)
add_custom_command(
  OUTPUT ${GEN_DIR}/codecs.hpp ${GEN_DIR}/bindings.py ${GEN_DIR}/registration.cpp ${GEN_DIR}/registration.json
         ${GEN_DIR}/widget_a.descriptor.json ${GEN_DIR}/widget_b.descriptor.json
  COMMAND syg_generator ${GEN_DIR}
  DEPENDS syg_generator ${CMAKE_CURRENT_SOURCE_DIR}/../nodes/widgets/widgets.hpp
  COMMENT "generating descriptors, codecs, bindings (ABI-1)")
add_custom_target(generated ALL DEPENDS ${GEN_DIR}/codecs.hpp)
