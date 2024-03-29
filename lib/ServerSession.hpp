#pragma once

#include <memory>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "MessageSender.hpp"
#include "MessageReceiver.hpp"
#include "Logger.hpp"
#include "CommandProcessor.hpp"
#include "Logger.hpp"

namespace kvdb
{

class ServerSession;
using ServerSessionPtr = std::shared_ptr<ServerSession>;

struct ServerSessionContext
{
    using InitCallback = std::function<void(const ServerSessionPtr&)>;
    using CloseCallback = std::function<void(const ServerSessionPtr&)>;

    boost::asio::io_context&        m_ioContext;
    Logger&                         m_logger;
    boost::asio::ip::tcp::acceptor& m_acceptor;
    CommandProcessor&               m_processor;
    InitCallback                    m_initCallback;
    CloseCallback                   m_closeCallback;
};

/// @brief manages connection with one client
class ServerSession
        : private ServerSessionContext
        , public std::enable_shared_from_this<ServerSession>
{
public:
    using Ptr = std::shared_ptr<ServerSession>;

    explicit ServerSession(const ServerSessionContext& context);

    virtual ~ServerSession();

    static void Init(const ServerSessionContext& context);

    std::string Address() const;

private:
    using Sender = MessageSender<ResultMessage>;
    using Receiver = MessageReceiver<CommandMessage>;

    void onConnectionAccepted(const boost::system::error_code& error);
    void onCommandReceived(const CommandMessage& command);
    void onConnectionClosed();

    boost::asio::io_context::strand m_strand;
    boost::asio::ip::tcp::socket    m_socket;
    Sender::Ptr                     m_sender;
    Receiver::Ptr                   m_receiver;
};

} // namespace kvdb
