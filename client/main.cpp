#include <iostream>
#include <vector>

#include <boost/asio/streambuf.hpp>

#include "../common/PersistableMap.hpp"
#include "../common/Protocol.hpp"
#include "../common/Serialization.hpp"

void testCommandMessageDeSerialize()
{
    kvdb::CommandMessage comIn{
        kvdb::CommandMessage::INSERT,
        "1.THIS\n\tIS A KEY",
        "2.AND THIS\n\tIS A VALUE",
    };

    kvdb::CommandMessage comOut;

    boost::asio::streambuf sbuf(1024);
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

    std::cout << "comIn: " << comIn << "\n";
    std::cout << "comOut: " << comOut << "\n";

    std::cout << comOut.key << ":" << comOut.value << "\n";

    assert(comIn == comOut);
}

int main(int argc, char** argv)
{
    testCommandMessageDeSerialize();

   // kvdb::PersistableMap map("/home/ivan/persistablemap.map");
   // map.Insert("HKJHLKJHLKJHLKJHLK", "KJLKJ:LKJ:LKJ:LK");
    return 0;
}
