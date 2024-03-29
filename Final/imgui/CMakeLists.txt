set(IMGUI_SOURCES
    imgui.h
    imgui.cpp

    imconfig.h
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_internal.h
    imgui_tables.cpp
    imgui_widgets.cpp
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h

    backends/imgui_impl_glfw.h
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_opengl3.h
    backends/imgui_impl_opengl3.cpp
    backends/imgui_impl_opengl3_loader.h

    implot/implot.h
    implot/implot.cpp
    implot/implot_demo.cpp
    implot/implot_internal.h
    implot/implot_items.cpp

    imspinner/imspinner.h
)

target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_SOURCES})
source_group(thirdparty/imgui FILES ${IMGUI_SOURCES})

find_package(OpenGL REQUIRED)
find_package(GLFW3)

if (${GLFW3_FOUND})
    message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")
else()
    message(STATUS "GLFW3 not found on system. Falling back to pre-packaged lib.")
    set(${GLFW3_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include)
    set(${GLFW3_LIBRARY} ${CMAKE_CURRENT_SOURCE_DIR}/glfw/lib/lib-vc2019/glfw3.lib)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLFW3_INCLUDE_DIR}
    backends
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_LIBRARY} ${GLFW3_LIBRARY})