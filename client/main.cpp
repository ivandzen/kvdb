#include <sstream>
#include <iostream>

#include <boost/asio/streambuf.hpp>

#include "../common/PersistableMap.hpp"
#include "../common/Protocol.hpp"

int main(int argc, char** argv)
{
    kvdb::CommandMessage com1{
        kvdb::CommandMessage::INSERT,
        "KEY",
        "VALUE"
    };

    boost::asio::streambuf sbuf(1024);

    std::ostream ostream(&sbuf);
    ostream << com1;

    kvdb::CommandMessage com2;

    std::cout << "com2 before = " << com2 << "\n";

    std::istream istream(&sbuf);
    istream >> com2;

    std::cout << "com1 = " << com1 << "\n";
    std::cout << "com2 after = " << com2 << "\n";
    std::cout << "com2 after = " << com2;

    return 0;

    assert(com1 == com2);

   // kvdb::PersistableMap map("/home/ivan/persistablemap.map");
   // map.Insert("HKJHLKJHLKJHLKJHLK", "KJLKJ:LKJ:LKJ:LK");
    return 0;
}
