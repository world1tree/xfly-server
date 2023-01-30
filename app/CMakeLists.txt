file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
# 生成execute文件，只需要包含链接需要的.cpp文件就可以, 头文件可以通过include_directories设定
add_executable(${PROJECT_NAME}_bin ${sources})
target_link_libraries(${PROJECT_NAME}_bin ${PROJECT_NAME}_log_lib)
# 生成.lib文件，可以链接到其它程序中, 这样就不需要显示指定具体的实现文件(.cpp文件)
add_library(${PROJECT_NAME}_app_lib STATIC ${sources})
target_link_libraries(${PROJECT_NAME}_bin
        ${PROJECT_NAME}_log_lib
        ${PROJECT_NAME}_signal_lib
        ${PROJECT_NAME}_proc_lib
        ${PROJECT_NAME}_net_lib
        ${PROJECT_NAME}_misc_lib
        )