cmake_minimum_required(VERSION 3.26)
project(arkan_poseidon LANGUAGES CXX)

# Windows/Asio targets + MSVC warnings
add_compile_definitions(_WIN32_WINNT=0x0601)
if (MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================
# Dependencies (vcpkg)
# =========================================
find_package(spdlog CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system asio)

# =========================================
# Main executable
# =========================================
add_executable(arkan_poseidon
  src/interface/bootstrap/Main.cpp
  src/infrastructure/log/Logger.cpp
  src/infrastructure/config/Config.cpp
  src/infrastructure/net/asio/AsioTcpServer.cpp
  src/infrastructure/net/asio/AsioTcpClient.cpp
  src/application/services/LoginService.cpp
  src/application/services/CharService.cpp
)

target_include_directories(arkan_poseidon
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(arkan_poseidon
  PRIVATE
    spdlog::spdlog
    tomlplusplus::tomlplusplus
    Boost::system
    Boost::asio
)

if (MSVC)
  target_compile_options(arkan_poseidon PRIVATE /W4 /permissive-)
else()
  target_compile_options(arkan_poseidon PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(GNUInstallDirs)
install(TARGETS arkan_poseidon RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# =========================================
# Tests
# =========================================
include(CTest)
option(BUILD_TESTING "Build tests" ON)

if (BUILD_TESTING)
  find_package(GTest CONFIG REQUIRED)

  add_executable(arkan_poseidon_tests
    tests/ProtocolTests.cpp
    tests/LoginServiceTests.cpp
    tests/CharServiceTests.cpp

    # Implementations used by tests
    src/application/services/LoginService.cpp
    src/application/services/CharService.cpp
  )

  target_include_directories(arkan_poseidon_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(arkan_poseidon_tests
    PRIVATE
      GTest::gtest
      GTest::gtest_main
      spdlog::spdlog
      tomlplusplus::tomlplusplus
      Boost::system
      Boost::asio
  )

  if (MSVC)
    target_compile_options(arkan_poseidon_tests PRIVATE /W4 /permissive-)
  else()
    target_compile_options(arkan_poseidon_tests PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  # Automatic discovery of each TEST as a CTest case
  include(GoogleTest)
  gtest_discover_tests(arkan_poseidon_tests
    DISCOVERY_MODE PRE_TEST
  )
endif()
