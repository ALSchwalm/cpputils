/**
 * Based on the boost scoped_ptr implementation:
 *
 * http://www.boost.org/doc/libs/1_59_0/libs/smart_ptr/scoped_ptr.htm
 */

//  scoped_ptr mimics a built-in pointer except that it guarantees deletion
//  of the object pointed to, either on destruction of the scoped_ptr or via
//  an explicit reset(). scoped_ptr is a simple solution for simple needs;
//  use shared_ptr or std::auto_ptr if your needs are more complex.

template <class T>
class scoped_ptr {
private:
    T* px;

    scoped_ptr(scoped_ptr const&);
    scoped_ptr& operator=(scoped_ptr const&);

    typedef scoped_ptr<T> this_type;

    void operator==(scoped_ptr const&) const;
    void operator!=(scoped_ptr const&) const;

public:
    typedef T element_type;

    explicit scoped_ptr(T* p = 0) : px(p) {}

    ~scoped_ptr() {
        // inline boost checked_delete
        typedef char type_must_be_complete[sizeof(T) ? 1 : -1];
        (void)sizeof(type_must_be_complete);
        delete px;
    }

    void reset(T* p = 0) { this_type(p).swap(*this); }

    T& operator*() const { return *px; }

    T* operator->() const { return px; }

    T* get() const { return px; }

    operator bool() const { return px != 0; }

    // operator! is redundant, but some compilers need it
    bool operator!() const { return px == 0; }

    void swap(scoped_ptr& b) {
        T* tmp = b.px;
        b.px = px;
        px = tmp;
    }
};

template <class T>
inline void swap(scoped_ptr<T>& a, scoped_ptr<T>& b) {
    a.swap(b);
}

// get_pointer(p) is a generic way to say p.get()

template <class T>
inline T* get_pointer(scoped_ptr<T> const& p) {
    return p.get();
}
