#include "../common/PersistableMap.hpp"

int main(int argc, char** argv)
{
    kvdb::PersistableMap map("/home/ivan/persistablemap.map");
    map.Insert("HKJHLKJHLKJHLKJHLK", "KJLKJ:LKJ:LKJ:LK");
    return 0;
}
