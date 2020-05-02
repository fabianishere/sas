cmake_minimum_required(VERSION 3.5)

add_library(rxc SHARED
    include/rxc/rxc.h
    include/rxc/pipeline.h
    include/rxc/logic.h
    include/rxc/scheduler.h
    include/rxc/ops/core.h

    src/pipeline.c
    src/logic.c
    src/scheduler.c
    src/schedulers/trampoline.c

    src/ops/canceled.c
    src/ops/count.c
    src/ops/empty.c
    src/ops/foreach.c
    src/ops/ignore.c
    src/ops/map.c
    src/ops/wrapper.c
)
target_include_directories(rxc PUBLIC include/)

if(MSVC)
    target_compile_options(rxc PRIVATE "/W4")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(rxc PRIVATE -Wall -Wno-long-long -pedantic)
endif()
