add_executable(syg main.cpp abi_audits.cpp compile_session.cpp naming_session.cpp peer_session.cpp realized_compile.cpp stage0_audits.cpp store_session.cpp
  ${CMAKE_BINARY_DIR}/generated/registration.cpp
  ${CMAKE_BINARY_DIR}/generated/leaf_natives.cpp
  ../abi/audit_alloc.cpp ../nodes/testnodes/testnodes.cpp)
target_link_libraries(syg PRIVATE syg_address syg_chunk syg_cid syg_dagcbor
  syg_crown syg_executor syg_hello_natives syg_movements syg_naming syg_organs syg_stage0 syg_store syg_abi)
set_target_properties(syg PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_dependencies(syg generated)
target_include_directories(syg PRIVATE ${CMAKE_BINARY_DIR}/generated ${CMAKE_SOURCE_DIR}/src/authoring ${CMAKE_SOURCE_DIR}/src/nodes)
