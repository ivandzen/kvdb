#include "ClientSession.hpp"

namespace kvdb
{

ClientSession::ClientSession(const ClientSessionContext& context)
    : ClientSessionContext(context)
    , m_resolver(m_ioContext)
    , m_socket(m_ioContext)
{
}

void ClientSession::Connect()
{
    if (m_socket.is_open())
    {
        m_logger->LogRecord("Already connected");
        return;
    }

    boost::asio::ip::tcp::resolver::query query(m_hostname, std::to_string(m_port));
    const WeakPtr weakSelf = shared_from_this();
    m_resolver.async_resolve(query, [weakSelf](const boost::system::error_code& error,
                                               boost::asio::ip::tcp::resolver::results_type results)
    {
        if (const auto self = weakSelf.lock())
        {
            self->onEPResolved(error, results);
        }
    });
}

void ClientSession::SendCommand(const CommandMessage& command)
{
    m_sender->SendMessage(command);
}

void ClientSession::onEPResolved(const boost::system::error_code& ec,
                                 boost::asio::ip::tcp::resolver::results_type results)
{
    if (ec)
    {
        m_logger->LogRecord(ec.message());
        m_callback(false);
        return;
    }

    const WeakPtr weakSelf = shared_from_this();
    m_socket.async_connect(*results, [weakSelf](const boost::system::error_code& ec)
    {
        if (const auto self = weakSelf.lock())
        {
            self->onSocketConnected(ec);
        }
    });
}

void ClientSession::onSocketConnected(const boost::system::error_code& ec)
{
    if (!ec)
    {
        m_logger->LogRecord(ec.message());
        m_callback(false);
        return;
    }

    MessageSenderContext msContext
    {
        boost::asio::io_context::strand(m_ioContext),
        m_socket,
        m_logger
    };

    m_sender = std::make_shared<MessageSender<CommandMessage>>(msContext);
    m_callback(true);
    return;
}

}// namespace kvdb
