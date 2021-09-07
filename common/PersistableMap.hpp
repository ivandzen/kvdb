#pragma once

#include <string>
#include <memory>
#include <shared_mutex>

#include <boost/interprocess/allocators/cached_node_allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/interprocess/containers/string.hpp>

namespace kvdb
{

/// @brief Maps unique key (std::string) to value(std::string)
/// Uses memory mapped file to store it's content
/// All public methods can be used concurrently from different threads,
/// all of them are blocking
class PersistableMap
{
public:
    using Ptr = std::shared_ptr<PersistableMap>;
    using SegmentManager = boost::interprocess::managed_mapped_file::segment_manager;
    using Millis = std::chrono::milliseconds;

    struct Stat
    {
        SegmentManager::size_type m_size;
        SegmentManager::size_type m_free;
    };

    PersistableMap(const char* filePath);

    virtual ~PersistableMap();

    void Insert(const std::string& key, const std::string& value, const Millis& lockTout);
    void Update(const std::string& key, const std::string& value, const Millis& lockTout);
    void Get(const std::string& key, std::string& output, const Millis& lockTout) const;
    void Delete(const std::string& key, const Millis& lockTout);

    Stat GetStat()
    {
        return Stat
        {
            m_mappedFile->get_segment_manager()->get_size(),
            m_mappedFile->get_segment_manager()->get_free_memory()
        };
    }

private:
    template<typename Type>
    using Allocator = boost::interprocess::allocator<Type, SegmentManager>;

    using AllocatorPtr = std::unique_ptr<Allocator<void>>;
    using MappedFile = boost::interprocess::managed_mapped_file;
    using MappedFilePtr = std::shared_ptr<MappedFile>;
    using StringType = boost::interprocess::basic_string<char, std::char_traits<char>, Allocator<char>>;

    struct Entry
    {
        StringType  key;
        StringType  value;

        explicit Entry(Allocator<void> a)
            : key(a)
            , value(a)
        {
        }

        Entry(const std::string& key, const std::string& value, Allocator<void> a)
            : key(key, a)
            , value(value, a)
        {}

        struct ByKey{};
    };

    struct StringTypeHash
    {
      std::size_t operator()(const std::basic_string<char>& value) const
      {
          return boost::hash<std::string>()(std::string(value));
      }
    };

    struct StringTypeEqual
    {
        std::size_t operator()(const std::basic_string<char>& str1, const StringType& str2) const
        {
            return std::equal(str1.begin(), str1.end(), str2.begin(), str2.end());
        }
    };

    using InternalStorage =
        boost::multi_index_container<
            Entry,
            boost::multi_index::indexed_by<
                boost::multi_index::hashed_unique<
                    boost::multi_index::tag<Entry::ByKey>,
                    boost::multi_index::member<Entry,StringType,&Entry::key>,
                    StringTypeHash,
                    StringTypeEqual
                >
            >,
            Allocator<Entry>>;

    using InternalStoragePtr = InternalStorage*;
    using InternalStorageAllocator = boost::multi_index::detail::rebind_alloc_for<Allocator<char>, PersistableMap>::type;

    MappedFilePtr               m_mappedFile;
    AllocatorPtr                m_allocator;
    InternalStoragePtr          m_internalStorage;
    mutable std::shared_timed_mutex m_mutex;
};


} // namespace kvdb
