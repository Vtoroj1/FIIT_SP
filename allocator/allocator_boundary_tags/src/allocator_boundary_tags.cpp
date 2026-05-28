#include <not_implemented.h>
#include "../include/allocator_boundary_tags.h"
#include <new>
#include <stdexcept>
#include <mutex>
#include <cstring>

namespace {
    size_t PARENT = 0;
    size_t MODE = PARENT + sizeof(std::pmr::memory_resource*);
    size_t TOTAL_SIZE = MODE + sizeof(allocator_with_fit_mode::fit_mode);
    size_t MUTEX = TOTAL_SIZE + sizeof(size_t);
    size_t FIRST_OCCUPIED = MUTEX + sizeof(std::mutex);
    size_t ALLOC_META_SIZE = FIRST_OCCUPIED + sizeof(void*);

    inline std::pmr::memory_resource* get_parent(void* trusted) {
        return *reinterpret_cast<std::pmr::memory_resource**>(reinterpret_cast<char*>(trusted) + PARENT);
    }
    inline void set_parent(void* trusted, std::pmr::memory_resource* parent) {
        *reinterpret_cast<std::pmr::memory_resource**>(reinterpret_cast<char*>(trusted) + PARENT) = parent;
    }

    inline allocator_with_fit_mode::fit_mode get_mode(void* trusted) {
        return *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<char*>(trusted) + MODE);
    }
    inline void set_mode(void* trusted, allocator_with_fit_mode::fit_mode mode) {
        *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<char*>(trusted) + MODE) = mode;
    }

    inline size_t get_total_size(void* trusted) {
        return *reinterpret_cast<size_t*>(reinterpret_cast<char*>(trusted) + TOTAL_SIZE);
    }
    inline void set_total_size(void* trusted, size_t size) {
        *reinterpret_cast<size_t*>(reinterpret_cast<char*>(trusted) + TOTAL_SIZE) = size;
    }

    inline std::mutex* get_mutex(void* trusted) {
        return reinterpret_cast<std::mutex*>(reinterpret_cast<char*>(trusted) + MUTEX);
    }

    inline void* get_first_occupied(void* trusted) {
        return *reinterpret_cast<void**>(reinterpret_cast<char*>(trusted) + FIRST_OCCUPIED);
    }
    inline void set_first_occupied(void* trusted, void* block) {
        *reinterpret_cast<void**>(reinterpret_cast<char*>(trusted) + FIRST_OCCUPIED) = block;
    }

    inline void* get_memory_start(void* trusted) { 
        return reinterpret_cast<char*>(trusted) + ALLOC_META_SIZE; 
    }
    inline void* get_memory_end(void* trusted) { 
        return reinterpret_cast<char*>(trusted) + get_total_size(trusted); 
    }

    size_t BLK_SIZE = 0;
    size_t BLK_OWNER = BLK_SIZE + sizeof(size_t);
    size_t BLK_PREV = BLK_OWNER + sizeof(void*);
    size_t BLK_NEXT = BLK_PREV + sizeof(void*);
    size_t BLK_META_SIZE = BLK_NEXT + sizeof(void*);

    inline size_t get_blk_size(void* blk) {
        return *reinterpret_cast<size_t*>(reinterpret_cast<char*>(blk) + BLK_SIZE);
    }
    inline void set_blk_size(void* blk, size_t size) {
        *reinterpret_cast<size_t*>(reinterpret_cast<char*>(blk) + BLK_SIZE) = size;
    }

    inline void* get_blk_owner(void* blk) {
        return *reinterpret_cast<void**>(reinterpret_cast<char*>(blk) + BLK_OWNER);
    }
    inline void set_blk_owner(void* blk, void* owner) {
        *reinterpret_cast<void**>(reinterpret_cast<char*>(blk) + BLK_OWNER) = owner;
    }

    inline void* get_blk_prev(void* blk) {
        return *reinterpret_cast<void**>(reinterpret_cast<char*>(blk) + BLK_PREV);
    }
    inline void set_blk_prev(void* blk, void* prev) {
        *reinterpret_cast<void**>(reinterpret_cast<char*>(blk) + BLK_PREV) = prev;
    }

    inline void* get_blk_next(void* blk) {
        return *reinterpret_cast<void**>(reinterpret_cast<char*>(blk) + BLK_NEXT);
    }
    inline void set_blk_next(void* blk, void* next) {
        *reinterpret_cast<void**>(reinterpret_cast<char*>(blk) + BLK_NEXT) = next;
    }

    inline void* get_blk_end(void* blk) {
        return reinterpret_cast<char*>(blk) + BLK_META_SIZE + get_blk_size(blk);
    }

    inline size_t get_gap_size(void* start, void* end) {
        return reinterpret_cast<char*>(end) - reinterpret_cast<char*>(start);
    }
}

