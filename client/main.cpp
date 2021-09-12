#include <thread>
#include <chrono>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/log/sinks.hpp>
#include <boost/core/null_deleter.hpp>

#include "../lib/ClientSession.hpp"

namespace kvdb
{

class ClientApp
{
public:
    static constexpr char scArgHostname[] = "hostname";
    static constexpr char scArgPort[] = "port";
    static constexpr char scArgCommand[] = "command";
    static constexpr int scDefaultPort = 1524;

    ClientApp(int argc, char** argv)
    {
        ///-----------------------------------------------------------------------------------------
        /// Configure logger

        {
            using namespace boost::log;

            using text_sink = sinks::asynchronous_sink<sinks::text_ostream_backend>;
            boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();

            boost::shared_ptr<std::ostream> stream{ &std::clog, boost::null_deleter{} };
            sink->locked_backend()->add_stream(stream);
            core::get()->add_sink(sink);
        }

        ///-----------------------------------------------------------------------------------------
        /// Configure application arguments
        using namespace boost::program_options;

        options_description desc("KVDB command line interface");
        desc.add_options()
                (scArgHostname, value<std::string>(),
                 "[required] name/address of the KVDB host to connect to")
                (scArgPort, value<int>()->default_value(scDefaultPort),
                 "[required] port of the KVDB host to connect to")
                (scArgCommand, value<std::vector<std::string>>(),
                 "[required] command to execute");

        positional_options_description posDesc;
        posDesc.add(scArgCommand, 3);

        try
        {
            store(command_line_parser(argc, argv)
                        .options(desc)
                        .positional(posDesc)
                        .run(),
                  m_varMap);
            notify(m_varMap);
        }
        catch (boost::program_options::error& e)
        {
            m_logger.LogRecord(std::string("Error while parse comand line arguments: ") + e.what());
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            exit(-1);
        }

        if (m_varMap.count(scArgHostname) == 0)
        {
            m_logger.LogRecord("host is required");
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            exit(-1);
        }

        if (m_varMap.count(scArgCommand) == 0)
        {
            m_logger.LogRecord("command is required");
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            exit(-1);
        }

        const auto command = m_varMap[scArgCommand].as<std::vector<std::string>>();
        if (!parseCommand(command, m_command))
        {
            m_logger.LogRecord("Unable to parse command");
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            exit(-1);
        }

        ///-----------------------------------------------------------------------------------------
        /// Initializing client session


        m_session = std::make_shared<ClientSession>(ClientSessionContext {
                                                        m_ioContext,
                                                        m_logger,
                                                        std::bind(&ClientApp::onConnect, this,
                                                                  std::placeholders::_1),
                                                        std::bind(&ClientApp::onClose, this)
                                                    });
    }

    void Start()
    {
        m_logger.LogRecord("=================================================\n"
                           "Starting client...");

        try
        {
            boost::asio::signal_set signals(m_ioContext, SIGINT, SIGTERM);
            signals.async_wait(std::bind(&ClientApp::onSystemSignal, this,
                                         std::placeholders::_1,
                                         std::placeholders::_2));
            m_session->Connect(m_varMap[scArgHostname].as<std::string>(),
                               m_varMap[scArgPort].as<int>());
            m_ioContext.run();
        }
        catch (std::exception& e)
        {
            m_logger.LogRecord((boost::format("Exception: %1%") % e.what()).str());
            exit(-1);
        }
    }

private:
    bool parseCommand(const std::vector<std::string>& command, CommandMessage& msg)
    {
        static const int scOperationIdx = 0;
        static const int scKeyIdx = 1;
        static const int scValueIdx = 2;

        const auto operation = command[scOperationIdx];
        if (operation == "INSERT")
        {
            if (command.size() != 3)
            {
                m_logger.LogRecord("INSERT requires 2 arguments separated by space: INSERT <key> <value>");
                return false;
            }

            msg.type = CommandMessage::INSERT;
            msg.key.Set(command[scKeyIdx]);
            msg.value.Set(command[scValueIdx]);
        }
        else if (operation == "UPDATE")
        {
            if (command.size() != 3)
            {
                m_logger.LogRecord("UPDATE requires 2 arguments separated by space: UPDATE <key> <value>");
                return false;
            }

            msg.type = CommandMessage::UPDATE;
            msg.key.Set(command[scKeyIdx]);
            msg.value.Set(command[scValueIdx]);
        }
        else if (operation == "GET")
        {
            if (command.size() != 2)
            {
                m_logger.LogRecord("GET requires 1 argument: GET <key>");
                return false;
            }

            msg.type = CommandMessage::GET;
            msg.key.Set(command[scKeyIdx]);
            msg.value.Set(std::string());
        }
        else if (operation == "DELETE")
        {
            if (command.size() != 2)
            {
                m_logger.LogRecord("DELETE requires 1 argument: DELETE <key>");
                return false;
            }

            msg.type = CommandMessage::DELETE;
            msg.key.Set(command[scKeyIdx]);
            msg.value.Set(std::string());
        }
        else
        {
            m_logger.LogRecord(std::string("Unknown operation : ") + operation);
            return false;
        }

        return true;
    }

    void onConnect(bool success)
    {
        if (!success)
        {
            throw std::runtime_error("Failed to connect to server. Exiting...");
        }
        else
        {
            m_logger.LogRecord("ClientSession connected!");
            m_session->SendCommand(m_command,
                                   std::bind(&ClientApp::onResultReceived, this,
                                             std::placeholders::_1,
                                             std::placeholders::_2));
        }
    }

    void onResultReceived(bool success, const std::string& value)
    {
        if (!success)
        {
            throw std::runtime_error("Server failed to execute command");
        }
        else if (!value.empty())
        {
            std::cout << value;
        }

        m_ioContext.stop();
    }

    void onClose()
    {
        m_logger.LogRecord("Connection closed. Exiting...");
        m_ioContext.stop();
    }

    void onSystemSignal(const boost::system::error_code& error, int signalNumber)
    {
        if (!error)
        {
            m_logger.LogRecord((boost::format("Signal %1% occured") % signalNumber).str());
            if (signalNumber == SIGINT || signalNumber == SIGTERM)
            {
                m_logger.LogRecord("Terminating...");
                m_ioContext.stop();
            }

            return;
        }

        throw std::runtime_error((boost::format("Error occured while waiting for system signal: %1%")
                                  % error.message()).str());
    }

    boost::program_options::variables_map   m_varMap;
    boost::asio::io_context m_ioContext;
    Logger                  m_logger;
    ClientSession::Ptr      m_session;
    CommandMessage          m_command;
};

} // namespace kvdb

int main(int argc, char** argv)
{
    kvdb::ClientApp app(argc, argv);
    app.Start();
}
