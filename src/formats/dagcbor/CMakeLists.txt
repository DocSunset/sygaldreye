add_library(syg_dagcbor dagcbor.cpp)
target_include_directories(syg_dagcbor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(syg_dagcbor PUBLIC nlohmann_json::nlohmann_json)
