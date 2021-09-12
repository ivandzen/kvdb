#include <string>
#include <iostream>

#include <boost/asio.hpp>

#include "../lib/Protocol.hpp"
#include "../lib/Serialization.hpp"


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

    {
        using namespace kvdb;

        Serialize(comIn, ostream);
        Deserialize(istream, comOut);
    }

    std::cout << sbuf.size() << "\n";
    std::cout << "From sbuf: " << std::string((char*)sbuf.data().begin()->data()) << "\n";
    std::cout << comOut.key.Get() << ":" << comOut.value.Get() << "\n";

    assert(comIn == comOut);
}

void testResultMessageDeSerialize()
{
    kvdb::ResultMessage resIn(0, "HELL  jklk O");

    kvdb::ResultMessage resOut;

    boost::asio::streambuf sbuf(500);
    std::ostream ostream(&sbuf);
    std::istream istream(&sbuf);

    {
        using namespace kvdb;

        Serialize(resIn, ostream);
        Deserialize(istream, resOut);
    }

    std::cout << sbuf.size() << "\n";
    std::cout << "From sbuf: " << std::string((char*)sbuf.data().begin()->data()) << "\n";
    std::cout << "code " << resOut.code << " " << resOut.value.Get() << "\n";

    assert(resIn == resOut);
}


int main(int argc, char** argv)
{
    testCommandMessageDeSerialize();
    testResultMessageDeSerialize();
    return 0;
}
