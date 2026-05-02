cmake_minimum_required(VERSION 3.25)

project (Engine)

add_library(${PROJECT_NAME} STATIC)

# Vulkan SDK
find_package(Vulkan REQUIRED)

include(../cmake/Shaders.cmake)

# SDL3 (from source)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL_DOCS ON CACHE BOOL "" FORCE)
set(SDL_CAMERA  OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR  OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC  OFF CACHE BOOL "" FORCE)
set(SDL_POWER   OFF CACHE BOOL "" FORCE)
set(SDL_DIALOG  OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_HIDAPI   OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL
                 ${CMAKE_CURRENT_BINARY_DIR}/SDL
                 EXCLUDE_FROM_ALL)

# GLM
add_subdirectory(libs/glm)

file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
)
target_sources(${PROJECT_NAME} PRIVATE ${ENGINE_SRC})

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
 Vulkan::Vulkan
 SDL3::SDL3
 glm
 )

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

file(GLOB_RECURSE SHADER_SRC CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/assets/shaders/*.vert
  ${CMAKE_SOURCE_DIR}/assets/shaders/*.frag
)

add_shaders(KreidaEngineShaders ${SHADER_SRC})
add_dependencies(Engine KreidaEngineShaders)