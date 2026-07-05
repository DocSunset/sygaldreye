add_executable(syg main.cpp naming_session.cpp)
target_link_libraries(syg PRIVATE syg_address syg_chunk syg_cid syg_dagcbor syg_movements syg_naming)
set_target_properties(syg PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
