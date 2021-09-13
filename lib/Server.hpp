#pragma once

#include <memory>
#include <set>

#include <boost/asio.hpp>

#include "Logger.hpp"
#include "ServerSession.hpp"
#include "CommandProcessor.hpp"

namespace kvdb
{

struct ServerContext
{
    boost::asio::io_context&        m_ioContext;
    Logger&                         m_logger;
    CommandProcessor&               m_processor;
    boost::asio::ip::tcp::endpoint  m_endpoint; // endpoint to listen to
};

static const uint32_t scMaxConnections = 100;

/// @brief KVDB server class
/// continuously accepts client connections and executes commands received from clients
class Server
        : public ServerContext
{
public:
    using Ptr = std::shared_ptr<Server>;

    explicit Server(const ServerContext& context);

    virtual ~Server();

    void Start();

private:
    void initNewSession();

    void onSessionInitialized(const ServerSessionPtr& session);

    void onSessionClosed(const ServerSessionPtr& session);

    boost::asio::io_context::strand m_strand;
    boost::asio::ip::tcp::acceptor  m_acceptor;
    std::set<ServerSessionPtr>      m_sessions;
};

}
