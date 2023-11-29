/*

Test implementation of a polymorphic_value class for possible introduction into the C++ standard library.

This implementation diverts from P0201 in several ways. See README,md for details.

This software is provided under the MIT license:

Copyright 2022 Bengt Gustafsson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files
(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Further information at: https://opensource.org/licenses/MIT.

*/



#pragma once

#include <memory>           // unique_ptr
#include <type_traits>      // is_copy_constructible, is_move_constructible
#include <utility>          // construct_at, destroy_at
#include <optional>         // nullopt
#include <algorithm>        // all_of, max_element
#include <initializer_list>

#if IS_STANDARDIZED

#define STD std
namespace std {

#else

#define STD stdx
namespace stdx {

using namespace std;

#endif


/// Options struct for polymorphic_value. Using an options struct as a template parameter seems to be the most ergonomic
/// way to set options in C++20. A system of named template parameters is the only way to improve on this as all other systems such
/// as for instance polymorphic_value<T, auto...> requires each option value to be std:: prefixed.
struct polymorphic_value_options {
    size_t size = 128;
    size_t alignment = 0;
    bool heap = true;
    bool copy = true;
    bool move = true;
};

template<typename T, polymorphic_value_options Options = polymorphic_value_options{}> class polymorphic_value {
    // Copies of the options, adjusted for properties of T
    static const size_t sbo_size = Options.heap ? (Options.size >= sizeof(T) ? Options.size : 0) : max(Options.size, sizeof(T));
    static const size_t alignment = max(alignof(T), Options.alignment);
    static const bool allow_heap_allocation = Options.heap;
    static const bool copyable = Options.copy && is_copy_constructible_v<T>;
    static const bool movable = Options.move && is_move_constructible_v<T>;

public:
    polymorphic_value() {}
    polymorphic_value(nullopt_t) {}
    polymorphic_value(const polymorphic_value& src)  {
        std::launder(&src.m_handler)->copy(*this, src.m_data);
    }
    polymorphic_value(polymorphic_value&& src)  {
        std::launder(&src.m_handler)->move(*this, src.m_data);
        src.reset();
    }
    template<typename U, typename... Args> polymorphic_value(in_place_type_t<U>, Args&&... args) requires is_base_of_v<T, U> {
        emplace<U>(forward<Args>(args)...);
    }

    ~polymorphic_value() {
        std::launder(&m_handler)->destroy(m_data);
    }

    // static make function which could be somewhat more ergonomic than the in_place_type constructor, especially after creating a
    // type alias for a certain pointer type.
    template<typename U, typename... Args> static polymorphic_value make(Args&&... args) requires is_base_of_v<T, U> {
        return polymorphic_value(in_place_type<U>, forward<Args>(args)...);
    }

    polymorphic_value& operator=(const polymorphic_value& src) {
        if (this == &src)
            return *this;

        std::launder(&m_handler)->destroy(m_data);
        std::launder(&src.m_handler)->copy(*this, src.m_data);
        return *this;
    };

    polymorphic_value& operator=(polymorphic_value&& src) {
        if (this == &src)
            return *this;

        std::launder(&m_handler)->destroy(m_data);
        std::launder(&src.m_handler)->move(*this, src.m_data);
        src.reset();
        return *this;
    };

    // Create object of subclass U of T, or by default a T.
    template<typename U = T, typename... Args> void emplace(Args&&... args) requires is_base_of_v<T, U> {
        static_assert(!copyable || is_copy_constructible_v<U>, "To use a non-copyable subclass the copy option must be set to false");
        static_assert(!movable || is_move_constructible_v<U>, "To use a non-movable subclass the copy option must be set to false");
        static_assert(allow_heap_allocation || sizeof(U) <= sbo_size, "The class does not fit in the polymorphic_value");
        static_assert(alignof(U) <= alignment, "The class has a higher alignment requirement than specified");

        std::launder(&m_handler)->destroy(m_data);
        if constexpr (sizeof(U) <= sbo_size) {
            new(&m_handler) small_handler<U>;
            construct_at(reinterpret_cast<U*>(m_data.m_bytes), forward<Args>(args)...);
        }
        else {
            new(&m_handler) big_handler<U>;
            construct_at(&m_data.m_ptr, make_unique<U>(forward<Args>(args)...));
        }
    }

    // Get rid of a stored object, resetting the handler so that no double delete occurs later and so that operator bool returns false.
    void reset() { std::launder(&m_handler)->destroy(m_data); new(&m_handler) handler_base; }

    operator bool() const { return get() != nullptr; }

    // Access the stored object. This is the unique_ptr API to allow for drop in replacement.
    T* get() { return std::launder(&m_handler)->get(m_data); }
    const T* get() const { return const_cast<polymorphic_value*>(this)->get(); }

    T& operator*() { return *get(); }
    const T& operator*() const { return *get(); }

    T* operator->() { return get(); }
    const T* operator->() const { return get(); }

    // optional API
    // Maybe a holds_alternative<U> from variant is more appropriate? But viewing different subclasses as alternatives seems a bit
    // misleading.
    bool has_value() const { return *this; }
    template<typename U> bool has_value() const { return dynamic_cast<const U*>(get()) != nullptr; }

    // Note: Skip rvalue versions for now.
    template<typename U = T> U& value() {
        U* ret = dynamic_cast<U*>(get());
        if (ret == nullptr)
            throw bad_optional_access();
        return *ret;
    }
    template<typename U = T> const U& value() const {
        U* ret = dynamic_cast<const U*>(get());
        if (ret == nullptr)
            throw bad_optional_access();
        return *ret;
    }
    template<typename U = T, typename V> U value_or(V&& default_value) const {
        const U* ret = dynamic_cast<const U*>(get());
        if (ret != nullptr)
            return *ret;
        else
            return static_cast<U>(forward<V>(default_value));
    }

    // Note: and_then requires F to return optional<X> or polymorphic_value<X> for some type X.
    // This requirement is from optional, but in fact is quite strange, it could be any return type with a value-constructed
    // default, such as a pointer.
    template<typename U = T, typename F> auto and_then(F&& f) {
        using R = remove_cvref_t<invoke_result_t<F, U&>>; // R must be some optional type.
        if (has_value<U>())
            return f(*static_cast<U*>(get()));
        else
            return R();
    }
    template<typename U = T, typename F> auto and_then(F&& f) const {
        using R = remove_cvref_t<invoke_result_t<F, const U&>>;
        if (has_value<U>())
            return f(*static_cast<const U*>(get()));
        else
            return R();
    }

    // transform wraps the return value of F in an optional.
    template<typename U = T, typename F> auto transform(F&& f) {
        using R = remove_cvref_t<invoke_result_t<F, U&>>;
        if (has_value<U>())
            return optional<R>(f(*static_cast<U*>(get())));
        else
            return optional<R>();
    }
    template<typename U = T, typename F> auto transform(F&& f) const {
        using R = remove_cvref_t<invoke_result_t<F, const U&>>;
        if (has_value<U>())
            return optional<R>(f(*static_cast<const U*>(get())));
        else
            return optional<R>();
    }

    // or_else requires F to return some optional.
    template<typename U = T, typename F> auto or_else(F&& f) {
        using R = remove_cvref_t<invoke_result_t<F>>;
        if (has_value<U>())
            return R(*static_cast<U*>(get()));
        else
            return f();
    }
    template<typename U = T, typename F> auto or_else(F&& f) const {
        using R = remove_cvref_t<invoke_result_t<F>>;
        if (has_value<U>())
            return R(*static_cast<const U*>(get()));
        else
            return f();
    }

private:
    union data {
        data() : m_ptr(nullptr) {}
        ~data() {}

        alignas(alignment) byte m_bytes[max(size_t(1), sbo_size)];      // 0 sized arrays not allowed.
        unique_ptr<T> m_ptr;
    };

    // Note: handler_base is not abstract, instead it is used for empty objects.
    struct handler_base {
        virtual void imbue_handler(handler_base& dest) const { new(&dest) handler_base; }

        virtual T* get(data& d) const { return nullptr; }
        virtual const T* get(const data& d) const { return nullptr; }

        virtual void copy(polymorphic_value& dest, const data& src) const {}
        virtual void move(polymorphic_value& dest, data& src) const {}
        virtual void destroy(data& d) const {}
    };

    // Handler for Us that fit the SBO size
    template<typename U> struct small_handler final : public handler_base {
        void imbue_handler(handler_base& dest) const override { }

        T* get(data& d) const override { return static_cast<T*>(reinterpret_cast<U*>(d.m_bytes)); }
        const T* get(const data& d) const override { return static_cast<const T*>(reinterpret_cast<const U*>(d.m_bytes)); }

        void copy(polymorphic_value& dest, const data& src) const override {
            new(&dest.m_handler) handler_base;
            if constexpr (is_copy_constructible_v<U>) // Always true thanks to requires clauses on constructors/assignment operators.
                construct_at<U>(reinterpret_cast<U*>(dest.m_data.m_bytes), *reinterpret_cast<const U*>(src.m_bytes));
            new(&dest.m_handler) small_handler<U>;
        }

        void move(polymorphic_value& dest, data& src) const override {
            new(&dest.m_handler) handler_base;
            if constexpr (is_move_constructible_v<U>)
                construct_at<U>(reinterpret_cast<U*>(dest.m_data.m_bytes), std::move(*reinterpret_cast<U*>(src.m_bytes)));
            new(&dest.m_handler) small_handler<U>;
        }

        void destroy(data& d) const override { destroy_at(reinterpret_cast<U*>(d.m_bytes)); }
    };

    // Handler for Us that don't fit the SBO size
    template<typename U> struct big_handler final : public handler_base {
        void imbue_handler(handler_base& dest) const override { new(&dest) big_handler<U>; }

        T* get(data& d) const override { return d.m_ptr.get(); }
        const T* get(const data& d) const override { return d.m_ptr.get(); }

        void copy(polymorphic_value& dest, const data& src) const override {
            new(&dest.m_handler) handler_base;
            if constexpr (is_copy_constructible_v<U>)
                construct_at(&dest.m_data.m_ptr, make_unique<U>(static_cast<const U&>(*src.m_ptr)));
            new(&dest.m_handler) big_handler<U>;
        }
        void move(polymorphic_value& dest, data& src) const override {
            new(&dest.m_handler) handler_base;
            if constexpr (is_move_constructible_v<U>)
                construct_at(&dest.m_data.m_ptr, std::move(src.m_ptr));
            new(&dest.m_handler) big_handler<U>;
        }

        void destroy(data& d) const override { destroy_at(&d.m_ptr); }
    };

    data m_data;
    handler_base m_handler;     // Should be after m_data to avoid a hole if data has a larger alignment than a pointer.
};

#if 0 // I thought this should work but it gives me std::ranges_dangling from max_element et al.
// Create polymorphic_value_options suitable for a closed set of SubClasses
template<typename... SubClasses> constexpr polymorphic_value_options polymorphic_value_options_for = {
    .size = size_t(*ranges::max_element(initializer_list<size_t>{ sizeof(SubClasses)... })),
    .alignment = *ranges::max_element(initializer_list<size_t>{ alignof(SubClasses)... }),
    .heap = false,
    .copy = ranges::all_of(initializer_list<size_t>{std::is_copy_constructible_v<SubClasses>...}, [](bool x) { return x; }),
    .move = ranges::all_of(initializer_list<size_t>{std::is_move_constructible_v<SubClasses>...}, [](bool x) { return x; })
};
#else

template<typename S, typename... Ss> constexpr polymorphic_value_options polymorphic_value_options_for = {
    .size = max(sizeof(S), polymorphic_value_options_for<Ss...>.size),
    .alignment = max(alignof(S), polymorphic_value_options_for<Ss...>.alignment),
    .heap = false,
    .copy = is_copy_constructible_v<S> && polymorphic_value_options_for<Ss...>.copy,
    .move = is_move_constructible_v<S> && polymorphic_value_options_for<Ss...>.move
};
template<typename S> constexpr polymorphic_value_options polymorphic_value_options_for<S> = {
    .size = sizeof(S),
    .alignment = alignof(S),
    .heap = false,
    .copy = is_copy_constructible_v<S>,
    .move = is_move_constructible_v<S>
};

#endif

// Type alias suitable for a polymorphic_value able to hold all of the subclasses.
template<typename T, typename... SubClasses> using polymorphic_value_for = polymorphic_value<T, polymorphic_value_options_for<SubClasses...>>;


}       // Namespace std or stdx
