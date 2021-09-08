#include <functional>

#include "Server.hpp"

namespace kvdb
{

Server::Server(const ServerContext& context)
    : ServerContext(context)
    , m_acceptor(context.m_ioContext, context.m_endpoint)
    , m_strand(context.m_ioContext)
{
    m_acceptor.listen(scMaxConnections);
}

Server::~Server()
{
    m_logger.LogRecord("Server destroyed");
}

void Server::Start()
{
    initNewSession();
}

void Server::initNewSession()
{
    ServerSession::Init(ServerSessionContext {
                            m_ioContext,
                            m_logger,
                            m_acceptor,
                            m_processor,
                            // protect m_sessions set from concurrent access by executing on strand
                            m_strand.wrap(std::bind(&Server::onSessionInitialized, this, std::placeholders::_1)),
                            m_strand.wrap(std::bind(&Server::onSessionClosed, this, std::placeholders::_1))
                        });
}

void Server::onSessionInitialized(const ServerSessionPtr& session)
{
    m_sessions.insert(session);
    initNewSession(); // continue to accept clients
}

void Server::onSessionClosed(const ServerSessionPtr& session)
{
    m_sessions.erase(session);
}

} // namespace kvdb
