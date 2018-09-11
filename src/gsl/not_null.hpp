#pragma once
#include <type_traits>
#include <exception>

namespace gsl {

template <class T>
class not_null
{
    static_assert(std::is_assignable<T&, std::nullptr_t>::value, "T cannot be assigned nullptr.");

public:

    not_null(T t) : m_ptr(t) {if(m_ptr == nullptr) std::terminate();}

    not_null& operator=(const T& t)
    {
        m_ptr = t;
        if(m_ptr == nullptr) std::terminate();
        return *this;
    }

    not_null(const not_null& other) = default;

    not_null(not_null&& other) = default;

    not_null& operator=(const not_null& other) = default;

    template <typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
    not_null(const not_null<U>& other)
    {
        *this = other;
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
    not_null& operator=(const not_null<U>& other)
    {
        m_ptr = other.get();
        return *this;
    }

    // prevents compilation when someone attempts to assign a nullptr
    not_null(std::nullptr_t) = delete;
    not_null(int) = delete;
    not_null<T>& operator=(std::nullptr_t) = delete;
    not_null<T>& operator=(int) = delete;

    T get() const
    {
        return m_ptr;
    }

    operator T() const {return get();}

    T operator->() const {return get();}

    bool operator==(const T& rhs) const {return m_ptr == rhs;}

    bool operator!=(const T& rhs) const {return !(*this == rhs);}

private:

    T m_ptr;

    not_null<T>& operator++() = delete;
    not_null<T>& operator--() = delete;
    not_null<T> operator++(int) = delete;
    not_null<T> operator--(int) = delete;
    not_null<T>& operator+(size_t) = delete;
    not_null<T>& operator+=(size_t) = delete;
    not_null<T>& operator-(size_t) = delete;
    not_null<T>& operator-=(size_t) = delete;
};

} // namespace gsl
