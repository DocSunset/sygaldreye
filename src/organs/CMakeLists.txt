add_library(syg_organs parser/parser.cpp resolver/naive_resolver.cpp)
target_include_directories(syg_organs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(syg_organs PUBLIC nlohmann_json::nlohmann_json syg_cid)
