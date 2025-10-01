#pragma once

#include <boost/asio.hpp>
#include <cstdint>
#include <vector>

namespace arkan::thanatos::infrastructure::net::asio_impl
{
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using Buffer = std::vector<std::uint8_t>;
}  // namespace arkan::thanatos::infrastructure::net::asio_impl
