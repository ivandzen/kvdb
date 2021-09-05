#pragma once

#include <memory>
#include <functional>

#include <boost/asio.hpp>

#include "MessageSender.hpp"
#include "MessageReceiver.hpp"
#include "Logger.hpp"

namespace kvdb
{

struct ClientSessionContext
{
    using ConnectCallback = std::function<void(bool)>;

    boost::asio::io_context&    m_ioContext;
    Logger::Ptr                 m_logger;
    ConnectCallback             m_callback;
    std::string                 m_hostname;
    int                         m_port;
};

class ClientSession
        : public ClientSessionContext
        , public std::enable_shared_from_this<ClientSession>
{
public:
    using Ptr = std::shared_ptr<ClientSession>;
    using WeakPtr = std::weak_ptr<ClientSession>;

    explicit ClientSession(const ClientSessionContext& context);

    void Connect();

    void SendCommand(const CommandMessage& command);

private:
    using Sender = MessageSender<CommandMessage>;
    using Receiver = MessageReceiver<ResultMessage>;

    void onEPResolved(const boost::system::error_code& error,
                      boost::asio::ip::tcp::resolver::results_type results);

    void onSocketConnected(const boost::system::error_code& ec);

    void onResultReceived(const ResultMessage& result);

    void onConnectionClosed();

    boost::asio::ip::tcp::resolver  m_resolver;
    boost::asio::ip::tcp::socket    m_socket;
    Sender::Ptr                     m_sender;
    Receiver::Ptr                   m_receiver;

};

}
