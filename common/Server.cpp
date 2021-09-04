#include <functional>

#include "Server.hpp"

namespace kvdb
{

Server::Server(const ServerContext& context)
    : ServerContext(context)
    , m_acceptor(context.m_ioContext)
    , m_strand(context.m_ioContext)
{
    m_acceptor.listen(scMaxConnections);
}

void Server::initNewSession()
{
    const auto self = shared_from_this();

    ServerSessionContext sessionContext =
    {
        m_ioContext,
        m_acceptor,
        m_processor,
        m_logger,
        // protect m_sessions set from concurrent access by executing on strand
        m_strand.wrap(std::bind(&Server::onSessionInitialized, self, std::placeholders::_1)),
        m_strand.wrap(std::bind(&Server::onSessionClosed, self, std::placeholders::_1))
    };

    ServerSession::Init(sessionContext);
}

void Server::onSessionInitialized(const ServerSessionPtr& session)
{
    m_sessions.insert(session);
}

void Server::onSessionClosed(const ServerSessionPtr& session)
{
    m_sessions.erase(session);
}

} // namespace kvdb
