#ifndef __ARRAY_DELETER__H
#define __ARRAY_DELETER__H

// ---------------------------------------------------------------------------!
namespace YumaTest {

/**
 * Template class that provides a deleter that can be passed to
 * a shared_ptr to automatically free arrays allocated using new[].
 *
 * \tparam T The type of array.
 */
template <typename T>
struct ArrayDeleter
{
    /**
     * Call delete[] on the supplied pointer.
     *
     * \param p the pointer to call delete[] on.
     */
    void operator()(T* p)
    {
        delete[] p;
    }
};

} // namespace YumaTest

#endif // __ARRAY_DELETER__H
