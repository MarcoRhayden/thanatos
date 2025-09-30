#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "application/ports/query/IQueryServer.hpp"

namespace boost
{
namespace asio
{
class io_context;
}
}  // namespace boost

namespace arkan::poseidon::interface::query
{

// Implementation of the IQueryServer port speaking the "PSDN" frame.
// Frame:
// magic[4] = "PSDN"
// type(u16) = 0x0051 Query / 0x0052 Reply
// length(u32) = N
// payload[N]
class QueryServer final : public application::ports::query::IQueryServer
{
   public:
    QueryServer(boost::asio::io_context& io, const std::string& host, std::uint16_t port);
    ~QueryServer();

    void start() override;
    void stop() override;

    void onQuery(std::function<void(std::vector<std::uint8_t>)> cb) override;
    void sendReply(const std::vector<std::uint8_t>& gg_reply) override;

    std::string endpoint_description() const override;

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace arkan::poseidon::interface::query
