cmake_minimum_required(VERSION 3.5)

add_library(sas-core STATIC
    include/sas/chunk.h
    include/sas/codec.h
    include/sas/filter.h
    include/sas/log.h
    include/sas/transport.h

    src/chunk.c
    src/log.c
    src/transport.c
)
target_include_directories(sas-core PUBLIC include/)
target_link_libraries(sas-core rxc)
