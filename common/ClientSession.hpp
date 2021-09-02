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
    using WeakPtr = std::weak_ptr<ClientSession>;

    explicit ClientSession(const ClientSessionContext& context);

    void Connect();

    void SendCommand(const CommandMessage& command);

private:
    void onEPResolved(const boost::system::error_code& error,
                      boost::asio::ip::tcp::resolver::results_type results);

    void onSocketConnected(const boost::system::error_code& ec);


    boost::asio::ip::tcp::resolver      m_resolver;
    boost::asio::ip::tcp::socket        m_socket;
    MessageSender<CommandMessage>::Ptr  m_sender;
    MessageReceiver<ResultMessage>::Ptr m_receiver;

};

}
