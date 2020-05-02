cmake_minimum_required(VERSION 3.5)

add_executable(sas-client
    include/sas/client.h
    include/sas/drivers/ignore.h

    src/client.h
    src/client.c
    src/source.h
    src/source.c
    src/main.c

    src/drivers/ignore.c
)
target_include_directories(sas-client PUBLIC include/)
target_link_libraries(sas-client sas-core)
