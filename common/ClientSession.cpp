#include <boost/format.hpp>

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

void ClientSession::SendCommand(const CommandMessage& command,
                                const ResultCallback& callback)
{
    const auto self = shared_from_this();
    m_strand.post([self, command, callback]()
    {
        if (self->m_resultCallbacks.count(command.id) != 0)
        {
            self->m_logger->LogRecord((boost::format("Command with id %1% is already in processing")
                                       % command.id).str());
            callback(false, std::string());
            return;
        }

        self->m_resultCallbacks.insert({ command.id, callback });
        self->m_sender->SendMessage(command);
    });
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
    const auto self = shared_from_this();
    m_strand.post([self, result]()
    {
        const auto cbIt = self->m_resultCallbacks.find(result.commandId);
        if (cbIt == self->m_resultCallbacks.end())
        {
            self->m_logger->LogRecord(std::string("Result for unknown comand received : ")
                                      + std::to_string(result.commandId));
            return;
        }

        const auto& callback = (*cbIt).second;

        switch (result.code)
        {
        case ResultMessage::UnknownCommand:
        {
            self->m_logger->LogRecord("Unknown command");
            callback(false, std::string());
            break;
        }
        case ResultMessage::WrongCommandFormat:
        {
            self->m_logger->LogRecord("Wrong command format");
            callback(false, std::string());
            break;
        }
        case ResultMessage::InsertSuccess:
        case ResultMessage::UpdateSuccess:
        case ResultMessage::GetSuccess:
        case ResultMessage::DeleteSuccess:
        {
            self->m_logger->LogRecord("OK");
            callback(true, result.value.Get());
            break;
        }

        case ResultMessage::InsertFailed:
        case ResultMessage::UpdateFailed:
        case ResultMessage::GetFailed:
        case ResultMessage::DeleteFailed:
        {
            self->m_logger->LogRecord("Failed");
            callback(false, std::string());
            break;
        }
        default:
        {
            self->m_logger->LogRecord(std::string("Unknown result code : ")
                                      + std::to_string(result.code));
            callback(false, std::string());
            break;
        }
        }

        self->m_resultCallbacks.erase(cbIt);
    });
}

}// namespace kvdb
