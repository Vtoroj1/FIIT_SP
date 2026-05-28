#include <not_implemented.h>
#include "../include/allocator_sorted_list.h"
#include <mutex>
#include <new>
#include <stdexcept>
#include <cstring>

namespace {
    struct allocator_data
    {
        std::pmr::memory_resource* parent;
        allocator_with_fit_mode::fit_mode mode; 
        size_t total;
        std::mutex mtx;
        void* first_free;
    };

    struct block_header {
        size_t size;
        void* next_or_parent;
    };    
};

allocator_sorted_list::~allocator_sorted_list()
{
    if (_trusted_memory == nullptr) return;

    allocator_data* data = reinterpret_cast<allocator_data*>(_trusted_memory);
    std::pmr::memory_resource* parent = data->parent;
    size_t total = data->total;
    data->~allocator_data();

    if (parent != nullptr) {
        parent->deallocate(_trusted_memory, total);
    } else {
        ::operator delete(_trusted_memory);
    }

    _trusted_memory = nullptr;
}

allocator_sorted_list::allocator_sorted_list(
    allocator_sorted_list &&other) noexcept
{
    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;
}

allocator_sorted_list &allocator_sorted_list::operator=(
    allocator_sorted_list &&other) noexcept
{
    if (this != &other) {
        this->~allocator_sorted_list();
        _trusted_memory = other._trusted_memory;
        other._trusted_memory = nullptr;
    }
    return *this;
}

allocator_sorted_list::allocator_sorted_list(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    size_t header_size = sizeof(allocator_data);
    size_t block_size = sizeof(block_header);

    if (space_size < block_size) {
        space_size = block_size;
    }

    size_t total_needed = header_size + space_size;

    if (parent_allocator != nullptr) {
        _trusted_memory = parent_allocator->allocate(total_needed);
    } else {
        _trusted_memory = ::operator new(total_needed); 
    }

    allocator_data* data = new (_trusted_memory) allocator_data();

    data->parent = parent_allocator;
    data->mode = allocate_fit_mode; 
    data->total = total_needed;

    void* first_block_address = reinterpret_cast<char*>(_trusted_memory) + header_size;
    data->first_free = first_block_address;

    block_header* first_block = new (first_block_address) block_header();
    first_block->size = space_size;
    first_block->next_or_parent = nullptr;
}

[[nodiscard]] void *allocator_sorted_list::do_allocate_sm(
    size_t size)
{
    if (_trusted_memory == nullptr) {
        throw std::bad_alloc();
    }

    allocator_data* data = reinterpret_cast<allocator_data*>(_trusted_memory);
    std::lock_guard<std::mutex> lock(data->mtx);

    size_t alignment = alignof(std::max_align_t);
    size_t payload_size = (size + alignment - 1) & ~(alignment - 1);

    size_t header_size = sizeof(block_header);
    size_t total_block_size = payload_size + header_size;

    block_header* prev = nullptr;
    block_header* curr = reinterpret_cast<block_header*>(data->first_free);

    block_header* best_prev = nullptr;
    block_header* best_curr = nullptr;

    while (curr != nullptr) {
        if (curr->size >= total_block_size) {
            if (data->mode == allocator_with_fit_mode::fit_mode::first_fit) {
                best_curr = curr;
                best_prev = prev;
                break;
            } else if (data->mode == allocator_with_fit_mode::fit_mode::the_best_fit) {
                if (best_curr == nullptr || curr->size < best_curr->size) {
                    best_curr = curr;
                    best_prev = prev;
                }
            } else if (data->mode == allocator_with_fit_mode::fit_mode::the_worst_fit) {
                if (best_curr == nullptr || curr->size > best_curr->size) {
                    best_curr = curr;
                    best_prev = prev;
                }
            }
        }
        prev = curr;
        curr = reinterpret_cast<block_header*>(curr->next_or_parent);
    }

    if (best_curr == nullptr) {
        throw std::bad_alloc();
    }

    if (best_curr->size >= total_block_size + header_size + alignment) {
        void* new_free_addr = reinterpret_cast<char*>(best_curr) + total_block_size;

        block_header* new_free = new(new_free_addr) block_header();
        
        new_free->size = best_curr->size - total_block_size;
        
        new_free->next_or_parent = best_curr->next_or_parent;

        if (best_prev == nullptr) {
            data->first_free = new_free;
        } else {
            best_prev->next_or_parent = new_free;
        }

        best_curr->size = total_block_size;
    } else {
        if (best_prev == nullptr) {
            data->first_free = best_curr->next_or_parent;
        } else {
            best_prev->next_or_parent = best_curr->next_or_parent;
        }
    }

    best_curr->next_or_parent = _trusted_memory;

    return reinterpret_cast<char*>(best_curr) + header_size;
}

