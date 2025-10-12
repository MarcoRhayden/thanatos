cmake_minimum_required(VERSION 3.26)

# =============================================================================
# Project
# =============================================================================
project(Thanatos LANGUAGES CXX)

# =============================================================================
# Language / Global defs
# =============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)

# =============================================================================
# Platform tweaks
# =============================================================================
if (MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  add_compile_options(/utf-8)  # treat source as UTF-8
endif()

set(THANATOS_WINVER 0x0A00)
set(THANATOS_WIN_DEFS
  _WIN32_WINNT=${THANATOS_WINVER}
  WIN32_LEAN_AND_MEAN
  NOMINMAX
)

# =============================================================================
# Options
# =============================================================================
option(THANATOS_USE_WINCNG "Use Windows CNG (bcrypt) RNG on Windows" ON)

# =============================================================================
# Dependencies (vcpkg/system)
# =============================================================================
find_package(spdlog CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)

cmake_policy(SET CMP0167 NEW)
find_package(Boost REQUIRED COMPONENTS system asio)

# =============================================================================
# Ragnarok interface library (static)
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
  # Handlers / Flows
  src/interface/ragnarok/char/CharHandler.hpp
  src/interface/ragnarok/login/LoginHandler.hpp
  src/interface/ragnarok/login/LoginFlow.hpp
  src/interface/ragnarok/char/CharFlow.hpp

  # Protocol / DTO / Mappers
  src/interface/ragnarok/model/SpawnTable.hpp
  src/interface/ragnarok/protocol/Codec.hpp
  src/interface/ragnarok/protocol/Coords.hpp
  src/interface/ragnarok/protocol/Opcodes.hpp
  src/interface/ragnarok/dto/ActorDTO.hpp
  src/interface/ragnarok/dto/CharacterDTO.hpp
  src/interface/ragnarok/dto/MapDTO.hpp
  src/interface/ragnarok/dto/SystemDTO.hpp
  src/interface/ragnarok/dto/LoginDTO.hpp
  src/interface/ragnarok/mappers/ActorMapper.hpp
  src/interface/ragnarok/mappers/CharacterMapper.hpp
  src/interface/ragnarok/mappers/MapMapper.hpp
  src/interface/ragnarok/mappers/SystemMapper.hpp
  src/interface/ragnarok/mappers/LoginMapper.hpp

  # Crypto ports/adapters
  src/application/ports/crypto/IRng.hpp
  src/application/ports/crypto/ITokenGenerator.hpp
  src/infrastructure/crypto/Base64Url.hpp
  src/infrastructure/crypto/Random16TokenGenerator.hpp
  src/infrastructure/crypto/WinCngRng.hpp
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
  target_compile_definitions(arkan_ragnarok PRIVATE ${THANATOS_WIN_DEFS})
else()
  target_compile_options(arkan_ragnarok PRIVATE -Wall -Wextra -Wpedantic)
endif()

# =============================================================================
# Main executable
# =============================================================================
add_executable(Thanatos
  src/bootstrap/Main.cpp

  # Infra
  src/infrastructure/log/Logger.cpp
  src/infrastructure/config/Config.cpp
  src/infrastructure/net/asio/AsioTcpServer.cpp
  src/infrastructure/net/asio/AsioTcpClient.cpp

  # Shared states
  src/application/state/SessionRegistry.cpp
)

target_include_directories(Thanatos
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Thanatos
  PRIVATE
    arkan_ragnarok
    spdlog::spdlog
    tomlplusplus::tomlplusplus
    Boost::system
    Boost::asio
)

if (MSVC)
  target_compile_options(Thanatos PRIVATE /W4 /permissive-)
  target_compile_definitions(Thanatos PRIVATE ${THANATOS_WIN_DEFS})
else()
  target_compile_options(Thanatos PRIVATE -Wall -Wextra -Wpedantic)
endif()

# =============================================================================
# Windows resources (icon/version) + CNG RNG
# =============================================================================
if (WIN32)
  enable_language(RC)
  set(THANATOS_RES_DIR "${CMAKE_SOURCE_DIR}/resources")
  set(THANATOS_RC      "${THANATOS_RES_DIR}/thanatos.rc")

  if (EXISTS "${THANATOS_RC}")
    target_sources(Thanatos PRIVATE "${THANATOS_RC}")
    if (MSVC)
      source_group("Resource Files" FILES "${THANATOS_RC}")
    endif()
  else()
    message(WARNING "resources/thanatos.rc not found; EXE icon will not be embedded.")
  endif()

  if (THANATOS_USE_WINCNG)
    target_compile_definitions(Thanatos PRIVATE THANATOS_USE_WINCNG=1)
    target_compile_definitions(arkan_ragnarok PRIVATE THANATOS_USE_WINCNG=1)
    target_link_libraries(Thanatos PRIVATE bcrypt)
    target_link_libraries(arkan_ragnarok PRIVATE bcrypt)
  endif()
endif()

# =============================================================================
# Install
# =============================================================================
include(GNUInstallDirs)
install(TARGETS Thanatos RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# =============================================================================
# Tests
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
