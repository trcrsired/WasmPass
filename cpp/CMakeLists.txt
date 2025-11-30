cmake_minimum_required(VERSION 3.20)
project(genpass_wasm LANGUAGES CXX)

# Require modern C++ standard
set(CMAKE_CXX_STANDARD 26)

# Define the executable target (output will be genpass.wasm)
add_executable(genpass.wasm genpass.cpp)

# Optionally allow user to set FAST_IO_DIR
set(FAST_IO_DIR "" CACHE PATH "Path to fast_io installation")

# Disable exceptions and rtti for this target
target_compile_options(genpass.wasm PRIVATE
    -fno-exceptions
    -fno-rtti
)

# If FAST_IO_DIR is provided, add its include path
if(FAST_IO_DIR)
    target_include_directories(genpass.wasm PRIVATE "${FAST_IO_DIR}/include")
endif()

# Linker options:
# -nostartfiles : omit default startup files so we control initialization manually
# --no-entry    : do not generate a default `_start` entrypoint; this makes the module behave like a library
# --export      : explicitly mark functions as exports so they are accessible from JavaScript
#   __wasm_call_ctors : runs all global/static constructors (initializes global state before use)
#   __wasm_call_dtors : runs all global/static destructors and atexit handlers (cleans up resources)
#   others : custom functions to expose the HTML buffer
target_link_options(genpass.wasm PRIVATE
    -nostartfiles
    -Wl,--no-entry
    -Wl,--export=__wasm_call_ctors
    -Wl,--export=__wasm_call_dtors
    -Wl,--export=generate_data
    -Wl,--export=get_last_generated_category
    -Wl,--export=get_html_ptr
    -Wl,--export=get_html_len
    -Wl,--export=get_saved_ptr
    -Wl,--export=get_saved_len
    -Wl,--export=generated_elapsed_time_ptr
    -Wl,--export=generated_elapsed_time_len
    -Wl,--export=generated_last_timestamp_ptr
    -Wl,--export=generated_last_timestamp_len
)

# Install the wasm binary directly into the install prefix
install(TARGETS genpass.wasm
    RUNTIME DESTINATION .
)
