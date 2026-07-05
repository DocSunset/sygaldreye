add_library(syg_executor exec_plan.cpp lift.cpp regions.cpp)
target_include_directories(syg_executor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(syg_executor PUBLIC syg_abi syg_crown syg_naming syg_organs)
