#pragma once

#include <string>
#include <memory>

#include <boost/interprocess/allocators/cached_node_allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

namespace kvdb
{

class PersistableMap
{
public:
    PersistableMap(const char* filePath);

    virtual ~PersistableMap();

    void Insert(const std::string& key, const std::string& value);

private:
    template<typename Type>
    using Allocator = boost::interprocess::allocator<Type, boost::interprocess::managed_mapped_file::segment_manager>;

    using AllocatorPtr = std::unique_ptr<Allocator<char>>;
    using MappedFile = boost::interprocess::managed_mapped_file;
    using MappedFilePtr = std::shared_ptr<MappedFile>;

    using StringType = std::vector<char, Allocator<char>>;

    struct Entry
    {
        StringType  key;
        StringType  value;

        struct ByKey{};
    };

    using InternalStorage =
        boost::multi_index_container<
            Entry,
            boost::multi_index::indexed_by<
                boost::multi_index::ordered_unique<
                    boost::multi_index::tag<Entry::ByKey>,
                    boost::multi_index::member<Entry,StringType,&Entry::key>
                >
            >,
            Allocator<Entry>>;

    using InternalStoragePtr = InternalStorage*;
    using InternalStorageAllocator = boost::multi_index::detail::rebind_alloc_for<Allocator<char>, PersistableMap>::type;

    MappedFilePtr       m_mappedFile;
    AllocatorPtr        m_allocator;
    InternalStoragePtr  m_internalStorage;
};


} // namespace kvdb
