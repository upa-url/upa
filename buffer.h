#ifndef WHATWG_BUFFER_H
#define WHATWG_BUFFER_H

#include <memory>
#include <type_traits>


template <class T, std::size_t fixed_capacity = 1024, class Allocator = std::allocator<T>>
class simple_buffer {
    static_assert(std::is_trivially_copyable<T>::value,
        "simple_buffer supports only trivially copyable elements");

public:
    typedef T value_type;
    typedef Allocator allocator_type;
    typedef std::allocator_traits<allocator_type> allocator_traits;
    typedef std::size_t size_type;

    // default
    simple_buffer() : simple_buffer(Allocator()) {}
    explicit simple_buffer(const Allocator& alloc)
        : allocator_(alloc)
        , data_(fixed_buffer_)
        , size_(0)
        , capacity_(fixed_capacity)
    {}
    // with count
    explicit simple_buffer(size_type count, const Allocator& alloc = Allocator())
        : allocator_(alloc)
        , data_(fixed_buffer_)
        , size_(0)
        , capacity_(fixed_capacity)
    {
        if (count > fixed_capacity)
            init_capacity(count);
    }
    // copy
//todo: simple_buffer(const simple_buffer& other);
//todo: simple_buffer(const simple_buffer& other, const Allocator& alloc);

    ~simple_buffer() {
        if (data_ != fixed_buffer_)
            allocator_traits::deallocate(allocator_, data_, capacity_);
    }

    allocator_type get_allocator() const {
        return allocator_;
    }

    value_type* data() {
        return data_;
    }
    const value_type* data() const {
        return data_;
    }

    // Capacity
    bool empty() const {
        return size_ == 0;
    }
    size_type size() const {
        return size_;
    }
    size_type max_size() const {
        return allocator_traits::max_size(allocator_);
    }
    size_type capacity() const {
        return capacity_;
    }

    void reserve(size_type new_cap) {
        if (new_cap > capacity_)
            grow_capacity(new_cap);
    }

    // Modifiers
    void clear() {
        size_ = 0;
    }

    template<class InputIt>
    void append(InputIt first, InputIt last) {
        auto ncopy = std::distance(first, last);
        // grow
        reserve(size_ + ncopy);
        // copy
        std::uninitialized_copy(first, last, data_ + size_);
        // add size
        size_ += ncopy;
    }

    void push_back(const value_type& value) {
        if (size_ < capacity_) {
            data_[size_] = value;
            size_++;
            return;
        }
        // grow
        reserve(size_ + 1);
        data_[size_] = value;
        size_++;
    }
    void pop_back() {
        size_--;
    }
    void resize(size_type count) {
        reserve(count);
        size_ = count;
    }

protected:
    void init_capacity(size_type new_cap) {
        data_ = allocator_traits::allocate(allocator_, new_cap);
        capacity_ = new_cap;
    }
    void grow_capacity(size_type new_cap) {
        value_type* new_data = allocator_traits::allocate(allocator_, new_cap);
        // copy data
        std::uninitialized_copy(data(), data() + size(), new_data);
        // deallocate old data & assign new
        if (data_ != fixed_buffer_)
            allocator_traits::deallocate(allocator_, data_, capacity_);
        data_ = new_data;
        capacity_ = new_cap;
    }

private:
    allocator_type allocator_;
    value_type* data_;
    size_type size_;
    size_type capacity_;

    // fixed size buffer
    value_type fixed_buffer_[fixed_capacity];
};


#endif // WHATWG_BUFFER_H
