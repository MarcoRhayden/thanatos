cmake_minimum_required(VERSION 3.26)
project(arkan_poseidon LANGUAGES CXX)

# =========================================================
# Toolchain / Standard / Warnings
# =========================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(_WIN32_WINNT=0x0601)  # Win7+
if (MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# =========================================================
# Dependencies (vcpkg)
# =========================================================
find_package(spdlog CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system asio)

# =========================================================
# Main executable
# =========================================================
add_executable(arkan_poseidon
  src/interface/bootstrap/Main.cpp

  # Infra
  src/infrastructure/log/Logger.cpp
  src/infrastructure/config/Config.cpp
  src/infrastructure/net/asio/AsioTcpServer.cpp
  src/infrastructure/net/asio/AsioTcpClient.cpp

  # Shared states
  src/application/state/SessionRegistry.cpp

  # Query bridge
  src/interface/query/QueryHandler.cpp

  # RO bridge
  src/interface/dev/RoBridgeHandler.cpp

  # RagnarokServer
  src/interface/ro/RagnarokServer.cpp
  src/interface/ro/LoginHandler.cpp
  src/interface/ro/CharHandler.cpp
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

# =========================================================
# Tests
# =========================================================
include(CTest)
option(BUILD_TESTING "Build tests" ON)

if (BUILD_TESTING)
  find_package(GTest CONFIG REQUIRED)

  function(make_test exe_name)
    add_executable(${exe_name} ${ARGN})
    target_include_directories(${exe_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(${exe_name}
      PRIVATE
        GTest::gtest
        GTest::gtest_main
        spdlog::spdlog
        tomlplusplus::tomlplusplus
        Boost::system
        Boost::asio
    )
    if (MSVC)
      target_compile_options(${exe_name} PRIVATE /W4 /permissive-)
    else()
      target_compile_options(${exe_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    include(GoogleTest)
    gtest_discover_tests(${exe_name} DISCOVERY_MODE PRE_TEST)
  endfunction()

  make_test(tests_net_client
    tests/NetClientTests.cpp
    src/infrastructure/net/asio/AsioTcpClient.cpp
    src/infrastructure/net/asio/AsioTcpServer.cpp
    src/infrastructure/log/Logger.cpp
  )

  make_test(tests_ro_bridge
    tests/RoBridgeHandlerTests.cpp
    src/interface/dev/RoBridgeHandler.cpp
    src/infrastructure/net/asio/AsioTcpClient.cpp
    src/infrastructure/log/Logger.cpp
  )

endif()