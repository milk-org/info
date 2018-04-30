project(lib_info_project)

include_directories ("${PROJECT_SOURCE_DIR}/src")
include_directories ("${PROJECT_SOURCE_DIR}/..")
add_library(info SHARED info.c)

install(TARGETS info DESTINATION lib)
install(FILES info.h DESTINATION include)

