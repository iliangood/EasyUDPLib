cmake_minimum_required(VERSION 3.28.3)
project(EasyUDPLib VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -pedantic)

add_library(udp_library STATIC
    src/dynamicMessage.cpp
    src/udpsocket.cpp
	src/ipaddress.cpp
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
    NAMESPACE UDPLibrary::
    DESTINATION lib/cmake/UDPLibrary
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/UDPLibraryConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/UDPLibraryConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/UDPLibraryConfigVersion.cmake"
    DESTINATION lib/cmake/UDPLibrary
)

add_subdirectory(example)