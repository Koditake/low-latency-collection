#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include "macros.h"

namespace common 
{
    template<typename T>

    class MemoryPool final
    {
        public:
            explicit MemoryPool(std::size_t num_elems): store_ (num_elems, {T(), true})
            {
                ASSERT(reinterpret_cast<const ObjectBlock *>(&(store_[0].object_)) == &(store_[0]), "T object should be first member of ObjectBlock.");
            }

            MemoryPool() = delete;
            MemoryPool(const MemoryPool &) = delete;
            MemoryPool(const MemoryPool &&) = delete;
            MemoryPool &operator=(const MemoryPool &) = delete;
            MemoryPool &operator=(const MemoryPool &&) = delete;

            auto UpdateNextFreeIndex() noexcept
            {
                const auto initial_free_index = next_free_index_;

                while(!store_[next_free_index_].is_free_)
                {
                    next_free_index_++;
                    if (UNLIKELY(next_free_index_ == store_.size()))
                    {
                        next_free_index_ = 0;
                    }
                    if (UNLIKELY(initial_free_index == next_free_index_))
                    {
                        ASSERT(initial_free_index != next_free_index_, "Memory Pool is out of space.");
                    }
                }
            }

            template<typename... Args>
            T *allocate(Args... args) noexcept
            {
                auto obj_block = &(store_[next_free_index_]);
                ASSERT(obj_block->is_free_, "Expected free ObjectBlock at index: " + std::to_string(next_free_index_));

                T *ret = &(obj_block->object_);
                ret = new(ret) T(args...);
                obj_block->is_free_ = false;

                UpdateNextFreeIndex();
                return ret;
            }

            auto deallocate(const T *elem) noexcept 
            {
                const auto elem_index = (reinterpret_cast<const ObjectBlock *>(elem) - &store_[0]);
                ASSERT(elem_index >= 0 && static_cast<size_t>(elem_index) < store_.size(), "Element subjected to deallocation did not belong to this Memory Pool.");
                ASSERT(!store_[elem_index].is_free_, "Expected current issued ObjectBlock at index: " + std::to_string(elem_index));

                store_[elem_index].is_free_ = true;
            }

        private:
            struct ObjectBlock
            {
                T object_;
                bool is_free_ = true;
            };
            std::vector<ObjectBlock> store_;
            size_t next_free_index_ = 0;
    };
}