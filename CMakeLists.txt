cmake_minimum_required(VERSION 3.26)

# =============================================================================
# Project
# Core build setup for Thanatos.
# Thanatos 用のビルド設定（コア）。
# =============================================================================
project(thanatos LANGUAGES CXX)

# =============================================================================
# Toolchain / Language standard / Global definitions
# Require C++20; enable spdlog debug level globally.
# C++20 を必須にし、spdlog のデバッグレベルを全体に適用。
# =============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)

# =============================================================================
# Platform-specific tweaks
# On MSVC, make the CRT warnings quiet and use UTF-8 source.
#   Also define Windows version for Boost.Asio and slim down <Windows.h>.
#
# MSVC では CRT 警告を抑制し、ソースを UTF-8 とする。
#   Boost.Asio 用の Windows ターゲット定義と <Windows.h> の軽量化も設定。
# =============================================================================
if (MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  add_compile_options(/utf-8)      # treat source files as UTF-8
endif()

set(THANATOS_WINVER 0x0A00)
set(THANATOS_WIN_DEFS
  _WIN32_WINNT=${THANATOS_WINVER}
  WIN32_LEAN_AND_MEAN
  NOMINMAX
)

# =============================================================================
# Dependencies (via vcpkg or system)
# Boost.Asio is header-only; we still link Boost.System.
# Boost.Asio はヘッダオンリー。Boost.System はリンクが必要。
# =============================================================================
find_package(spdlog CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)

cmake_policy(SET CMP0167 NEW)
find_package(Boost REQUIRED COMPONENTS system asio)

# =============================================================================
# Ragnarok interface library (static)
# Protocol/flows + query server + GG bridge.
# プロトコル/フロー、クエリサーバ、GG ブリッジをまとめた静的ライブラリ。
# =============================================================================
set(RAGNAROK_SRC
  # Ragnarok / flows / protocol
  src/interface/ragnarok/RagnarokServer.cpp
  src/interface/ragnarok/char/CharHandler.cpp
  src/interface/ragnarok/char/CharFlow.cpp
  src/interface/ragnarok/login/LoginHandler.cpp
  src/interface/ragnarok/login/LoginFlow.cpp

  src/interface/ragnarok/protocol/Codec.cpp

  # Query + Services
  src/interface/query/QueryServer.cpp
  src/application/services/GameGuardBridge.cpp
)

set(RAGNAROK_HDR
  src/interface/ragnarok/char/CharHandler.hpp
  src/interface/ragnarok/login/LoginHandler.hpp
  src/interface/ragnarok/login/LoginFlow.hpp
  src/interface/ragnarok/char/CharFlow.hpp
  src/interface/ragnarok/model/SpawnTable.hpp
  src/interface/ragnarok/protocol/Codec.hpp
  src/interface/ragnarok/protocol/Coords.hpp
  
  src/interface/ragnarok/protocol/Opcodes.hpp
  src/interface/ragnarok/dto/ActorDTO.hpp
  src/interface/ragnarok/dto/CharacterDTO.hpp
  src/interface/ragnarok/dto/MapDTO.hpp
  src/interface/ragnarok/dto/SystemDTO.hpp
  src/interface/ragnarok/mappers/ActorMapper.hpp
  src/interface/ragnarok/mappers/CharacterMapper.hpp
  src/interface/ragnarok/mappers/MapMapper.hpp
  src/interface/ragnarok/mappers/SystemMapper.hpp
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
    Boost::system      # Asio uses system errors
    Boost::asio        # header-only target, but keep for include dirs/features
)

if (MSVC)
  target_compile_options(arkan_ragnarok PRIVATE /W4 /permissive-)
  target_compile_definitions(arkan_ragnarok PRIVATE ${THANATOS_WIN_DEFS})
else()
  target_compile_options(arkan_ragnarok PRIVATE -Wall -Wextra -Wpedantic)
endif()

# =============================================================================
# Main executable
# Bootstrap + infrastructure + shared state + dev bridge.
# ブートストラップ、インフラ、共有状態、開発用ブリッジをリンク。
# =============================================================================
add_executable(thanatos
  src/bootstrap/Main.cpp

  # Infra
  src/infrastructure/log/Logger.cpp
  src/infrastructure/config/Config.cpp
  src/infrastructure/net/asio/AsioTcpServer.cpp
  src/infrastructure/net/asio/AsioTcpClient.cpp

  # Shared states
  src/application/state/SessionRegistry.cpp
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
  target_compile_definitions(thanatos PRIVATE ${THANATOS_WIN_DEFS})
else()
  target_compile_options(thanatos PRIVATE -Wall -Wextra -Wpedantic)
endif()

# =============================================================================
# Install
# Install exe in the standard binary dir.
# 実行ファイルを標準のバイナリディレクトリにインストール。
# =============================================================================
include(GNUInstallDirs)
install(TARGETS thanatos RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# =============================================================================
# Tests
# Optional; uses GTest via vcpkg.
# 任意。vcpkg 経由の GTest を使用。
# =============================================================================
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
      target_compile_definitions(${exe_name} PRIVATE ${THANATOS_WIN_DEFS})
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
endif()