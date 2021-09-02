#include <boost/format.hpp>

#include "ServerSession.hpp"

namespace kvdb
{

ServerSession::ServerSession(const ServerSessionContext& context)
    : ServerSessionContext(context)
    , m_socket(context.m_ioContext)
{}

void ServerSession::StartAccept(const ServerSessionContext& context,
                                const InitCallback& initCallback)
{
    auto newSession = std::make_shared<ServerSession>(context);



    newSession->m_acceptor.async_accept(newSession->m_socket, [newSession, initCallback](const boost::system::error_code& error)
    {
        newSession->onConnectionAccepted(error, initCallback);
    });
}

void ServerSession::onConnectionAccepted(const boost::system::error_code& error,
                                         const InitCallback& initCallback)
{
    if (error)
    {
        m_logger->LogRecord((boost::format("Failed to accept new connection : %1%") % error).str());
        initCallback(nullptr);
        return;
    }

    m_sender = std::make_shared<Sender>(
                   MessageSenderContext {
                       boost::asio::io_context::strand(m_ioContext),
                       m_socket,
                       m_logger
                   });

    const auto self = shared_from_this();
    const auto onMessage = [self](const CommandMessage& message)
    {
        self->m_processor->ProcessCommand(message, [self](const ResultMessage& result)
        {
            self->m_sender->SendMessage(result);
        });
    };

    m_receiver = std::make_shared<Receiver>(
                     Receiver::Context {
                         m_ioContext,
                         m_socket,
                         m_logger,
                         onMessage,
                         1000 // receiver data timeout
                     });

    initCallback(shared_from_this());
}

} // namespace kvdb
