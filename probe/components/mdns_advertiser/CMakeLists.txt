add_library(mdns_advertiser STATIC mdns_advertiser.cpp mdns_advertiser_impl.c)
target_include_directories(mdns_advertiser PUBLIC .)
target_include_directories(mdns_advertiser PRIVATE ${CMAKE_SOURCE_DIR}/vendor)
target_compile_options(mdns_advertiser PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra>
    $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra -Wno-missing-field-initializers>
)
