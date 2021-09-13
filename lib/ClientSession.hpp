#pragma once

#include <memory>
#include <functional>
#include <map>

#include <boost/asio.hpp>

#include "MessageSender.hpp"
#include "MessageReceiver.hpp"
#include "Logger.hpp"

namespace kvdb
{

struct ClientSessionContext
{
    using ConnectCallback = std::function<void(bool)>;
    using CloseCallback = std::function<void(void)>;

    boost::asio::io_context&    m_ioContext;
    Logger&                     m_logger;
    ConnectCallback             m_connectCallback;
    CloseCallback               m_onCloseCallback;
};

/// @brief manages client connection
class ClientSession
        : private ClientSessionContext
{
public:
    using Ptr = std::shared_ptr<ClientSession>;
    using WeakPtr = std::weak_ptr<ClientSession>;

    /// @brief callback type for command execution result
    /// 1st arg - true if executiion was successfull, false otherwise
    /// 2nd arg - optional string with execution result data (string)
    using ResultCallback = std::function<void(bool, const std::string&)>;

    explicit ClientSession(const ClientSessionContext& context);

    virtual ~ClientSession();

    void Connect(const std::string& hostname, int port);

    void SendCommand(const CommandMessage& command,
                     const ResultCallback& callback);

private:
    using Sender = MessageSender<CommandMessage>;
    using Receiver = MessageReceiver<ResultMessage>;

    void onEPResolved(const boost::system::error_code& error,
                      boost::asio::ip::tcp::resolver::results_type results);

    void onSocketConnected(const boost::system::error_code& ec);

    void onResultReceived(const ResultMessage& result);

    boost::asio::io_context::strand m_strand;
    boost::asio::ip::tcp::resolver  m_resolver;
    boost::asio::ip::tcp::socket    m_socket;
    Sender::Ptr                     m_sender;
    Receiver::Ptr                   m_receiver;
    std::map<CommandID, ResultCallback> m_resultCallbacks;
};

}
