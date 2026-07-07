add_library(mongoose STATIC mongoose.c)
target_include_directories(mongoose PUBLIC .)
target_compile_definitions(mongoose PUBLIC MG_ENABLE_OPENSSL=0 MG_ENABLE_MBEDTLS=0)
# suppress warnings we don't own
target_compile_options(mongoose PRIVATE -Wno-everything)
# Model uploads (whisper weights ~150 MB) arrive as one POST body.
target_compile_definitions(mongoose PUBLIC MG_MAX_RECV_SIZE=268435456)
