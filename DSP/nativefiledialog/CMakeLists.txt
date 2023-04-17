target_sources(${PROJECT_NAME} PRIVATE
    src/include/nfd.h
    src/common.h
    src/nfd_common.h
    src/nfd_common.c
    src/simple_exec.h
)

if(UNIX)
    target_sources(${PROJECT_NAME} PRIVATE
        src/nfd_gtk.c
        src/nfd_zenity.c
    )

    find_package(GTK2 2.6 REQUIRED gtk)

    if(${GTK2_FOUND})
        target_include_directories(${PROJECT_NAME} PRIVATE ${GTK2_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${GTK2_LIBRARIES})
    else()
        message(ERROR "Could not find GTK")
    endif()
elseif(WIN32)
    target_sources(${PROJECT_NAME} PRIVATE
        src/nfd_win.cpp
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE src/include)