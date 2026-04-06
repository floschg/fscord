cmake_minimum_required(VERSION 3.20)


# project settings
project(fscord)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -pedantic-errors)
    #add_compile_options(-Wall -Wextra -Wpedantic -pedantic-errors)
endif()

set(FSCORD_ROOT_DIR     "${CMAKE_CURRENT_LIST_DIR}/..")
set(FSCORD_SRC_DIR      "${FSCORD_ROOT_DIR}/src")
set(FSCORD_EXTERNAL_DIR "${FSCORD_ROOT_DIR}/external")

include_directories("${FSCORD_ROOT_DIR}/external")



# external: Mongoose
add_library(mongoose STATIC
    ${FSCORD_EXTERNAL_DIR}/mongoose/mongoose.c
)
set_target_properties(mongoose PROPERTIES
    C_EXTENSIONS ON
)


# external: stb
add_library(stb STATIC
    ${FSCORD_EXTERNAL_DIR}/stb/stb_truetype.c
    ${FSCORD_EXTERNAL_DIR}/stb/stb_image.c
    ${FSCORD_EXTERNAL_DIR}/stb/stb_image_write.c
)



# fscord client
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../data/fscord)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../data/fscord)
add_executable(fscord)
target_sources(fscord PUBLIC
    ${FSCORD_SRC_DIR}/client/main.c
    ${FSCORD_SRC_DIR}/basic/arena.c
    ${FSCORD_SRC_DIR}/basic/math.c
    ${FSCORD_SRC_DIR}/basic/string32.c
    ${FSCORD_SRC_DIR}/basic/ring_alloc.c
    ${FSCORD_SRC_DIR}/client/draw.c
    ${FSCORD_SRC_DIR}/client/sound.c
    ${FSCORD_SRC_DIR}/client/font.c
    ${FSCORD_SRC_DIR}/client/login.c
    ${FSCORD_SRC_DIR}/client/session.c
    ${FSCORD_SRC_DIR}/client/string32_handles.c
    ${FSCORD_SRC_DIR}/client/server_connection.c
    ${FSCORD_SRC_DIR}/crypto/aes_gcm.c
    ${FSCORD_SRC_DIR}/crypto/rsa.c
    ${FSCORD_SRC_DIR}/os/sdl/sdl_window.c
    ${FSCORD_SRC_DIR}/os/linux/linux_library.c
    ${FSCORD_SRC_DIR}/os/linux/linux_memory.c
    ${FSCORD_SRC_DIR}/os/linux/linux_net_secure_stream.c
    ${FSCORD_SRC_DIR}/os/linux/linux_time.c
    ${FSCORD_SRC_DIR}/os/linux/linux_file.c
)
target_include_directories(fscord PRIVATE ${FSCORD_SRC_DIR} ${FSCORD_EXTERNAL_DIR/cglm})
target_link_libraries(fscord SDL3 GL asound ssl crypto m stb)


# fscord server
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../data/fscord_server)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../data/fscord_server)
add_executable(fscord_server)
target_sources(fscord_server PUBLIC
    ${FSCORD_SRC_DIR}/basic/arena.c
    ${FSCORD_SRC_DIR}/basic/string32.c
    ${FSCORD_SRC_DIR}/os/linux/linux_memory.c
    ${FSCORD_SRC_DIR}/os/linux/linux_net_secure_stream.c
    ${FSCORD_SRC_DIR}/os/linux/linux_time.c
    ${FSCORD_SRC_DIR}/os/linux/linux_file.c
    ${FSCORD_SRC_DIR}/crypto/aes_gcm.c
    ${FSCORD_SRC_DIR}/crypto/rsa.c
    ${FSCORD_SRC_DIR}/server/main.c
    ${FSCORD_SRC_DIR}/server/client_manager.c
    ${FSCORD_SRC_DIR}/server/c2s_handler.c
    ${FSCORD_SRC_DIR}/server/s2c_sender.c
)
target_include_directories(fscord_server PUBLIC ${FSCORD_SRC_DIR})
target_link_libraries(fscord_server ssl crypto mongoose)

