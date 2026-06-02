add_library(renderer STATIC renderer.cpp)
target_include_directories(renderer PUBLIC .)
target_link_libraries(renderer
    PUBLIC  egl_context eye_swapchain vr_math
    PRIVATE GLESv3 openxr_loader log)
target_compile_options(renderer PRIVATE -Wall -Wextra)
