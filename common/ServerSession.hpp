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

struct ServerSessionContext
{
    boost::asio::io_context&        m_ioContext;
    boost::asio::ip::tcp::acceptor& m_acceptor;
    CommandProcessor::Ptr           m_processor;
    Logger::Ptr                     m_logger;
};

class ServerSession
        : public ServerSessionContext
        , public std::enable_shared_from_this<ServerSession>
{
public:
    using Ptr = std::shared_ptr<ServerSession>;

    using InitCallback = std::function<void(const Ptr&)>;

    explicit ServerSession(const ServerSessionContext& context);

    static void StartAccept(const ServerSessionContext& context,
                            const InitCallback& initCallback);

private:
    using Sender = MessageSender<ResultMessage>;
    using Receiver = MessageReceiver<CommandMessage>;

    void onConnectionAccepted(const boost::system::error_code& error,
                              const InitCallback& initCallback);

    boost::asio::ip::tcp::socket    m_socket;
    Sender::Ptr                     m_sender;
    Receiver::Ptr                   m_receiver;
};

} // namespace kvdb
