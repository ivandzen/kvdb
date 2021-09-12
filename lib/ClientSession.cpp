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

ClientSession::~ClientSession()
{
    m_logger.LogRecord("ClientSession destroyed");
}

void ClientSession::Connect(const std::string& hostname, int port)
{
    if (m_socket.is_open())
    {
        m_logger.LogRecord("Already connected");
        return;
    }

    boost::asio::ip::tcp::resolver::query query(hostname, std::to_string(port));
    m_resolver.async_resolve(query, std::bind(&ClientSession::onEPResolved, this,
                                              std::placeholders::_1,
                                              std::placeholders::_2));
}

void ClientSession::SendCommand(const CommandMessage& command,
                                const ResultCallback& callback)
{
    m_strand.post([this, command, callback]()
    {
        if (m_resultCallbacks.count(command.id) != 0)
        {
            m_logger.LogRecord((boost::format("Command with id %1% is already in processing")
                                % command.id).str());
            callback(false, std::string());
            return;
        }

        m_resultCallbacks.insert({ command.id, callback });
        m_sender->SendMessage(command);
    });
}

void ClientSession::onEPResolved(const boost::system::error_code& ec,
                                 boost::asio::ip::tcp::resolver::results_type results)
{
    if (ec)
    {
        m_logger.LogRecord(std::string("Failed to resolve EP: ") + ec.message());
        m_connectCallback(false);
        return;
    }

    m_socket.async_connect(*results, std::bind(&ClientSession::onSocketConnected, this,
                                               std::placeholders::_1));
}

void ClientSession::onSocketConnected(const boost::system::error_code& ec)
{
    if (ec)
    {
        m_logger.LogRecord(std::string("Failed to connect to server: ") + ec.message());
        m_connectCallback(false);
        return;
    }

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
                         m_strand.wrap(std::bind(&ClientSession::onResultReceived, this,
                                                 std::placeholders::_1)),
                         m_onCloseCallback,
                         scReceiveDataTOutMs
                     });

    m_receiver->Start();
    m_connectCallback(true);
    return;
}

void ClientSession::onResultReceived(const ResultMessage& result)
{
     const auto cbIt = m_resultCallbacks.find(result.commandId);
     if (cbIt == m_resultCallbacks.end())
     {
         m_logger.LogRecord(std::string("Result for unknown comand received : ")
                           + std::to_string(result.commandId));
         return;
     }

     const auto& callback = (*cbIt).second;

     switch (result.code)
     {
     case ResultMessage::UnknownCommand:
     {
         m_logger.LogRecord("Unknown command");
         callback(false, std::string());
         break;
     }
     case ResultMessage::WrongCommandFormat:
     {
         m_logger.LogRecord("Wrong command format");
         callback(false, std::string());
         break;
     }
     case ResultMessage::InsertSuccess:
     case ResultMessage::UpdateSuccess:
     case ResultMessage::GetSuccess:
     case ResultMessage::DeleteSuccess:
     {
         m_logger.LogRecord("OK");
         callback(true, result.value.Get());
         break;
     }

     case ResultMessage::InsertFailed:
     case ResultMessage::UpdateFailed:
     case ResultMessage::GetFailed:
     case ResultMessage::DeleteFailed:
     {
         m_logger.LogRecord("Failed");
         callback(false, std::string());
         break;
     }
     default:
     {
         m_logger.LogRecord(std::string("Unknown result code : ") + std::to_string(result.code));
         callback(false, std::string());
         break;
     }
     }

     m_resultCallbacks.erase(cbIt);
}

}// namespace kvdb