allocator_boundary_tags::~allocator_boundary_tags()
{
    if(!_trusted_memory) return;

    auto mtx = get_mutex(_trusted_memory);
    auto parent = get_parent(_trusted_memory);
    auto size = get_total_size(_trusted_memory);

    mtx->~mutex();

    if (parent) {
        parent->deallocate(_trusted_memory, size);
    } else {
        ::operator delete(_trusted_memory);
    }

    _trusted_memory = nullptr;
}

allocator_boundary_tags::allocator_boundary_tags(
    allocator_boundary_tags &&other) noexcept
{
    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;
}

allocator_boundary_tags &allocator_boundary_tags::operator=(
    allocator_boundary_tags &&other) noexcept
{
    if (this != &other) {
        this->~allocator_boundary_tags();

        _trusted_memory = other._trusted_memory;
        other._trusted_memory = nullptr;
    }

    return *this;
}


/** If parent_allocator* == nullptr you should use std::pmr::get_default_resource()
 */
allocator_boundary_tags::allocator_boundary_tags(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (!parent_allocator) parent_allocator = std::pmr::get_default_resource();

    if (space_size < occupied_block_metadata_size) throw std::bad_alloc();

    size_t total_size = allocator_metadata_size + space_size;

    _trusted_memory = parent_allocator->allocate(total_size);
    if (!_trusted_memory) throw std::bad_alloc(); 

    set_parent(_trusted_memory, parent_allocator);
    set_mode(_trusted_memory, allocate_fit_mode);
    set_total_size(_trusted_memory, total_size);

    new (get_mutex(_trusted_memory)) std::mutex();

    set_first_occupied(_trusted_memory, nullptr);
}   

[[nodiscard]] void *allocator_boundary_tags::do_allocate_sm(
    size_t size)
{
    if (!_trusted_memory) throw std::bad_alloc();

    std::lock_guard<std::mutex> lock(*get_mutex(_trusted_memory));

    size_t needed_size = size + occupied_block_metadata_size;
    auto mode = get_mode(_trusted_memory);
   
    void* best_gap_start = nullptr;
    void* best_left_blk = nullptr;
    void* best_right_blk = nullptr;
    size_t best_gap_size = 0;
    bool found = false;


    void* current_right = get_first_occupied(_trusted_memory);
    void* current_left = nullptr;

    while (true) {
        void* gap_start = current_left ? get_blk_end(current_left) : get_memory_start(_trusted_memory);
        void* gap_end = current_right ? current_right :get_memory_end(_trusted_memory);

        size_t gap_size = get_gap_size(gap_start, gap_end);

        if (gap_size >= needed_size) {
            if (!found) {
                found = true;
                best_gap_start = gap_start;
                best_left_blk = current_left;
                best_right_blk = current_right;
                best_gap_size = gap_size;
                
                if (mode == allocator_with_fit_mode::fit_mode::first_fit) break; 
            } else {
                if ((mode == allocator_with_fit_mode::fit_mode::the_best_fit && gap_size < best_gap_size) ||
                    (mode == allocator_with_fit_mode::fit_mode::the_worst_fit && gap_size > best_gap_size))
                {
                    best_gap_start = gap_start; 
                    best_left_blk = current_left;
                    best_right_blk = current_right;
                    best_gap_size = gap_size;
                }
            }
        }

        if (!current_right) break;

        current_left = current_right;
        current_right = get_blk_next(current_right);
    }

    if (!found) throw std::bad_alloc();

    void* new_block = best_gap_start;
    size_t actual_payload_size = size;

    size_t leftover = best_gap_size - needed_size;
    if (leftover > 0 && leftover < occupied_block_metadata_size) {
        actual_payload_size += leftover;
    }

    set_blk_size(new_block, actual_payload_size);
    set_blk_owner(new_block, _trusted_memory);
    set_blk_prev(new_block, best_left_blk);
    set_blk_next(new_block, best_right_blk);

    if (best_left_blk) {
        set_blk_next(best_left_blk, new_block);
    } else {
        set_first_occupied(_trusted_memory, new_block);
    }
    
    if (best_right_blk) {
        set_blk_prev(best_right_blk, new_block);
    }

    return reinterpret_cast<char*>(new_block) + occupied_block_metadata_size;
}

