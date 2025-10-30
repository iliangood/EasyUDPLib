cmake_minimum_required(VERSION 3.28.3)
project(EasyUDPLib VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -pedantic)

add_library(udp_library STATIC
    src/dynamicMessage.cpp
    src/udpsocket.cpp
	src/ipaddress.cpp
	src/udptransmitter.cpp
)

target_include_directories(udp_library PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(WIN32)
    target_link_libraries(udp_library PRIVATE ws2_32)
endif()

install(TARGETS udp_library
    EXPORT UDPLibraryTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT UDPLibraryTargets
    FILE UDPLibraryTargets.cmake
    NAMESPACE EasyUDPLibrary::
    DESTINATION lib/cmake/EasyUDPLibrary
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/EasyUDPLibraryConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

include(CMakePackageConfigHelpers)

# Генерация Config-файла
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EasyUDPLibraryConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/EasyUDPLibraryConfig.cmake"
    INSTALL_DESTINATION lib/cmake/EasyUDPLibrary
)

# Версия
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/EasyUDPLibraryConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Установка
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/EasyUDPLibraryConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/EasyUDPLibraryConfigVersion.cmake"
    DESTINATION lib/cmake/EasyUDPLibrary
)

add_subdirectory(example)