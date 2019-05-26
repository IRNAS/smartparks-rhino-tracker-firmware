cmake_minimum_required(VERSION 2.8.8)
project(arduino-mock-test-all)

find_package(Threads REQUIRED)
add_subdirectory(arduino_mock)

include_directories(
    ${ARDUINO_MOCK_INCLUDE_DIRS}
    ${ARDUINO_MOCK_LIBS_DIR}/lib/gtest/gtest/src/gtest/googletest/include
    ${ARDUINO_MOCK_LIBS_DIR}/lib/gtest/gtest/src/gtest/googlemock/include
)

file(GLOB LIB_SRCS "../*.ino")
file(GLOB SRCS "*.cpp")
add_executable(test-all ${SRCS} ${LIB_SRCS})

target_link_libraries(test-all
    ${ARDUINO_MOCK_LIBS_DIR}/lib/gtest/gtest/src/gtest-build/googlemock/gtest/libgtest.a
    ${ARDUINO_MOCK_LIBS_DIR}/lib/gtest/gtest/src/gtest-build/googlemock/libgmock.a
    ${ARDUINO_MOCK_LIBS_DIR}/dist/lib/libarduino_mock.a
    ${CMAKE_THREAD_LIBS_INIT}
)

add_dependencies(test-all arduino_mock)

enable_testing()
add_test(TestAll test-all)
