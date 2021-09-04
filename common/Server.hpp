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
    boost::asio::io_context&    m_ioContext;
    CommandProcessor::Ptr       m_processor;
    Logger::Ptr                 m_logger;
};

static const uint32_t scMaxConnections = 100;

class Server
        : public ServerContext
        , public std::enable_shared_from_this<Server>
{
public:
    explicit Server(const ServerContext& context);

private:
    void initNewSession();

    void onSessionInitialized(const ServerSessionPtr& session);

    void onSessionClosed(const ServerSessionPtr& session);

    boost::asio::io_context::strand m_strand;
    boost::asio::ip::tcp::acceptor  m_acceptor;
    std::set<ServerSessionPtr>      m_sessions;
};

}
