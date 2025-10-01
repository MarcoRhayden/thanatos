cmake_minimum_required(VERSION 3.26)
project(thanatos LANGUAGES CXX)

# =========================================================
# Toolchain / Standard / Warnings
# =========================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
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
# Ragnarok interface library
# =========================================================
set(RAGNAROK_SRC
  # Ragnarok / flows / proto
  src/interface/ragnarok/RagnarokServer.cpp
  src/interface/ragnarok/char/CharHandler.cpp
  src/interface/ragnarok/char/CharFlow.cpp
  src/interface/ragnarok/login/LoginHandler.cpp
  src/interface/ragnarok/login/LoginFlow.cpp
  src/interface/ragnarok/proto/Codec.cpp

  src/interface/query/QueryServer.cpp
  src/application/services/GameGuardBridge.cpp
)

set(RAGNAROK_HDR
  src/interface/ragnarok/char/CharHandler.hpp
  src/interface/ragnarok/login/LoginHandler.hpp
  src/interface/ragnarok/login/LoginFlow.hpp
  src/interface/ragnarok/char/CharFlow.hpp
  src/interface/ragnarok/proto/Codec.hpp
  src/interface/ragnarok/proto/Coords.hpp
  src/interface/ragnarok/proto/Messages.hpp
  src/interface/ragnarok/proto/LoginMessages.hpp
  src/interface/ragnarok/model/SpawnTable.hpp
)

add_library(arkan_ragnarok STATIC ${RAGNAROK_SRC} ${RAGNAROK_HDR})
target_include_directories(arkan_ragnarok
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(arkan_ragnarok
  PUBLIC
    spdlog::spdlog
    tomlplusplus::tomlplusplus
    Boost::system
    Boost::asio
)
if (MSVC)
  target_compile_options(arkan_ragnarok PRIVATE /W4 /permissive-)
else()
  target_compile_options(arkan_ragnarok PRIVATE -Wall -Wextra -Wpedantic)
endif()

# =========================================================
# Main executable
# =========================================================
add_executable(thanatos
  src/interface/bootstrap/Main.cpp

  # Infra
  src/infrastructure/log/Logger.cpp
  src/infrastructure/config/Config.cpp
  src/infrastructure/net/asio/AsioTcpServer.cpp
  src/infrastructure/net/asio/AsioTcpClient.cpp

  # Shared states
  src/application/state/SessionRegistry.cpp

  src/interface/dev/RoBridgeHandler.cpp
)

target_include_directories(thanatos
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(thanatos
  PRIVATE
    arkan_ragnarok
    spdlog::spdlog
    tomlplusplus::tomlplusplus
    Boost::system
    Boost::asio
)
if (MSVC)
  target_compile_options(thanatos PRIVATE /W4 /permissive-)
else()
  target_compile_options(thanatos PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(GNUInstallDirs)
install(TARGETS thanatos RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

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
        arkan_ragnarok
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
  )

endif()
