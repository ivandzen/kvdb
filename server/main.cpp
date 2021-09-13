
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <boost/log/sinks.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/format.hpp>

#include "../lib/Logger.hpp"
#include "../lib/Server.hpp"
#include "../lib/Application.hpp"

namespace kvdb
{

class ServerApp
        : public Application
{
public:
    static const uint32_t scReportingIntervalSec = 60;

    ServerApp(int argc, char** argv)
        : m_map(m_logger)
        , m_commandProcessor(CommandProcessorContext {
                             m_ioContext,
                             m_logger,
                             m_map,
                             scReportingIntervalSec})
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
            m_logger.LogRecord(std::string("Error while parse comand line arguments: ") + e.what());
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            exit(-1);
        }

        if (vm.count(scArgFile) == 0)
        {
            m_logger.LogRecord("file is required");
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            exit(-1);
        }

        m_map.InitStorage(vm[scArgFile].as<std::string>());

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

    void Run()
    {
        const auto numThreads = std::thread::hardware_concurrency();
        m_logger.LogRecord(std::string("Starting KVDB Server. Num threads : ") + std::to_string(numThreads));
        m_commandProcessor.Start();
        m_server->Start();
        Application::Run(numThreads);
    }

private:
    // all fields must be in the order of initialization
    PersistableMap          m_map;
    CommandProcessor        m_commandProcessor;
    Server::Ptr             m_server;
};

}

int main(int argc, char** argv)
{
    kvdb::ServerApp app(argc, argv);
    app.Run();
    return 0;
}