allocator_sorted_list::allocator_sorted_list(const allocator_sorted_list& other) {
    if (!other._trusted_memory) {
        _trusted_memory = nullptr;
        return;
    }

    allocator_data* other_data = reinterpret_cast<allocator_data*>(other._trusted_memory);
    std::lock_guard<std::mutex> lock(other_data->mtx);

    size_t total_size = other_data->total;
    std::pmr::memory_resource* parent = other_data->parent;

    if (parent)
        _trusted_memory = parent->allocate(total_size);
    else
        _trusted_memory = ::operator new(total_size);

    allocator_data* new_data = new (_trusted_memory) allocator_data();
    new_data->parent = parent;
    new_data->mode = other_data->mode;
    new_data->total = total_size;
    new_data->first_free = nullptr;

    size_t meta_size = sizeof(allocator_data);
    void* src_blocks = reinterpret_cast<char*>(other._trusted_memory) + meta_size;
    void* dst_blocks = reinterpret_cast<char*>(_trusted_memory) + meta_size;
    size_t blocks_size = total_size - meta_size;
    std::memcpy(dst_blocks, src_blocks, blocks_size);

    std::ptrdiff_t delta = reinterpret_cast<char*>(_trusted_memory) - reinterpret_cast<char*>(other._trusted_memory);

    if (other_data->first_free) {
        new_data->first_free = reinterpret_cast<char*>(other_data->first_free) + delta;
    }

    char* current = static_cast<char*>(dst_blocks);
    char* end = current + blocks_size;

    while (current < end) {
        block_header* blk = reinterpret_cast<block_header*>(current);

        if (blk->next_or_parent == other._trusted_memory) {
            blk->next_or_parent = _trusted_memory;
        } else {
            if (blk->next_or_parent != nullptr) {
                blk->next_or_parent = reinterpret_cast<char*>(blk->next_or_parent) + delta;
            }
        }

        current += blk->size;
        if (current > end) break;
    }
}

allocator_sorted_list &allocator_sorted_list::operator=(const allocator_sorted_list &other)
{
    if (this != &other) {
        allocator_sorted_list temp(other);
        *this = std::move(temp);
    }
    return *this;
}

bool allocator_sorted_list::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    auto p = dynamic_cast<const allocator_sorted_list*>(&other);
    return p != nullptr && p->_trusted_memory == _trusted_memory;
}

void allocator_sorted_list::do_deallocate_sm(
    void *at)
{
    if (at == nullptr) return;

    allocator_data* data = reinterpret_cast<allocator_data*>(_trusted_memory);
    std::lock_guard<std::mutex> loc(data->mtx);

    size_t header_size = sizeof(block_header);
    block_header* block_to_free = reinterpret_cast<block_header*>(reinterpret_cast<char*>(at) - header_size);

    if (block_to_free->next_or_parent != _trusted_memory) {
        throw std::logic_error("Попытка освободить чужую память или уже свободный блок!");
    }

    block_header* prev = nullptr;
    block_header* curr = reinterpret_cast<block_header*>(data->first_free);

    while (curr != nullptr && curr < block_to_free) {
        prev = curr;
        curr = reinterpret_cast<block_header*>(curr->next_or_parent);
    }

    block_to_free->next_or_parent = curr;
    if (prev == nullptr) {
        data->first_free = block_to_free;
    } else {
        prev->next_or_parent = block_to_free; 
    }

    if (curr != nullptr) {
        char* end_of_block = reinterpret_cast<char*>(block_to_free) + block_to_free->size;
        
        if (end_of_block == reinterpret_cast<char*>(curr)) {
            block_to_free->size += curr->size;
            block_to_free->next_or_parent = curr->next_or_parent;
        }
    }

    if (prev != nullptr) {
        char* end_of_block = reinterpret_cast<char*>(prev) + prev->size;

        if (end_of_block == reinterpret_cast<char*>(block_to_free)) {
            prev->size += block_to_free->size;
            prev->next_or_parent = block_to_free->next_or_parent;
        }
    }
}

inline void allocator_sorted_list::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    if (_trusted_memory == nullptr) return;
    allocator_data* data = reinterpret_cast<allocator_data*>(_trusted_memory);
    std::lock_guard<std::mutex> lock(data->mtx);
    data->mode = mode;
}