void allocator_boundary_tags::do_deallocate_sm(
    void *at)
{
    if (!at || !_trusted_memory) return; 

    std::lock_guard<std::mutex> lock(*get_mutex(_trusted_memory));

    void* block = reinterpret_cast<char*>(at) - occupied_block_metadata_size;

    if (get_blk_owner(block) != _trusted_memory) {
        throw std::logic_error("Попытка освободить чужую память!");
    }

    void* prev = get_blk_prev(block);
    void* next = get_blk_next(block);

    if (prev) {
        set_blk_next(prev, next);
    } else {
        set_first_occupied(_trusted_memory, next);
    }

    if (next) {
        set_blk_prev(next, prev);
    }
}

inline void allocator_boundary_tags::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    if (!_trusted_memory) return;
    std::lock_guard<std::mutex> lock(*get_mutex(_trusted_memory));
    set_mode(_trusted_memory, mode);
}


std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const
{
    if (!_trusted_memory) return {};

    std::lock_guard<std::mutex> lock(*get_mutex(_trusted_memory));
    return get_blocks_info_inner();
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::begin() const noexcept
{
    return boundary_iterator(_trusted_memory);
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::end() const noexcept
{
    return boundary_iterator();
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info_inner() const
{
    std::vector<allocator_test_utils::block_info> result;
    
    for (auto it = begin(); it != end(); ++it) {
        result.push_back({
            .block_size = it.size(),
            .is_block_occupied = it.occupied()
        });
    }
    
    return result;
}

allocator_boundary_tags::allocator_boundary_tags(const allocator_boundary_tags &other)
{
    if(!other._trusted_memory) {
        _trusted_memory = nullptr;
        return;
    }

    std::lock_guard<std::mutex> lock(*get_mutex(other._trusted_memory));

    size_t total_size = get_total_size(other._trusted_memory);
    auto parent = get_parent(other._trusted_memory);

    _trusted_memory = parent ? parent->allocate(total_size) : ::operator new(total_size);
    std::memcpy(_trusted_memory, other._trusted_memory, total_size);

    new(get_mutex(_trusted_memory)) std::mutex();

    std::ptrdiff_t delta = reinterpret_cast<char*>(_trusted_memory) - reinterpret_cast<char*>(other._trusted_memory);

    void* first_occ = get_first_occupied(_trusted_memory);
    if (first_occ) {
        first_occ = reinterpret_cast<char*>(first_occ) + delta;
        set_first_occupied(_trusted_memory, first_occ);

        void* current = first_occ;
        while (current) { 
            set_blk_owner(current, _trusted_memory);

            void* prev = get_blk_prev(current);
            if (prev) set_blk_prev(current, reinterpret_cast<char*>(prev) + delta);

            void* next = get_blk_next(current);
            if (next) set_blk_next(current, reinterpret_cast<char*>(next) + delta); 

            current = get_blk_next(current);
        }
    }
}

allocator_boundary_tags &allocator_boundary_tags::operator=(const allocator_boundary_tags &other)
{
    if (this != &other) {
        allocator_boundary_tags temp(other);
        *this = std::move(temp);
    }

    return *this;
}

bool allocator_boundary_tags::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    auto p = dynamic_cast<const allocator_boundary_tags*>(&other);
    return p != nullptr && p->_trusted_memory == _trusted_memory;}

bool allocator_boundary_tags::boundary_iterator::operator==(
        const allocator_boundary_tags::boundary_iterator &other) const noexcept
{
    return _trusted_memory == other._trusted_memory &&
            _occupied_ptr == other._occupied_ptr && 
            _occupied == other._occupied;
}

bool allocator_boundary_tags::boundary_iterator::operator!=(
        const allocator_boundary_tags::boundary_iterator & other) const noexcept
{
    return !(*this == other);
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator++() & noexcept
{
    if (!_trusted_memory) return *this;

    if (!_occupied) {
        if (_occupied_ptr) {
            _occupied = true;
        } else {
            _trusted_memory = nullptr;
            _occupied_ptr = nullptr;
            _occupied = false;
        }
    } else {
        void* next = get_blk_next(_occupied_ptr);
        void* current_end = get_blk_end(_occupied_ptr);
        void* next_start = next ? next : get_memory_end(_trusted_memory);

        if (current_end < next_start) {
            _occupied_ptr = next;
            _occupied = false; 
        } else {
            if (next) {
                _occupied_ptr = next;
                _occupied = true;
            } else {
                _trusted_memory = nullptr;
                _occupied_ptr = nullptr;
                _occupied = false;
            }
        }
    }

    return *this;
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator--() & noexcept
{
    if (!_trusted_memory) return *this;

    if (!_occupied) {
        if (!_occupied_ptr) {
            void* last = get_first_occupied(_trusted_memory);
            if (last) {
                while (get_blk_next(last)) last = get_blk_next(last); 
            }
            _occupied_ptr = last;
            _occupied = (last != nullptr);
        } else {
            void* prev = get_blk_prev(_occupied_ptr);
            _occupied_ptr = prev;
            _occupied = (prev != nullptr);
        }
    } else { 
        void* prev = get_blk_prev(_occupied_ptr);
        void* current_start = _occupied_ptr;
        void* prev_end = prev ? get_blk_end(prev) : get_memory_start(_trusted_memory);

        if (prev_end < current_start) {
            _occupied = false; 
        } else {
            _occupied_ptr = prev; 
            _occupied = true;
        }
    }
    
    return *this;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator++(int n)
{
    auto copy = *this;
    ++(*this);
    return copy;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator--(int n)
{
    auto copy = *this;
    --(*this);
    return copy;
}

size_t allocator_boundary_tags::boundary_iterator::size() const noexcept
{
    if (!_trusted_memory) return 0;
    if (_occupied_ptr == get_memory_end(_trusted_memory)) return 0;

    if (_occupied) {
        return allocator_boundary_tags::occupied_block_metadata_size + get_blk_size(_occupied_ptr);
    }

    void* gap_start = nullptr;
    void* gap_end = nullptr;

    if (!_occupied_ptr) {
        void* last = get_first_occupied(_trusted_memory);
        if (last) {
            while (get_blk_next(last)) last = get_blk_next(last);
            gap_start = get_blk_end(last);
        } else {
            gap_start = get_memory_start(_trusted_memory);
        }

        gap_end = get_memory_end(_trusted_memory);
    } else {
        void* prev = get_blk_prev(_occupied_ptr);
        gap_start = prev ? get_blk_end(prev) : get_memory_start(_trusted_memory);
        gap_end = _occupied_ptr;
    }

    return get_gap_size(gap_start, gap_end);
}

bool allocator_boundary_tags::boundary_iterator::occupied() const noexcept
{
    return _occupied;
}

void* allocator_boundary_tags::boundary_iterator::operator*() const noexcept
{
    return get_ptr();
}

allocator_boundary_tags::boundary_iterator::boundary_iterator()
    : _occupied_ptr(nullptr), _occupied(false), _trusted_memory(nullptr)
{
}

allocator_boundary_tags::boundary_iterator::boundary_iterator(void *trusted)
    : _occupied_ptr(nullptr), _occupied(false), _trusted_memory(trusted)
{
    if (!_trusted_memory) return;

    void* first = get_first_occupied(_trusted_memory);
    if (!first) return;

    if (get_memory_start(_trusted_memory) < first) {
        _occupied_ptr = first;
        _occupied = false;
    } else {
        _occupied_ptr = first;
        _occupied = true;
    }
}

void *allocator_boundary_tags::boundary_iterator::get_ptr() const noexcept
{
    if (!_trusted_memory) return nullptr;
    if (_occupied_ptr == get_memory_end(_trusted_memory)) return nullptr;
    
    if (_occupied) return _occupied_ptr; 
    if (!_occupied_ptr) {
        void* last = get_first_occupied(_trusted_memory);
        if (last) {
            while (get_blk_next(last)) last = get_blk_next(last);
            return get_blk_end(last);
        }
        return get_memory_start(_trusted_memory);
    }
    
    void* prev = get_blk_prev(_occupied_ptr);
    return prev ? get_blk_end(prev) : get_memory_start(_trusted_memory);
}