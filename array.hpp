/*
 * Based on the boost array implementation, see boost/array.hpp or
 *
 *   http://www.boost.org/libs/array/
 */

#include <algorithm> // equal, fill_n, lexicographical_compare, swap
#include <cstddef>   // size_t, ptrdiff_t
#include <iterator>  // reverse_iterator
#include <stdexcept> // out_of_range

template<typename T, std::size_t N>
class Array{
public:
    T elems[N];

    // Required typedefs for containers
    typedef T value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type* iterator;
    typedef const value_type* const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    // bounds-checked access
    reference at(size_type i) {
        boundscheck(i);
        return elems[i];
    }

    const_reference at(size_type i) const {
        boundscheck(i);
        return elems[i];
    }

    // non-bounds-checked access
    reference operator[](std::size_t index) {
        return elems[index];
    }

    const_reference operator[](std::size_t index) const {
        return elems[index];
    }

    // Front/back
    reference front() { return elems[0]; }
    const_reference front() const { return elems[0]; }

    reference back() { return elems[N - 1]; }
    const_reference back() const { return elems[N - 1]; }

    // Directly acess underlying array
    pointer data() { return elems; }
    const_pointer data() const { return elems; }

    // Iterators
    iterator begin() { return elems; }
    const_iterator begin() const { return elems; }
    const_iterator cbegin() const { return elems; }

    iterator end() { return elems + N; }
    const_iterator end() const { return elems + N; }
    const_iterator cend() const { return elems + N; }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return reverse_iterator(end()); }
    const_reverse_iterator crbegin() const { return reverse_iterator(end()); }

    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const { return reverse_iterator(begin()); }
    const_reverse_iterator crend() const { return reverse_iterator(begin()); }

    // An array is never empty
    static bool empty() { return false; }

    // Size is constant
    static size_type size() { return N; }
    static size_type max_size() { return N; }

    void fill(const value_type& value) {
        std::fill_n(begin(), N, value);
    }

    void swap(const Array& other) {
        for (size_type i = 0; i < N; ++i) {
            std::swap(elems[i], other[i]);
        }
    }

private:
    static void boundscheck(size_type i) {
        if (i >= N) {
            throw std::out_of_range("Array<>: index out of range");
        }
    }
};

// Comparison operators
template<typename T, std::size_t N>
bool operator==(const Array<T, N>& left, const Array<T, N>& right) {
    return std::equal(left.begin(), left.end(), right.begin());
}

template<typename T, std::size_t N>
bool operator!=(const Array<T, N>& left, const Array<T, N>& right) {
    return !(left == right);
}

template<typename T, std::size_t N>
bool operator<(const Array<T, N>& left, const Array<T, N>& right) {
    return std::lexicographical_compare(left.begin(),left.end(),
                                        right.begin(),right.end());
}

template<class T, std::size_t N>
bool operator> (const Array<T,N>& left, const Array<T,N>& right) {
    return right < left;
}

template<class T, std::size_t N>
bool operator>= (const Array<T,N>& left, const Array<T,N>& right) {
    return !(left < right);
}

template<class T, std::size_t N>
bool operator<= (const Array<T,N>& left, const Array<T,N>& right) {
    return !(right < left);
}

template<class T, std::size_t N>
void swap(const Array<T,N>& left, const Array<T,N>& right) {
    left.swap(right);
}

// Compile time checked access
template<std::size_t I, typename T, std::size_t N>
inline T& get(Array<T, N>& a) {
    // Basically a static_assert. Ensure that I < N
    (void) sizeof(char[(I < N) ? 1 : -1]);
    return a[I];
}

template<std::size_t I, typename T, std::size_t N>
inline const T& get(const Array<T, N>& a) {
    return get<I>(a);
}
