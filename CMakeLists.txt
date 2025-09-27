cmake_minimum_required(VERSION 3.26)
project(arkan_poseidon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTING "Build tests" OFF)

find_package(spdlog CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

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
    Boost::system
    tomlplusplus::tomlplusplus  
)

if (MSVC)
  target_compile_options(arkan_poseidon PRIVATE /W4 /permissive-)
else()
  target_compile_options(arkan_poseidon PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(GNUInstallDirs)
install(TARGETS arkan_poseidon RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
