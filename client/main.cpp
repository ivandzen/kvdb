#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

#include <boost/program_options.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/format.hpp>
#include <boost/log/sinks.hpp>
#include <boost/core/null_deleter.hpp>

#include "../common/PersistableMap.hpp"
#include "../common/Protocol.hpp"
#include "../common/Serialization.hpp"


#include "../common/ClientSession.hpp"

void testCommandMessageDeSerialize()
{
    kvdb::CommandMessage comIn(kvdb::CommandMessage::INSERT);

    std::string key;
    key.resize(1024);
    std::fill(key.begin(), key.end(), 'a');
    comIn.key.Set(key);

    std::string value;
    value.resize(10240);
    std::fill(value.begin(), value.end(), 'a');
    comIn.value.Set(value);

    kvdb::CommandMessage comOut;

    boost::asio::streambuf sbuf(101240);
    std::ostream ostream(&sbuf);
    std::istream istream(&sbuf);

    static const char scOpen = '(';
    static const char scDelimiter = ',';
    static const char scClose = ')';

    std::cout << boost::fusion::tuple_open(scOpen)
              << boost::fusion::tuple_delimiter(scDelimiter)
              << boost::fusion::tuple_close(scClose);

    {
        using namespace kvdb;

        Serialize(comIn, ostream);
        Deserialize(istream, comOut);
    }

    std::cout << sbuf.size() << "\n";
    std::cout << "From sbuf: " << std::string((char*)sbuf.data().begin()->data()) << "\n";

    //std::cout << "comIn: " << comIn << "\n";
    //std::cout << "comOut: " << comOut << "\n";

    std::cout << comOut.key.Get() << ":" << comOut.value.Get() << "\n";

    assert(comIn == comOut);
}

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

        m_logger = std::make_shared<kvdb::Logger>();

        ///-----------------------------------------------------------------------------------------
        /// Configure application arguments
        using namespace boost::program_options;

        options_description desc("KVDB command line interface");
        desc.add_options()
                (scArgHostname, value<std::string>(),
                 "[required] name/address of the KVDB host to connect to")
                (scArgPort, value<int>()->default_value(scDefaultPort),
                 "[required] port of the KVDB host to connect to")
                (scArgCommand, value<std::string>(),
                 "[required] command to execute");

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

        if (vm.count(scArgHostname) == 0)
        {
            m_logger->LogRecord("host is required");
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            exit(-1);
        }

        if (vm.count(scArgCommand) == 0)
        {
            m_logger->LogRecord("command is required");
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            exit(-1);
        }

        ///-----------------------------------------------------------------------------------------
        /// Initializing client session
        kvdb::ClientSessionContext sessionContext =
        {
            m_ioContext,
            m_logger,
            std::bind(&ClientApp::onConnect, this, std::placeholders::_1),
            vm[scArgHostname].as<std::string>(),
            vm[scArgPort].as<int>()
        };

        m_session = std::make_shared<kvdb::ClientSession>(sessionContext);
    }

    void Start()
    {
        try
        {
            boost::asio::signal_set signals(m_ioContext, SIGINT, SIGTERM);
            signals.async_wait(std::bind(&ClientApp::onSystemSignal, this,
                                         std::placeholders::_1,
                                         std::placeholders::_2));
            m_session->Connect();
            m_ioContext.run();
        }
        catch (std::exception& e)
        {
            m_logger->LogRecord((boost::format("Exception: %1%") % e.what()).str());
        }
    }

private:
    void onConnect(bool success)
    {
        if (!success)
        {
            m_logger->LogRecord("Failed to connect to server. Exiting...");
            m_ioContext.stop();
        }
        else
        {
            m_logger->LogRecord("ClientSession connected!");
        }
    }

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

        m_logger->LogRecord((boost::format("Error occured while waiting for system signal: %1%") % error.message()).str());
        exit(1);
    }

    boost::asio::io_context     m_ioContext;
    kvdb::Logger::Ptr           m_logger;
    kvdb::ClientSession::Ptr    m_session;
};

int main(int argc, char** argv)
{
    ClientApp app(argc, argv);
    app.Start();
}
