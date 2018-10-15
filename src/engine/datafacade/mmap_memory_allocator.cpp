#include "engine/datafacade/mmap_memory_allocator.hpp"

#include "storage/block.hpp"
#include "storage/io.hpp"
#include "storage/serialization.hpp"
#include "storage/storage.hpp"

#include "util/log.hpp"
#include "util/mmap_file.hpp"

#include "boost/assert.hpp"

namespace osrm
{
namespace engine
{
namespace datafacade
{

MMapMemoryAllocator::MMapMemoryAllocator(const storage::StorageConfig &config)
{
    storage::Storage storage(config);
    std::vector<std::pair<bool, boost::filesystem::path>> files;
    storage.GetStaticFiles(&files);
    storage.GetUpdatableFiles(&files);

    std::vector<storage::SharedDataIndex::AllocatedRegion> allocated_regions;

    constexpr bool REQUIRED = true;

    for (const auto &file : files)
    {
        if (boost::filesystem::exists(file.second))
        {
            std::unique_ptr<storage::BaseDataLayout> layout =
                std::make_unique<storage::TarDataLayout>();
            boost::iostreams::mapped_file mapped_memory_file;
            util::mmapFile<char>(file.second, mapped_memory_file);
            mapped_memory_files.push_back(std::move(mapped_memory_file));
            storage.readBlocks(file.second, layout);
            allocated_regions.push_back({mapped_memory_file.data(), std::move(layout)});
        }
        else
        {
            if (file.first == REQUIRED)
            {
                throw util::exception("Could not find required filed: " +
                                      std::get<1>(file).string());
            }
        }
    }

    {
        // Figure out the path to the rtree file (it's not a tar file)
        auto absolute_file_index_path =
            boost::filesystem::absolute(config.GetPath(".osrm.fileIndex"));

        // Convert the boost::filesystem::path object into a plain string
        // that's stored as a member of this allocator object
        rtree_filename = absolute_file_index_path.string();

        std::unique_ptr<storage::BaseDataLayout> fake_layout =
            std::make_unique<storage::TarDataLayout>();

        // Here, we hardcode the special file_index_path block name.
        // The important bit here is that the "offset" is set to zero
        fake_layout->SetBlock("/common/rtree/file_index_path",
                              {rtree_filename.size(), rtree_filename.size() + 1, 0});

        // Now, we add one more AllocatedRegion, with it's start address as the start
        // of the rtree_filename string we've saved.  In the fake_layout, we've
        // stated that the data is at offset 0, which is where the string starts
        // at it's own memory address.
        // The syntax &(rtree_filename[0]) gets the memory address of the first char.
        // We can't use the convenient `.data()` or `.c_str()` methods, because
        // prior to C++17 (which we're not using), those return a `const char *`,
        // which isn't compatible with the `char *` that AllocatedRegion expects
        // for it's memory_ptr
        allocated_regions.push_back({&(rtree_filename[0]), std::move(fake_layout)});
    }

    index = storage::SharedDataIndex{std::move(allocated_regions)};
} // namespace datafacade

MMapMemoryAllocator::~MMapMemoryAllocator() {}

const storage::SharedDataIndex &MMapMemoryAllocator::GetIndex() { return index; }

} // namespace datafacade
} // namespace engine
} // namespace osrm
