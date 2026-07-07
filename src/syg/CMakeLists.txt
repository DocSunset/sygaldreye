add_executable(syg main.cpp)
target_link_libraries(syg PRIVATE
  nlohmann_json::nlohmann_json
  PkgConfig::SODIUM
  Threads::Threads
)
