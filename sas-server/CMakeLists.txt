cmake_minimum_required(VERSION 3.5)

add_executable(sas-server
    include/sas/server.h
    include/sas/formats/wav.h

    src/server.h
    src/server.c
    src/session.h
    src/session.c
    src/sink.h
    src/sink.c
    src/timeout.h
    src/timeout.c
    src/main.c

    src/formats/wav.c
)
target_include_directories(sas-server PUBLIC include/)
target_link_libraries(sas-server sas-core)

