#include <functional>

#include <boost/format.hpp>

#include "ServerSession.hpp"

namespace kvdb
{

ServerSession::ServerSession(const ServerSessionContext& context)
    : ServerSessionContext(context)
    , m_strand(context.m_ioContext)
    , m_socket(context.m_ioContext)
{}

ServerSession::~ServerSession()
{
    m_logger.LogRecord("ServerSession destroyed");
}

void ServerSession::Init(const ServerSessionContext& context)
{
    auto newSession = std::make_shared<ServerSession>(context);
    newSession->m_acceptor.async_accept(newSession->m_socket,
                                        std::bind(&ServerSession::onConnectionAccepted, newSession,
                                                  std::placeholders::_1));
}

std::string ServerSession::Address() const
{
    return (boost::format("%1%:%2%")
            % m_socket.remote_endpoint().address().to_string()
            % m_socket.remote_endpoint().port()).str();
}

void ServerSession::onConnectionAccepted(const boost::system::error_code& error)
{
    if (error)
    {
        m_logger.LogRecord((boost::format("Failed to accept new connection : %1%") % error).str());
        return;
    }

    m_logger.LogRecord(std::string("Connection accepted ") + Address());

    m_sender = std::make_shared<Sender>(
                   MessageSenderContext {
                       m_logger,
                       m_strand,
                       m_socket
                   });

    m_receiver = std::make_shared<Receiver>(
                     Receiver::Context {
                         m_ioContext,
                         m_strand,
                         m_socket,
                         m_logger,
                         std::bind(&ServerSession::onCommandReceived, this, std::placeholders::_1),
                         std::bind(&ServerSession::onConnectionClosed, this),
                         scReceiveDataTOutMs
                     });

    m_receiver->Start();
    m_initCallback(shared_from_this());
}

void ServerSession::onCommandReceived(const CommandMessage& command)
{
    m_logger.LogRecord((boost::format("Command received:\n\ttype = %1%"
                                      "\n\tkey.size = %2%\n\tvalue.size = %3%")
                         % int(command.type)
                         % command.key.Get().size()
                         % command.value.Get().size()).str());

    m_processor.ProcessCommand(command,
                               std::bind(&Sender::SendMessage, m_sender,
                                         std::placeholders::_1));
}

void ServerSession::onConnectionClosed()
{
    m_logger.LogRecord(std::string("Connection closed ") + Address());
    m_socket.close();
    m_closeCallback(shared_from_this());
}

} // namespace kvdb
