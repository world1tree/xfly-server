add_library(${PROJECT_NAME}_log_lib STATIC fmtlog.cc)
# 依赖fmt, 这种错误似乎不会在build library阶段发现
find_package(fmt)
target_link_libraries(${PROJECT_NAME}_log_lib fmt)