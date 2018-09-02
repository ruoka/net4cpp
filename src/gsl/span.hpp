#pragma once
#include <iterator>
#include <algorithm>
#include <experimental/type_traits>
#include "gsl/assert.hpp"

namespace gsl {

// [views.constants], constants
constexpr std::ptrdiff_t dynamic_extent = -1;

template<std::ptrdiff_t Extent>
struct __span_size
{
    template<typename T>
    __span_size(T)
    {}
    constexpr operator ptrdiff_t () const
    {
        return Extent;
    }
    template<typename T>
    auto& operator = (T)
    {
        return *this;
    }
};

template<>
struct __span_size<dynamic_extent>
{
    template<typename T>
    __span_size(T s) : m_value{static_cast<ptrdiff_t>(s)}
    {}
    constexpr operator std::ptrdiff_t () const
    {
        return m_value;
    }
    template<typename T>
    auto& operator = (T s)
    {
        m_value = s;
        return *this;
    }
private:
    std::ptrdiff_t m_value;
};

// A view over a contiguous, single-dimension sequence of objects
template <class ElementType, std::ptrdiff_t Extent = dynamic_extent>
class span
{
public:
    // constants and types
    using element_type = ElementType;
    using index_type = std::ptrdiff_t;
    using pointer = element_type*;
    using reference = element_type&;
    using iterator = pointer;
    using const_iterator = const pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr index_type extent = Extent;

    // [span.cons], span constructors, copy, assignment, and destructor constexpr span();
    constexpr span(std::nullptr_t) : m_data{nullptr}, m_size{0}
    {}

    constexpr span(pointer ptr, index_type count) : m_data{ptr}, m_size{count}
    {}

    constexpr span(pointer firstElem, pointer lastElem) : m_data{firstElem}, m_size{std::distance(firstElem,lastElem)}
    {}

    template <size_t N>
    constexpr span(element_type (&arr)[N]) : m_data{arr}, m_size{N}
    {}

    template <size_t N>
    constexpr span(std::array<std::remove_const_t<element_type>, N>& arr) : m_data{arr.data()}, m_size{N}
    {}

    template <size_t N>
    constexpr span(const std::array<std::remove_const_t<element_type>, N>& arr) : m_data{arr.data()}, m_size{N}
    {}

    template <class Container>
    constexpr span(Container& cont) : m_data{reinterpret_cast<ElementType*>(cont.data())}, m_size{cont.size()}
    {}

    template <class Container>
    constexpr span(const Container& cont) : m_data{reinterpret_cast<const ElementType*>(cont.data())}, m_size{cont.size()}
    {}

    // template <class Container> span(const Container&&) = delete;

    constexpr span(const span& other) noexcept = default;

    constexpr span(span&& other) noexcept = default;

    template <class OtherElementType, ptrdiff_t OtherExtent>
    constexpr span(const span<OtherElementType, OtherExtent>& other) : span<ElementType,Extent>{other.data(), other.size()}
    {
        using std::experimental::is_convertible_v;
        static_assert(std::is_convertible_v<OtherElementType,ElementType>, "Not convertible");
        static_assert(OtherExtent <= Extent, "Size mismatch");
    }

    template <class OtherElementType, ptrdiff_t OtherExtent>
    constexpr span(span<OtherElementType, OtherExtent>&& other) : span<ElementType,Extent>{other.data(), other.size()}
    {
        using std::experimental::is_convertible_v;
        static_assert(std::is_convertible_v<OtherElementType,ElementType>, "Not convertible");
        static_assert(OtherExtent <= Extent, "Size mismatch");
    }

    ~span() noexcept = default;

    constexpr span& operator=(const span& other) noexcept = default;

    constexpr span& operator=(span&& other) noexcept = default;

    // [span.sub], span subviews
    template <ptrdiff_t Count>
    constexpr span<element_type, Count> first() const
    {
        return first(Count);
    }

    template <ptrdiff_t Count>
    constexpr span<element_type, Count> last() const
    {
        return last(Count);
    }

    template <ptrdiff_t Offset, ptrdiff_t Count = dynamic_extent>
    constexpr span<element_type, Count> subspan() const
    {
        return subspan(Offset, Count);
    }

    constexpr span<element_type, dynamic_extent> first(index_type count) const
    {
        Ensures(count <= m_size);
        return {m_data,count};
    }

    constexpr span<element_type, dynamic_extent> last(index_type count) const
    {
        Ensures(count <= m_size);
        return {m_data + m_size - count, count};
    }

    constexpr span<element_type, dynamic_extent> subspan(index_type offset, index_type count = dynamic_extent) const
    {
        Ensures(offset <= m_size);
        return {m_data + offset, (count == dynamic_extent) ? (m_size - offset) : count};
    }

    // [span.obs], span observers
    constexpr index_type length() const noexcept
    {
        return m_size;
    }

