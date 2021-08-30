#include <filesystem>
#include <iostream>

#include "PersistableMap.hpp"

namespace kvdb
{

static const char scMainObjectName[] = "Root";
static const std::size_t scDefaultMappedFileSize = 1024 * 1024 * 64;

PersistableMap::PersistableMap(const char* filePath)
{
    if (std::filesystem::exists(filePath))
    {
        if (!std::filesystem::is_regular_file(filePath))
        {
            throw std::runtime_error("Given path is not a regular file");
        }

        m_mappedFile = std::make_shared<MappedFile>(boost::interprocess::open_copy_on_write,
                                                    filePath);
    }
    else
    {
        m_mappedFile = std::make_shared<MappedFile>(boost::interprocess::create_only,
                                                    filePath,
                                                    scDefaultMappedFileSize);
    }

    m_allocator = std::make_unique<Allocator<char>>(m_mappedFile->get_segment_manager());


    m_internalStorage = m_mappedFile->find_or_construct<InternalStorage>(scMainObjectName)(*m_allocator);
}

PersistableMap::~PersistableMap()
{
    m_mappedFile->flush();
}

void PersistableMap::Insert(const std::string& key, const std::string& value)
{
    StringType keymapped(key.size(), *m_allocator);
    std::copy(key.begin(), key.end(), keymapped.begin());
    //int i = 0;
    //for (const auto& ch : key)
    //{
    //    keymapped.push_back(ch);
    //    std::cout << "-" << keymapped[i];
    //    ++i;
   // }

    std::cout << keymapped.data();

    StringType valueMapped(value.size(), *m_allocator);
    std::copy(value.begin(), value.end(), valueMapped.begin());

    auto& index = m_internalStorage->get<Entry::ByKey>();
    index.insert(Entry{ keymapped, valueMapped});
}

}// namespace kvdb