std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info() const noexcept
{
    if (_trusted_memory == nullptr) return {};
    allocator_data* data = reinterpret_cast<allocator_data*>(_trusted_memory);
    
    std::lock_guard<std::mutex> lock(data->mtx); 
    return get_blocks_info_inner();
}


std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info_inner() const
{
    std::vector<allocator_test_utils::block_info> info;
    
    for (auto it = begin(); it != end(); ++it) {
        allocator_test_utils::block_info b;
        b.block_size = it.size(); 
        b.is_block_occupied = it.occupied(); 
        info.push_back(b);
    }
    return info;
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::free_begin() const noexcept
{
    return sorted_free_iterator(_trusted_memory);
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::free_end() const noexcept
{
    return sorted_free_iterator(nullptr);
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::begin() const noexcept
{
    return sorted_iterator(_trusted_memory);
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::end() const noexcept
{
    return sorted_iterator(nullptr);
}


bool allocator_sorted_list::sorted_free_iterator::operator==(
        const allocator_sorted_list::sorted_free_iterator & other) const noexcept
{
    return _free_ptr == other._free_ptr;
}

bool allocator_sorted_list::sorted_free_iterator::operator!=(
        const allocator_sorted_list::sorted_free_iterator &other) const noexcept
{
    return _free_ptr != other._free_ptr;
}

allocator_sorted_list::sorted_free_iterator &allocator_sorted_list::sorted_free_iterator::operator++() & noexcept
{
    if (_free_ptr != nullptr) {
        _free_ptr = reinterpret_cast<block_header*>(_free_ptr)->next_or_parent;
    }
    return *this;
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::sorted_free_iterator::operator++(int n)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

size_t allocator_sorted_list::sorted_free_iterator::size() const noexcept
{
    return reinterpret_cast<block_header*>(_free_ptr)->size;
}

void *allocator_sorted_list::sorted_free_iterator::operator*() const noexcept
{
    return reinterpret_cast<char*>(_free_ptr) + sizeof(block_header);
}

allocator_sorted_list::sorted_free_iterator::sorted_free_iterator() : _free_ptr(nullptr) {}

allocator_sorted_list::sorted_free_iterator::sorted_free_iterator(void *trusted)
{
    if (trusted != nullptr) {
        allocator_data* data = reinterpret_cast<allocator_data*>(trusted);
        _free_ptr = data->first_free;
    } else {
        _free_ptr = nullptr;
    }
}

bool allocator_sorted_list::sorted_iterator::operator==(const allocator_sorted_list::sorted_iterator & other) const noexcept
{
    return _current_ptr == other._current_ptr;
}

bool allocator_sorted_list::sorted_iterator::operator!=(const allocator_sorted_list::sorted_iterator &other) const noexcept
{
    return _current_ptr != other._current_ptr;
}

allocator_sorted_list::sorted_iterator &allocator_sorted_list::sorted_iterator::operator++() & noexcept
{
    if (_current_ptr != nullptr) {
        block_header* block = reinterpret_cast<block_header*>(_current_ptr);
        _current_ptr = reinterpret_cast<char*>(_current_ptr) + block->size;

        allocator_data* data = reinterpret_cast<allocator_data*>(_trusted_memory);
        void* end_of_memory = reinterpret_cast<char*>(_trusted_memory) + data->total;

        if (_current_ptr >= end_of_memory) { 
            _current_ptr = nullptr;
        }
    }

    return *this;
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::sorted_iterator::operator++(int n)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

size_t allocator_sorted_list::sorted_iterator::size() const noexcept
{
    return reinterpret_cast<block_header*>(_current_ptr)->size;
}

void *allocator_sorted_list::sorted_iterator::operator*() const noexcept
{
    return reinterpret_cast<char*>(_current_ptr) + sizeof(block_header);
}

allocator_sorted_list::sorted_iterator::sorted_iterator() : _free_ptr(nullptr), _current_ptr(nullptr), _trusted_memory(nullptr) {}

allocator_sorted_list::sorted_iterator::sorted_iterator(void *trusted) : _trusted_memory(trusted)
{
    if (_trusted_memory != nullptr) {
        _current_ptr = reinterpret_cast<char*>(_trusted_memory) + sizeof(allocator_data);
    } else {
        _current_ptr = nullptr;
    }
}

bool allocator_sorted_list::sorted_iterator::occupied() const noexcept
{
    return reinterpret_cast<block_header*>(_current_ptr)->next_or_parent == _trusted_memory;
}
