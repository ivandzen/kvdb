#include "ClientSession.hpp"

namespace kvdb
{

ClientSession::ClientSession(const ClientSessionContext& context)
    : ClientSessionContext(context)
    , m_strand(m_ioContext)
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
        m_connectCallback(false);
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
    if (ec)
    {
        m_logger->LogRecord(ec.message());
        m_connectCallback(false);
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
                         std::bind(&ClientSession::onResultReceived, self, std::placeholders::_1),
                         m_onCloseCallback,
                         scReceiveDataTOutMs
                     });

    m_receiver->Start();
    m_connectCallback(true);
    return;
}

void ClientSession::onResultReceived(const ResultMessage& result)
{
    switch (result.code)
    {
    case ResultMessage::UnknownCommand:
    {
        m_logger->LogRecord("Unknown command");
        break;
    }
    case ResultMessage::WrongCommandFormat:
    {
        m_logger->LogRecord("Wrong command format");
        break;
    }
    case ResultMessage::InsertSuccess:
    case ResultMessage::UpdateSuccess:
    case ResultMessage::GetSuccess:
    case ResultMessage::DeleteSuccess:
    {
        m_logger->LogRecord("OK");
        break;
    }

    case ResultMessage::InsertFailed:
    case ResultMessage::UpdateFailed:
    case ResultMessage::GetFailed:
    case ResultMessage::DeleteFailed:
    {
        m_logger->LogRecord("Failed");
        break;
    }
    }
}

}// namespace kvdb
