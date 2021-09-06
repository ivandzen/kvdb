
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <boost/log/sinks.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/format.hpp>

#include "../common/Logger.hpp"
#include "../common/Server.hpp"


namespace kvdb
{

class ServerApp
{
public:
    ServerApp(int argc, char** argv)
    {
        static constexpr char scArgPort[] = "port";
        static constexpr char scArgFile[] = "file";
        static constexpr int scDefaultPort = 1524;
        static const std::string scMappedFile = "./memfile.map";

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

        m_logger = std::make_shared<kvdb::Logger>();

        ///-----------------------------------------------------------------------------------------
        /// Configure application arguments
        using namespace boost::program_options;

        options_description desc("KVDB server");
        desc.add_options()
                (scArgPort, value<int>()->default_value(scDefaultPort),
                 "[required] port")
                (scArgFile, value<std::string>()->default_value(scMappedFile),
                 "[required] memory mapped file path");

        variables_map vm;
        try
        {
            store(parse_command_line(argc, argv, desc), vm);
            notify(vm);
        }
        catch (boost::program_options::error& e)
        {
            m_logger->LogRecord(std::string("Error while parse comand line arguments: ") + e.what());
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            exit(-1);
        }

        if (vm.count(scArgFile) == 0)
        {
            m_logger->LogRecord("file is required");
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            exit(-1);
        }

        m_map = std::make_shared<PersistableMap>(vm[scArgFile].as<std::string>().c_str());
        m_commandProcessor = std::make_shared<CommandProcessor>(CommandProcessorContext {
                                                                    m_ioContext,
                                                                    m_logger,
                                                                    m_map,
                                                                    60 // seconds
                                                                });

        {
            using namespace boost::asio::ip;

            const tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), vm[scArgPort].as<int>());
            m_server = std::make_shared<Server>(ServerContext {
                                                    m_ioContext,
                                                    m_logger,
                                                    m_commandProcessor,
                                                    endpoint
                                                });
        }
    }

    void Start()
    {
        m_logger->LogRecord("Starting server...");

        try
        {
            boost::asio::signal_set signals(m_ioContext, SIGINT, SIGTERM);
            signals.async_wait(std::bind(&ServerApp::onSystemSignal, this,
                                         std::placeholders::_1,
                                         std::placeholders::_2));

            m_commandProcessor->Start();
            m_server->Start();
            m_ioContext.run();
        }
        catch (std::exception& e)
        {
            m_logger->LogRecord((boost::format("Exception: %1%") % e.what()).str());
        }
    }

private:
    void onSystemSignal(const boost::system::error_code& error, int signalNumber)
    {
        if (!error)
        {
            m_logger->LogRecord((boost::format("Signal %1% occured") % signalNumber).str());
            if (signalNumber == SIGINT || signalNumber == SIGTERM)
            {
                m_logger->LogRecord("Terminating...");
                m_ioContext.stop();
            }

            return;
        }

        m_logger->LogRecord((boost::format("Error occured while waiting for system signal: %1%")
                             % error.message()).str());
        exit(1);
    }

    boost::asio::io_context m_ioContext;
    kvdb::Logger::Ptr       m_logger;
    PersistableMap::Ptr     m_map;
    CommandProcessor::Ptr   m_commandProcessor;
    Server::Ptr             m_server;
};

}

int main(int argc, char** argv)
{
    kvdb::ServerApp app(argc, argv);
    app.Start();
    return 0;
}
