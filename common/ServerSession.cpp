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

void ServerSession::Init(const ServerSessionContext& context)
{
    auto newSession = std::make_shared<ServerSession>(context);
    newSession->m_acceptor.async_accept(newSession->m_socket, [newSession](const boost::system::error_code& error)
    {
        newSession->onConnectionAccepted(error);
    });
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
        m_logger->LogRecord((boost::format("Failed to accept new connection : %1%") % error).str());
        return;
    }

    m_sender = std::make_shared<Sender>(
                   MessageSenderContext {
                       m_strand,
                       m_socket,
                       m_logger
                   });

    const auto self = shared_from_this();
    m_receiver = std::make_shared<Receiver>(
                     Receiver::Context {
                         m_ioContext,
                         m_strand,
                         m_socket,
                         m_logger,
                         std::bind(&ServerSession::onCommandReceived, self, std::placeholders::_1),
                         [self](){ self->m_closeCallback(self); },
                         scReceiveDataTOutMs
                     });

    m_receiver->Start();
    m_initCallback(self);
}

void ServerSession::onCommandReceived(const CommandMessage& command)
{
    m_processor->ProcessCommand(command,
                                std::bind(&Sender::SendMessage,
                                          m_sender,
                                          std::placeholders::_1));
}

} // namespace kvdb