    constexpr index_type size() const noexcept
    {
        return m_size;
    }

    constexpr index_type length_bytes() const noexcept
    {
        return m_size * sizeof(element_type);
    }

    constexpr index_type size_bytes() const noexcept
    {
        return m_size * sizeof(element_type);
    }

    constexpr bool empty() const noexcept
    {
        return !m_size;
    }

    // [span.elem], span element access
    constexpr reference operator[](index_type idx) const
    {
        Ensures(idx < m_size);
        return m_data[idx];
    }

    constexpr reference operator()(index_type idx) const
    {
        Ensures(idx < m_size);
        return m_data[idx];
    }

    constexpr pointer data() const noexcept
    {
        return m_data;
    }

    // [span.iter], span iterator support
    iterator begin() const noexcept
    {
        return m_data;
    }

    iterator end() const noexcept
    {
        return m_data + m_size;
    }

    const_iterator cbegin() const noexcept
    {
        return begin();
    }

    const_iterator cend() const noexcept
    {
        return end();
    }

    reverse_iterator rbegin() const noexcept
    {
        return std::make_reverse_iterator(end());
    }

    reverse_iterator rend() const noexcept
    {
        return std::make_reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const noexcept
    {
        return std::make_reverse_iterator(cend());
    }

    const_reverse_iterator crend() const noexcept
    {
        return std::make_reverse_iterator(cbegin());
    }

private:

    // exposition only

    pointer m_data;

    __span_size<Extent> m_size;
};

// [span.comparison], span comparison operators
template <class ElementType, ptrdiff_t Extent>
constexpr bool operator==(const span<ElementType, Extent>& l, const span<ElementType, Extent>& r) noexcept
{
    return std::equal(l.cbegin(),l.cend(),r.cbegin(),r.cend());
}

template <class ElementType, ptrdiff_t Extent>
constexpr bool operator!=(const span<ElementType, Extent>& l, const span<ElementType, Extent>& r) noexcept
{
    return !(l == r);
}

template <class ElementType, ptrdiff_t Extent>
constexpr bool operator<(const span<ElementType, Extent>& l, const span<ElementType, Extent>& r) noexcept
{
    return std::lexicographical_compare(l.cbegin(),l.cend(),r.cbegin(),r.cend());
}

template <class ElementType, ptrdiff_t Extent>
constexpr bool operator<=(const span<ElementType, Extent>& l, const span<ElementType, Extent>& r) noexcept
{
    return r == l || r < l;
}

template <class ElementType, ptrdiff_t Extent>
constexpr bool operator>(const span<ElementType, Extent>& l, const span<ElementType, Extent>& r) noexcept
{
    return std::lexicographical_compare(r.cbegin(),r.cend(),l.cbegin(),l.cend());
}

template <class ElementType, ptrdiff_t Extent>
constexpr bool operator>=(const span<ElementType, Extent>& l, const span<ElementType, Extent>& r) noexcept
{
    return r == l || r > l;
}

// [span.objectrep], views of object representation
template <class ElementType, ptrdiff_t Extent>
constexpr span<const std::byte, (Extent == dynamic_extent ? dynamic_extent : (sizeof(ElementType) * Extent))> as_bytes(span<ElementType, Extent> s) noexcept
{
    return {static_cast<const std::byte*>(s.data()), s.length_bytes()};
}

template <class ElementType, ptrdiff_t Extent>
constexpr span<std::byte, (Extent == dynamic_extent ? dynamic_extent : (sizeof(ElementType) * Extent))> as_writeable_bytes(span<ElementType, Extent> s) noexcept
{
    return {static_cast<std::byte*>(s.data()), s.length_bytes()};
}

//
// make_span() - Utility functions for creating spans
//

template <class ElementType>
span<ElementType> make_span(ElementType* ptr, typename span<ElementType>::index_type count)
{
    return span<ElementType>(ptr, count);
}

template <class ElementType>
span<ElementType> make_span(ElementType* firstElem, ElementType* lastElem)
{
    return span<ElementType>(firstElem, lastElem);
}

template <class ElementType, std::size_t N>
span<ElementType, N> make_span(ElementType (&arr)[N])
{
    return span<ElementType, N>(arr);
}

template <class Container>
span<typename Container::value_type> make_span(Container& cont)
{
    return span<typename Container::value_type>(cont);
}

template <class Container>
span<const typename Container::value_type> make_span(const Container& cont)
{
    return span<const typename Container::value_type>(cont);
}

template <class Ptr>
span<typename Ptr::element_type> make_span(Ptr& cont, std::ptrdiff_t count)
{
    return span<typename Ptr::element_type>(cont, count);
}

template <class Ptr>
span<typename Ptr::element_type> make_span(Ptr& cont)
{
    return span<typename Ptr::element_type>(cont);
}

} // namespace gsl
