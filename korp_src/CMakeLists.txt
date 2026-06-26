if(WIN32)
    message(STATUS "Use minimum version 4.2.")
    cmake_minimum_required(VERSION 4.2)
    elseif(UNIX)
        message(STATUS "Use minimum version 3.10.")
        cmake_minimum_required(VERSION 3.10)
endif()

project(korpvm)

set(CMAKE_CXX_STANDARD 98)

if (MINGW OR MSYS)
    message(STATUS "MSYS detected.")

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-nxcompat")
endif()

if(UNIX AND CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "UNIX detected.")

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -g -fno-stack-protector")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -g -fno-stack-protector")

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,execstack -no-pie")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,execstack -no-pie")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-z,execstack -no-pie")
endif()

include_directories(class)
include_directories(ia32_jit)
include_directories(include)
include_directories(natives)
include_directories(utils)

add_library(class STATIC
        class/class_initialize.cpp
        class/class_loader.cpp
        class/class_loader.h
        class/class_parse.cpp
        class/class_prepare.cpp
        class/class_table.cpp
        class/class_table.h
        class/class_utils.cpp
        class/class_utils.h
        class/class_verify.cpp
        class/method.cpp
        class/method.h
        class/string_pool.cpp
        class/string_pool.h
        class/vm_class.h
        class/vm_interface.cpp
        class/vm_interface.h
        class/vm_object.h
)

add_library(gc STATIC
        gc/object_create.cpp
)

add_library(ia32_jit STATIC
        ia32_jit/code_gen.cpp
        ia32_jit/compile.cpp
        ia32_jit/compile.h
        ia32_jit/jit_runtime.cpp
        ia32_jit/jit_runtime.h
        ia32_jit/jit_support.cpp
        ia32_jit/prepass.cpp
        ia32_jit/x86.cpp
        ia32_jit/x86.h
)

add_library(natives STATIC
        natives/java_lang.cpp
        natives/java_lang.h
        natives/natives.cpp
        natives/natives.h
)

add_library(utils STATIC
        utils/bytecode.cpp
        utils/exception.cpp
        utils/exception.h
        utils/runtime.cpp
        utils/runtime.h
        utils/utils.cpp
        utils/utils.h)

add_executable(korpvm
        korp/korp.cpp
)

target_link_libraries(korpvm PRIVATE class gc ia32_jit natives utils)
target_link_libraries(utils PRIVATE ia32_jit)
target_link_libraries(ia32_jit PRIVATE class natives)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.13.0")
    if (NOT MINGW AND NOT MSYS)
        message(STATUS "Use target_link_options.")

        target_link_options(korpvm PRIVATE
                "SHELL:$<$<CXX_COMPILER_ID:MSVC>:/NXCOMPAT:NO>"
                "SHELL:$<$<CXX_COMPILER_ID:GNU,Clang>:-z,execstack>"
        )
    endif()
else()
    if(WIN32)
        message(STATUS "Win32 detected.")

        set_target_properties(korpvm PROPERTIES LINK_FLAGS "/NXCOMPAT:NO")
    endif()
endif()
