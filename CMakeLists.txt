cmake_minimum_required(VERSION 3.25)

project (Game)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME}
    source/main.cpp
)

file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
)
target_sources(${PROJECT_NAME} PRIVATE ${ENGINE_SRC})

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_subdirectory(engine)

target_link_libraries(${PROJECT_NAME} PRIVATE Engine)

# Copy SDL3.dll next to exe (if shared)
if (WIN32 AND TARGET SDL3::SDL3)
  get_target_property(_sdl_type SDL3::SDL3 TYPE)
  if (_sdl_type STREQUAL "SHARED_LIBRARY")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
  endif()
endif()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)