cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(iex_pcap)
set (CMAKE_CXX_STANDARD 11)

############################################################
### PcapPlusPlus

if(WIN32)
        SET(OS_CONFIG_COMMAND "configure-windows-mingw.bat")
endif()
IF(APPLE)
        SET(OS_CONFIG_COMMAND "configure-mac_os_x.sh")
endif()
IF(UNIX AND NOT APPLE)
        SET(OS_CONFIG_COMMAND "configure-linux.sh")
endif()

include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
ExternalProject_Add(project_pcapplusplus
        GIT_REPOSITORY "https://github.com/seladb/PcapPlusPlus"
        GIT_TAG "master"

        UPDATE_COMMAND ""
        PATCH_COMMAND ""

        SOURCE_DIR "${CMAKE_SOURCE_DIR}/3rdparty/pcapplusplus"
        CONFIGURE_COMMAND ./${OS_CONFIG_COMMAND} --default

        BUILD_COMMAND make
        BUILD_IN_SOURCE 1

        INSTALL_COMMAND ""

        TEST_COMMAND ""
)

# Add the link and include directories from the external library.
link_directories("3rdparty/pcapplusplus/Dist")
include_directories("3rdparty/pcapplusplus/Dist/header")

############################################################
### Gtest

ExternalProject_Add(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    UPDATE_COMMAND ""
    PATCH_COMMAND ""

    CMAKE_ARGS -Dgtest_force_shared_crt=ON
               -DBUILD_GTEST=ON
    PREFIX "${CMAKE_BINARY_DIR}/gtest"
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(googletest source_dir)
set(GTEST_INCLUDE_DIRS ${source_dir}/googletest/include)

ExternalProject_Get_Property(googletest binary_dir)
set(GTEST_LIBS_DIR ${binary_dir}/lib)

include_directories(${GTEST_INCLUDE_DIRS})
link_directories(${GTEST_LIBS_DIR})


############################################################
### IEX library
include_directories("include")

SET(EXT_LIBRARIES Packet++
                  Pcap++
                  Common++
                  pcap)

add_library(iex_pcap "src/iex_decoder.cpp" "src/iex_messages")
install(TARGETS iex_pcap DESTINATION ${CMAKE_SOURCE_DIR}/lib)
add_dependencies(iex_pcap project_pcapplusplus)
add_dependencies(iex_pcap googletest)

add_executable(csv_example  "src/csv_example.cpp")
target_link_libraries(csv_example iex_pcap ${EXT_LIBRARIES})
install(TARGETS csv_example DESTINATION ${CMAKE_SOURCE_DIR}/bin)


### Unit tests
add_executable(test_test "test/test.cpp")
target_link_libraries(test_test gtest gmock iex_pcap pthread ${EXT_LIBRARIES})



