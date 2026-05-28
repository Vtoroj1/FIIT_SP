#ifndef SYS_PROG_B_TREE_H
#define SYS_PROG_B_TREE_H

#include <iterator>
#include <utility>
#include <boost/container/static_vector.hpp>
#include <stack>
#include <pp_allocator.h>
#include <associative_container.h>
#include <not_implemented.h>
#include <initializer_list>
#include <stdexcept>

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class B_tree final : private compare // EBCO
{
public:

    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

private:

    static constexpr const size_t minimum_keys_in_node = t - 1;
    static constexpr const size_t maximum_keys_in_node = 2 * t - 1;

    // region comparators declaration

    inline bool compare_keys(const tkey& lhs, const tkey& rhs) const;
    inline bool compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const;
    inline bool keys_equal(const tkey& lhs, const tkey& rhs) const;

    // endregion comparators declaration


    struct btree_node
    {
        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<btree_node*, maximum_keys_in_node + 2> _pointers;
        btree_node() noexcept;
    };

    pp_allocator<value_type> _allocator;
    btree_node* _root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;
    pp_allocator<btree_node> get_node_allocator() const noexcept;

    static value_type& as_value(tree_data_type& x) noexcept;
    static const value_type& as_value(const tree_data_type& x) noexcept;

    template<typename node_ptr_ptr>
    static void increment_iterator(std::stack<std::pair<node_ptr_ptr, size_t>>& path, size_t& index);

    template<typename node_ptr_ptr>
    static void decrement_iterator(std::stack<std::pair<node_ptr_ptr, size_t>>& path, size_t& index);

    std::stack<std::pair<btree_node**, size_t>> get_leftmost_path();
    std::stack<std::pair<btree_node**, size_t>> get_rightmost_path();

    btree_node* make_node();
    void delete_node(btree_node* node);
    void destroy_tree(btree_node* node);
    void split_child(btree_node* parent, size_t child_index);
    void insert_bottom_up(btree_node* node, tree_data_type data);

    size_t find_key_index(btree_node* node, const tkey& key);
    tree_data_type& get_min_key(btree_node* node);
    tree_data_type& get_max_key(btree_node* node);

    void borrow_from_left(btree_node* parent, size_t child_index);
    void borrow_from_right(btree_node* parent, size_t child_index);
    void merge_children(btree_node* parent, size_t left_child_index);
    void ensure_child_has_enough_keys(btree_node* parent, size_t& child_index);

    bool try_erase_from_node(btree_node* node, const tkey& key);

public:

    // region constructors declaration

    explicit B_tree(const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    explicit B_tree(pp_allocator<value_type> alloc, const compare& comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit B_tree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    // endregion constructors declaration

    // region five declaration

    B_tree(const B_tree& other);

    B_tree(B_tree&& other) noexcept;

    B_tree& operator=(const B_tree& other);

    B_tree& operator=(B_tree&& other) noexcept;

    ~B_tree() noexcept;

    // endregion five declaration

    // region iterators declaration

    class btree_iterator;
    class btree_reverse_iterator;
    class btree_const_iterator;
    class btree_const_reverse_iterator;

    class btree_iterator final
    {
        std::stack<std::pair<btree_node**, size_t>> _path;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_iterator(const std::stack<std::pair<btree_node**, size_t>>& path = std::stack<std::pair<btree_node**, size_t>>(), size_t index = 0);

    };

    class btree_const_iterator final
    {
        std::stack<std::pair<btree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_iterator;
        friend class btree_const_reverse_iterator;

        btree_const_iterator(const btree_iterator& it) noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_const_iterator(const std::stack<std::pair<btree_node* const*, size_t>>& path = std::stack<std::pair<btree_node* const*, size_t>>(), size_t index = 0);
    };

    class btree_reverse_iterator final
    {
        std::stack<std::pair<btree_node**, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_reverse_iterator;

        friend class B_tree;
        friend class btree_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        btree_reverse_iterator(const btree_iterator& it) noexcept;
        operator btree_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_reverse_iterator(const std::stack<std::pair<btree_node**, size_t>>& path = std::stack<std::pair<btree_node**, size_t>>(), size_t index = 0);
    };

    class btree_const_reverse_iterator final
    {
        std::stack<std::pair<btree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_reverse_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_iterator;

        btree_const_reverse_iterator(const btree_reverse_iterator& it) noexcept;
        operator btree_const_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_const_reverse_iterator(const std::stack<std::pair<btree_node* const*, size_t>>& path = std::stack<std::pair<btree_node* const*, size_t>>(), size_t index = 0);
    };

    friend class btree_iterator;
    friend class btree_const_iterator;
    friend class btree_reverse_iterator;
    friend class btree_const_reverse_iterator;

    // endregion iterators declaration

    // region element access declaration

    /*
     * Returns a reference to the mapped value of the element with specified key. If no such element exists, an exception of type std::out_of_range is thrown.
     */
    tvalue& at(const tkey&);
    const tvalue& at(const tkey&) const;

    /*
     * If key not exists, makes default initialization of value
     */
    tvalue& operator[](const tkey& key);
    tvalue& operator[](tkey&& key);

    // endregion element access declaration
    // region iterator begins declaration

    btree_iterator begin();
    btree_iterator end();

    btree_const_iterator begin() const;
    btree_const_iterator end() const;

    btree_const_iterator cbegin() const;
    btree_const_iterator cend() const;

    btree_reverse_iterator rbegin();
    btree_reverse_iterator rend();

    btree_const_reverse_iterator rbegin() const;
    btree_const_reverse_iterator rend() const;

    btree_const_reverse_iterator crbegin() const;
    btree_const_reverse_iterator crend() const;

    // endregion iterator begins declaration

    // region lookup declaration

    size_t size() const noexcept;
    bool empty() const noexcept;

    /*
     * Returns end() if not exist
     */

    btree_iterator find(const tkey& key);
    btree_const_iterator find(const tkey& key) const;

    btree_iterator lower_bound(const tkey& key);
    btree_const_iterator lower_bound(const tkey& key) const;

    btree_iterator upper_bound(const tkey& key);
    btree_const_iterator upper_bound(const tkey& key) const;

    bool contains(const tkey& key) const;

    // endregion lookup declaration

    // region modifiers declaration

    void clear() noexcept;

    /*
     * Does nothing if key exists, delegates to emplace.
     * Second return value is true, when inserted
     */
    std::pair<btree_iterator, bool> insert(const tree_data_type& data);
    std::pair<btree_iterator, bool> insert(tree_data_type&& data);

    template <typename ...Args>
    std::pair<btree_iterator, bool> emplace(Args&&... args);

    /*
     * Updates value if key exists, delegates to emplace.
     */
    btree_iterator insert_or_assign(const tree_data_type& data);
    btree_iterator insert_or_assign(tree_data_type&& data);

    template <typename ...Args>
    btree_iterator emplace_or_assign(Args&&... args);

    /*
     * Return iterator to node next ro removed or end() if key not exists
     */
    btree_iterator erase(btree_iterator pos);
    btree_iterator erase(btree_const_iterator pos);

    btree_iterator erase(btree_iterator beg, btree_iterator en);
    btree_iterator erase(btree_const_iterator beg, btree_const_iterator en);

    btree_iterator erase(const tkey& key);

    // endregion modifiers declaration
};

template<std::input_iterator iterator, comparator<typename std::iterator_traits<iterator>::value_type::first_type> compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
        std::size_t t = 5, typename U>
B_tree(iterator begin, iterator end, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> B_tree<typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> B_tree<tkey, tvalue, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_pairs(const B_tree::tree_data_type &lhs,
                                                     const B_tree::tree_data_type &rhs) const
{
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const
{
    return compare::operator()(lhs, rhs);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::keys_equal(const tkey& lhs, const tkey& rhs) const
{
    return !compare_keys(lhs, rhs) && !compare_keys(rhs, lhs);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_node::btree_node() noexcept = default;

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename B_tree<tkey, tvalue, compare, t>::value_type> B_tree<tkey, tvalue, compare, t>::get_allocator() const noexcept
{
    return _allocator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename B_tree<tkey, tvalue, compare, t>::btree_node> 
B_tree<tkey, tvalue, compare, t>::get_node_allocator() const noexcept
{
    return pp_allocator<btree_node>(_allocator);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node* B_tree<tkey, tvalue, compare, t>::make_node()
{
    return get_node_allocator().template new_object<btree_node>();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::delete_node(btree_node* node)
{
    if (node != nullptr) {
        get_node_allocator().template delete_object<btree_node>(node);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::destroy_tree(btree_node* node)
{
    if (node == nullptr) return;

    for (auto* child : node->_pointers) {
        destroy_tree(child);
    }

    delete_node(node);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::value_type&
B_tree<tkey, tvalue, compare, t>::as_value(tree_data_type& x) noexcept
{
    return reinterpret_cast<value_type&>(x);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const typename B_tree<tkey, tvalue, compare, t>::value_type&
B_tree<tkey, tvalue, compare, t>::as_value(const tree_data_type& x) noexcept
{
    return reinterpret_cast<const value_type&>(x);
}

// region constructors implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        const compare& cmp,
        pp_allocator<value_type> alloc)
    : compare(cmp), 
      _allocator(alloc), 
      _root(nullptr), 
      _size(0)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        pp_allocator<value_type> alloc,
        const compare& comp)
    : compare(comp), 
      _allocator(alloc), 
      _root(nullptr), 
      _size(0)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
B_tree<tkey, tvalue, compare, t>::B_tree(
        iterator begin,
        iterator end,
        const compare& cmp,
        pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
    for (; begin != end; ++begin) {
        insert(tree_data_type(begin->first, begin->second));
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        std::initializer_list<std::pair<tkey, tvalue>> data,
        const compare& cmp,
        pp_allocator<value_type> alloc)
    : B_tree(data.begin(), data.end(), cmp, alloc)
{
}

// endregion constructors implementation

// region five implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::~B_tree() noexcept
{
    clear();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(const B_tree& other)
    : compare(other), _allocator(other._allocator), _root(nullptr), _size(0)
{
    for (auto it = other.begin(); it != other.end(); ++it) {
        insert(tree_data_type(it->first, it->second));
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(const B_tree& other)
{
    if (this != &other) {
        clear();
        static_cast<compare&>(*this) = static_cast<const compare&>(other);
        _allocator = other._allocator;

        for (auto it = other.begin(); it != other.end(); ++it) {
            insert(tree_data_type(it->first, it->second));
        }
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(B_tree&& other) noexcept
    : compare(std::move(other)), 
      _allocator(std::move(other._allocator)),
      _root(other._root),
      _size(other._size)
{
    other._root = nullptr;
    other._size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(B_tree&& other) noexcept
{
    if (this != &other) {
        clear();
        static_cast<compare&>(*this) = std::move(static_cast<compare&>(other));
        _allocator = std::move(other._allocator);
        _root = other._root;
        _size = other._size;
        other._root = nullptr;
        other._size = 0;
    }
    return *this;
}

//=============================================

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename node_ptr_ptr>
void B_tree<tkey, tvalue, compare, t>::increment_iterator(std::stack<std::pair<node_ptr_ptr, size_t>>& path, size_t& index)
{
    if (path.empty()) return;

    if ((*path.top().first)->_pointers.empty()) {
        if (index + 1 < (*path.top().first)->_keys.size()) {
            index++;
            return;
        }

        auto path_tmp = path;
        auto child = path_tmp.top();
        path_tmp.pop();

        while (!path_tmp.empty()) {
            size_t next_in_parent = child.second;

            if (next_in_parent < (*path_tmp.top().first)->_keys.size()) {
                path = path_tmp;
                index = next_in_parent;
                return;
            }

            child = path_tmp.top();
            path_tmp.pop();
        }

        index = (*path.top().first)->_keys.size();
        return;
    }

    auto kid = &((*path.top().first)->_pointers[index + 1]);
    path.push({ kid, index + 1 });

    while (!((*kid)->_pointers.empty())) {
        kid = &((*kid)->_pointers[0]);
        path.push({ kid, 0 });
    }

    index = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename node_ptr_ptr>
void B_tree<tkey, tvalue, compare, t>::decrement_iterator(std::stack<std::pair<node_ptr_ptr, size_t>>& path, size_t& index)
{
    if (path.empty()) return;

    if (index == (*path.top().first)->_keys.size()) {
        if (!(*path.top().first)->_keys.empty()) index--;
        return;
    }

    if ((*path.top().first)->_pointers.empty()) {
        if (index > 0) {
            index--;
            return;
        }

        auto path_tmp = path;
        auto child = path_tmp.top();
        path_tmp.pop();

        while (!path_tmp.empty()) {
            if (child.second > 0) {
                path = path_tmp;
                index = child.second - 1;
                return;
            }

            child = path_tmp.top();
            path_tmp.pop();
        }

        index = 0;
        return;
    }

    auto kid = &((*path.top().first)->_pointers[index]);
    path.push({ kid, index });

    while (!((*kid)->_pointers.empty())) {
        size_t child_index = (*kid)->_pointers.size() - 1;
        kid = &((*kid)->_pointers[child_index]);
        path.push({ kid, child_index });
    }

    index = (*kid)->_keys.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator::btree_iterator(
        const std::stack<std::pair<btree_node**, size_t>>& path, size_t index)
    : _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator*() const noexcept {
    return B_tree::as_value((*_path.top().first)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator->() const noexcept {
    return &(B_tree::as_value((*_path.top().first)->_keys[_index]));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator& B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++() {
    B_tree::increment_iterator(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++(int) {
    auto tmp = *this;
    ++(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator& B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--() {
    B_tree::decrement_iterator(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--(int) {
    auto tmp = *this;
    --(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator==(const self& other) const noexcept {
    if (_index != other._index || _path.size() != other._path.size()) return false;
    if (_path.empty()) return true;
    return _path.top().first == other._path.top().first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator!=(const self& other) const noexcept {
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::depth() const noexcept { 
    return _path.empty() ? 0 : (_path.size() - 1); 
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::current_node_keys_count() const noexcept { 
    return _path.empty() ? 0 : (*_path.top().first)->_keys.size(); 
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::is_terminate_node() const noexcept { 
    return _path.empty() || _index >= (*_path.top().first)->_keys.size(); 
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::index() const noexcept { 
    return _index; 
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(
        const std::stack<std::pair<btree_node* const*, size_t>>& path, size_t index) : _path(path), _index(index) 
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(const btree_iterator& it) noexcept : _index(it._index) {
    std::stack<std::pair<btree_node* const*, size_t>> tmp;
    auto st = it._path;

    while (!st.empty()) { 
        tmp.push({ st.top().first, st.top().second });
        st.pop(); 
    }

    while (!tmp.empty()) { 
        _path.push({ tmp.top().first, tmp.top().second });
        tmp.pop();
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator*() const noexcept {
    return B_tree::as_value((*_path.top().first)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator->() const noexcept {
    return &(B_tree::as_value((*_path.top().first)->_keys[_index]));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator& B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++() {
    B_tree::increment_iterator(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++(int) {
    auto tmp = *this; 
    ++(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator& B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--() {
    B_tree::decrement_iterator(_path, _index); 
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--(int) {
    auto tmp = *this;
    --(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator==(const self& other) const noexcept {
    if (_index != other._index || _path.size() != other._path.size()) return false;
    if (_path.empty()) return true;
    return _path.top().first == other._path.top().first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator!=(const self& other) const noexcept {
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::depth() const noexcept {
    return _path.empty() ? 0 : (_path.size() - 1);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::current_node_keys_count() const noexcept {
    return _path.empty() ? 0 : (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::is_terminate_node() const noexcept {
    return _path.empty() || _index >= (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::index() const noexcept {
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(
        const std::stack<std::pair<btree_node**, size_t>>& path, size_t index) : _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(const btree_iterator& it) noexcept : _path(it._path), _index(it._index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator B_tree<tkey, tvalue, compare, t>::btree_iterator() const noexcept {
    return btree_iterator(_path, _index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator*() const noexcept {
    btree_iterator it = *this;
    --it;
    return *it;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator->() const noexcept {
    return &(operator*());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator& B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++() {
    B_tree::decrement_iterator(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++(int) {
    auto tmp = *this;
    ++(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator& B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--() {
    B_tree::increment_iterator(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--(int) {
    auto tmp = *this;
    --(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator==(const self& other) const noexcept {
    return btree_iterator(_path, _index) == btree_iterator(other._path, other._index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator!=(const self& other) const noexcept { 
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::depth() const noexcept { 
    btree_iterator it = *this;
    --it;
    return it.depth();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::current_node_keys_count() const noexcept {
    btree_iterator it = *this;
    --it;
    return it.current_node_keys_count();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::is_terminate_node() const noexcept { 
    btree_iterator it = *this;
    --it;
    return it.is_terminate_node();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::index() const noexcept {
    btree_iterator it = *this;
    --it;
    return it.index();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(
        const std::stack<std::pair<btree_node* const*, size_t>>& path, size_t index) : _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(const btree_reverse_iterator& it) noexcept : _index(it._index) {
    std::stack<std::pair<btree_node* const*, size_t>> tmp;
    auto st = it._path;

    while (!st.empty()) {
        tmp.push({ st.top().first, st.top().second });
        st.pop();
    }

    while (!tmp.empty()) {
        _path.push({ tmp.top().first, tmp.top().second });
        tmp.pop();
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator B_tree<tkey, tvalue, compare, t>::btree_const_iterator() const noexcept {
    return btree_const_iterator(_path, _index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator*() const noexcept {
    btree_const_iterator it = *this;
    --it; 
    return *it;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator->() const noexcept {
    return &(operator*());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator& B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++() {
    B_tree::decrement_iterator(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++(int) {
    auto tmp = *this; ++(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator& B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--() {
    B_tree::increment_iterator(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--(int) {
    auto tmp = *this;
    --(*this); 
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator==(const self& other) const noexcept {
    return btree_const_iterator(_path, _index) == btree_const_iterator(other._path, other._index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator!=(const self& other) const noexcept {
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::depth() const noexcept {
    btree_const_iterator it = *this;
    --it;
    return it.depth();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::current_node_keys_count() const noexcept {
    btree_const_iterator it = *this;
    --it; 
    return it.current_node_keys_count();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::is_terminate_node() const noexcept {
    btree_const_iterator it = *this;
    --it;
    return it.is_terminate_node();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::index() const noexcept {
    btree_const_iterator it = *this;
    --it;
    return it.index();
}

// endregion iterators implementation

// region element access implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key)
{
    auto it = find(key);

    if (it == end()) 
        throw std::out_of_range("B_tree::at");

    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key) const
{
    auto it = find(key);

    if (it == end()) 
        throw std::out_of_range("B_tree::at");

    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](const tkey& key)
{
    return emplace(key, tvalue{}).first->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](tkey&& key)
{
    return emplace(std::move(key), tvalue{}).first->second;
}

// endregion element access implementation

// region iterator begins implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::stack<std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_node**, size_t>> 
B_tree<tkey, tvalue, compare, t>::get_leftmost_path()
{
    std::stack<std::pair<btree_node**, size_t>> path;
    if (_root == nullptr) return path;

    btree_node** cur = &_root;
    path.push({ cur, 0 });

    while (!((*cur)->_pointers.empty())) {
        cur = &((*cur)->_pointers[0]);
        path.push({ cur, 0 });
    }

    return path;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::stack<std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_node**, size_t>> 
B_tree<tkey, tvalue, compare, t>::get_rightmost_path()
{
    std::stack<std::pair<btree_node**, size_t>> path;
    if (_root == nullptr) return path;

    btree_node** cur = &_root;
    path.push({ cur, 0 });

    while (!((*cur)->_pointers.empty())) {
        size_t rightmost_index = (*cur)->_pointers.size() - 1;
        cur = &((*cur)->_pointers[rightmost_index]);
        path.push({ cur, rightmost_index });
    }

    return path;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::begin()
{
    if (_root == nullptr) return btree_iterator();

    return btree_iterator(get_leftmost_path(), 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::end()
{
    if (_root == nullptr) return btree_iterator();
    
    auto path = get_rightmost_path();
    return btree_iterator(path, (*path.top().first)->_keys.size());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::begin() const
{
    return cbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::end() const
{
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cbegin() const
{
    return btree_const_iterator(const_cast<B_tree*>(this)->begin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cend() const
{
    return btree_const_iterator(const_cast<B_tree*>(this)->end());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin()
{
    return btree_reverse_iterator(end());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend()
{
    return btree_reverse_iterator(begin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin() const
{
    return crbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend() const
{
    return crend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crbegin() const
{
    return btree_const_reverse_iterator(const_cast<B_tree*>(this)->rbegin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crend() const
{
    return btree_const_reverse_iterator(const_cast<B_tree*>(this)->rend());
}

// endregion iterator begins implementation

// region lookup implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::size() const noexcept
{
    return _size;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::empty() const noexcept
{
    return _size == 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key)
{
    if (_root == nullptr) return end();

    std::stack<std::pair<btree_node**, size_t>> path;
    btree_node** cur = &_root;
    path.push({ cur, 0 });

    while (true) {
        btree_node* node = *cur;
        size_t i = 0;

        for (; i < node->_keys.size() && compare_keys(node->_keys[i].first, key); i++);

        if (i < node->_keys.size() && keys_equal(node->_keys[i].first, key)) {
            return btree_iterator(path, i);
        }

        if (node->_pointers.empty()) return end();

        path.top().second = i;
        cur = &(node->_pointers[i]);
        path.push({ cur, 0 });
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key) const
{
    return btree_const_iterator(const_cast<B_tree*>(this)->find(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key)
{
    if (_root == nullptr) return end();

    std::stack<std::pair<btree_node**, size_t>> path;
    std::stack<std::pair<btree_node**, size_t>> best_path;
    size_t best_index = 0;
    bool found_best = false;
    
    btree_node** cur = &_root;
    path.push({ cur, 0 });

    while (true) {
        btree_node* node = *cur;
        size_t i = 0;
        for (; i < node->_keys.size() && compare_keys(node->_keys[i].first, key); i++);

        if (i < node->_keys.size()) {
            if (keys_equal(node->_keys[i].first, key)) {
                return btree_iterator(path, i);
            }
            
            best_path = path;
            best_index = i;
            found_best = true;
        }

        if (node->_pointers.empty()) return found_best ? btree_iterator(best_path, best_index) : end();

        path.top().second = i;
        cur = &(node->_pointers[i]);
        path.push({ cur, 0 });
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) const
{
    return btree_const_iterator(const_cast<B_tree*>(this)->lower_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key)
{
    if (_root == nullptr) return end();

    std::stack<std::pair<btree_node**, size_t>> path;
    std::stack<std::pair<btree_node**, size_t>> best_path;
    size_t best_index = 0;
    bool found_best = false;
    
    btree_node** cur = &_root;
    path.push({ cur, 0 });

    while (true) {
        btree_node* node = *cur;
        size_t i = 0;
        for (; i < node->_keys.size() && !compare_keys(key, node->_keys[i].first); i++);

        if (i < node->_keys.size()) {
            best_path = path;
            best_index = i;
            found_best = true;
        }

        if (node->_pointers.empty()) return found_best ? btree_iterator(best_path, best_index) : end();

        path.top().second = i;
        cur = &(node->_pointers[i]);
        path.push({ cur, 0 });
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) const
{
    return btree_const_iterator(const_cast<B_tree*>(this)->upper_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::contains(const tkey& key) const
{
    return find(key) != end();
}

// endregion lookup implementation

// region modifiers implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::clear() noexcept
{
    destroy_tree(_root);
    _root = nullptr;
    _size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)
{
    return emplace(data);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)
{
    return emplace(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::emplace(Args&&... args) 
{
    tree_data_type data(args...);
    
    auto existing = find(data.first);
    if (existing != end()) return { existing, false };

    tkey key = data.first; 

    if (_root == nullptr) {
        _root = make_node();
        _root->_keys.push_back(std::move(data));
        _size++;
        return { begin(), true };
    }

    insert_bottom_up(_root, std::move(data));

    if (_root->_keys.size() > maximum_keys_in_node) {
        btree_node* new_root = make_node();
        new_root->_pointers.push_back(_root);
        _root = new_root;
        split_child(_root, 0);
    }

    _size++;
    return { find(key), true };
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)
{
    auto it = find(data.first);
    if (it != end()) {
        it->second = data.second;
        return it;
    }
    return insert(data).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)
{
    auto it = find(data.first);
    if (it != end()) {
        it->second = std::move(data.second);
        return it;
    }
    return insert(std::move(data)).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)
{
    tree_data_type data(args...);
    return insert_or_assign(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::split_child(btree_node* parent, size_t child_index)
{
    btree_node* left = parent->_pointers[child_index];
    btree_node* right = make_node();

    tree_data_type median = std::move(left->_keys[t]);
    for (size_t i = t + 1; i < left->_keys.size(); i++) {
        right->_keys.push_back(std::move(left->_keys[i]));
    }
    left->_keys.erase(left->_keys.begin() + t, left->_keys.end());

    if (!left->_pointers.empty()) { 
        for (size_t i = t + 1; i < left->_pointers.size(); i++) {
            right->_pointers.push_back(left->_pointers[i]);
        }
        left->_pointers.erase(left->_pointers.begin() + (t + 1), left->_pointers.end());
    }

    parent->_pointers.insert(parent->_pointers.begin() + (child_index + 1), right); 
    parent->_keys.insert(parent->_keys.begin() + child_index, std::move(median)); 
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::insert_bottom_up(btree_node* node, tree_data_type data)
{
    size_t i = 0;
    for (; i < node->_keys.size() && compare_keys(node->_keys[i].first, data.first); i++);

    if (node->_pointers.empty()) {
        node->_keys.insert(node->_keys.begin() + i, std::move(data));
        return;
    }

    insert_bottom_up(node->_pointers[i], std::move(data));

    if (node->_pointers[i]->_keys.size() > maximum_keys_in_node) {
        split_child(node, i);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::find_key_index(btree_node* node, const tkey& key)
{
    size_t i = 0;
    for (; i < node->_keys.size() && compare_keys(node->_keys[i].first, key); i++);
    return i;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::tree_data_type& 
B_tree<tkey, tvalue, compare, t>::get_min_key(btree_node* node)
{
    while (!node->_pointers.empty()) node = node->_pointers.front();
    return node->_keys.front();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::tree_data_type& 
B_tree<tkey, tvalue, compare, t>::get_max_key(btree_node* node)
{
    while (!node->_pointers.empty()) node = node->_pointers.back();
    return node->_keys.back();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::borrow_from_left(btree_node* parent, size_t child_index)
{
    btree_node* child = parent->_pointers[child_index];
    btree_node* left = parent->_pointers[child_index - 1];

    child->_keys.insert(child->_keys.begin(), std::move(parent->_keys[child_index - 1]));
    
    if (!left->_pointers.empty()) {
        child->_pointers.insert(child->_pointers.begin(), left->_pointers.back());
        left->_pointers.pop_back();
    }
    
    parent->_keys[child_index - 1] = std::move(left->_keys.back());
    left->_keys.pop_back();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::borrow_from_right(btree_node* parent, size_t child_index)
{
    btree_node* child = parent->_pointers[child_index];
    btree_node* right = parent->_pointers[child_index + 1];

    child->_keys.push_back(std::move(parent->_keys[child_index]));
    
    if (!right->_pointers.empty()) {
        child->_pointers.push_back(right->_pointers.front());
        right->_pointers.erase(right->_pointers.begin());
    }
    
    parent->_keys[child_index] = std::move(right->_keys.front());
    right->_keys.erase(right->_keys.begin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::merge_children(btree_node* parent, size_t left_child_index)
{
    btree_node* left = parent->_pointers[left_child_index];
    btree_node* right = parent->_pointers[left_child_index + 1];

    left->_keys.push_back(std::move(parent->_keys[left_child_index]));
    
    for (auto& key : right->_keys) 
        left->_keys.push_back(std::move(key));
    for (auto& ptr : right->_pointers) 
        left->_pointers.push_back(ptr);

    parent->_keys.erase(parent->_keys.begin() + left_child_index);
    parent->_pointers.erase(parent->_pointers.begin() + (left_child_index + 1));
    
    delete_node(right);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::ensure_child_has_enough_keys(btree_node* parent, size_t& child_index)
{
    if (parent->_pointers[child_index]->_keys.size() >= t) return;

    if (child_index > 0 && parent->_pointers[child_index - 1]->_keys.size() >= t) { 
        borrow_from_left(parent, child_index); 
        return; 
    }

    if (child_index + 1 < parent->_pointers.size() && parent->_pointers[child_index + 1]->_keys.size() >= t) { 
        borrow_from_right(parent, child_index); 
        return; 
    }

    if (child_index + 1 < parent->_pointers.size()) { 
        merge_children(parent, child_index); 
    } else { 
        merge_children(parent, child_index - 1); 
        --child_index; 
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::try_erase_from_node(btree_node* node, const tkey& key)
{
    size_t idx = find_key_index(node, key);

    if (idx < node->_keys.size() && keys_equal(node->_keys[idx].first, key)) {
        if (node->_pointers.empty()) {
            node->_keys.erase(node->_keys.begin() + idx);
            return true;
        }

        if (node->_pointers[idx]->_keys.size() >= t) {
            tree_data_type left = get_max_key(node->_pointers[idx]);
            node->_keys[idx] = left;
            return try_erase_from_node(node->_pointers[idx], left.first);
        }

        if (node->_pointers[idx + 1]->_keys.size() >= t) {
            tree_data_type right = get_min_key(node->_pointers[idx + 1]);
            node->_keys[idx] = right;
            return try_erase_from_node(node->_pointers[idx + 1], right.first);
        }

        merge_children(node, idx);
        return try_erase_from_node(node->_pointers[idx], key);
    }

    if (node->_pointers.empty()) return false;

    ensure_child_has_enough_keys(node, idx);
    return try_erase_from_node(node->_pointers[idx], key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::erase(const tkey& key)
{
    if (!_root) return end();
    
    if (!try_erase_from_node(_root, key)) return end();
    
    --_size; 

    if (_root->_keys.empty()) {
        btree_node* old_root = _root;
        _root = _root->_pointers.empty() ? nullptr : _root->_pointers[0];
        delete_node(old_root);
    }
    
    return lower_bound(key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::erase(btree_iterator pos)
{
    if (pos == end()) return end();
    return erase(pos->first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator pos)
{
    if (pos == cend()) return end();
    return erase(pos->first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::erase(btree_iterator beg, btree_iterator en)
{
    while (beg != en) {
        beg = erase(beg);
    }
    return beg;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator beg, btree_const_iterator en)
{
    auto first = (beg == cend()) ? end() : find(beg->first);
    auto last = (en == cend()) ? end() : find(en->first);
    return erase(first, last);
}

// endregion modifiers implementation

#endif