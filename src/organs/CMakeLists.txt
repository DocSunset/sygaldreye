add_library(syg_organs parser/parser.cpp parser/parser_native.cpp slot/slot.cpp slot/slot_native.cpp resolver/naive_resolver_native.cpp registry_face/registry_face_native.cpp supervisor/supervisor_native.cpp registry_face/registry_face.cpp resolver/naive_resolver.cpp)
target_include_directories(syg_organs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(syg_organs PUBLIC nlohmann_json::nlohmann_json syg_cid syg_crown)
