#ifndef VARIADIC_HPP_CDACDCDS
#define VARIADIC_HPP_CDACDCDS

#include <tuple>

/** \weakgroup MinLib
 * @{
 */

template <typename T>
void VariadicForEach(T&) {}

template <typename T, typename Arg, typename... Args>
void VariadicForEach(T& t, Arg&& arg, Args&&... args) {
  t(arg);
  VariadicForEach(t, args...);
}

/// Given Tuple = std::tuple<Args...>, JoinFromTuple<T,Tuple>::type = T<Args...>
template <template <typename...> class T, typename Tuple>
struct JoinFromTuple;

/// Given Tuple = std::tuple<Args...>,
/// TupleAddArgFront<Tuple, NewArg>::type = std::tuple<NewArg, Args...>
template <typename Tuple, typename NewArg>
struct TupleAddArgFront;

/// Given Tuple1 = std::tuple<Args1...>, Tuple2 = std::tuple<Args2...>,
/// TupleJoin<Tuple1,Tuple2>::type = std::tuple<Args1..., Args2...>
template <typename Tuple1, typename Tuple2>
struct TupleJoin;

/// Given Tuple = std::tuple<Arg1, Args...>, TupleFront<Tuple>::type = Arg1
template <typename Tuple>
struct TupleFront;

/// Given Tuple = std::tuple<Arg1,...,Argn, Args...>,
/// TupleGrabNargs<std::tuple<Arg1,...,Argn, Args...>>::type =
/// std::tuple<Arg1,...,Argn>
template <typename Tuple, int n>
struct TupleGrabNargs;

/// Given Tuple = std::tuple<Arg1,...,Argn, Args...>,
/// TupleGrabNargs<std::tuple<Arg1,...,Argn, Args...>>::type =
/// std::tuple<Args...>
template <typename Tuple, int n>
struct TupleDropNargs;

/// A stupid special case of NoArgStackConstraint, for non-templated types
template <typename, int>
struct NoArgStackConstructor;

// A stupid special case of TemplateHeapConstructor, for non-templated types
template <typename, int>
struct NoArgHeapConstructor;

/// A way to join together typelists, works around a bug in g++4.4
template <template <typename...> class T, typename... Args>
struct TypeJoin {
  typedef T<Args...> type;
};

/// Turns T&, const T&, T&& and const T&& all into just T
template <typename T>
struct drop_ref {
  typedef T type;
};

template <typename T>
struct addPointer {
  typedef T* type;
};

template <typename T>
struct removePointer;

template <typename T>
struct removeAllPointers;

template <typename T>
struct removePointerFromTupleArgs;

template <typename Base, typename... Args>
struct CommonType;

template <typename... Args>
struct SizeOf;

/// This is just for template tomfollowery
template <typename T>
T instance();

#include "variadic_impl/variadic_impl.hpp"

// We could use ArgCount = sizeof the argument list for T, but some
// of the arguments to T might be defaulted, and we have no way of
// counting those.
template <template <typename...> class T, int ArgCount>
struct TemplateStackConstructor {
  template <typename... Args>
  struct Type {
    typedef typename JoinFromTuple<T, typename removePointerFromTupleArgs<typename TupleGrabNargs<
                                          std::tuple<Args...>, ArgCount>::type>::type>::type type;
  };

  template <typename... Args>
  typename Type<Args...>::type operator()(const Args&... args) {
    return typename Type<Args...>::type(args...);
  }
};

// We could use ArgCount = sizeof the argument list for T, but some
// of the arguments to T might be defaulted, and we have no way of
// counting those.
template <template <typename...> class T, int ArgCount>
struct TemplateStackConstructorAllArgs {
  template <typename... Args>
  struct Type {
    typedef typename JoinFromTuple<
        T, typename TupleJoin<
               typename removePointerFromTupleArgs<
                   typename TupleGrabNargs<std::tuple<Args...>, ArgCount>::type>::type,
               typename TupleDropNargs<std::tuple<Args...>, ArgCount>::type>::type>::type type;
  };

  template <typename... Args>
  typename Type<Args...>::type operator()(const Args&... args) {
    return typename Type<Args...>::type(args...);
  }
};

template <typename T, int i>
NoArgStackConstructor<T, i> getStackConstructor();

template <template <typename...> class T, int i>
TemplateStackConstructor<T, i> getStackConstructor();

template <typename T, int i>
NoArgStackConstructor<T, i> getStackAllArgsConstructor();

template <template <typename...> class T, int i>
TemplateStackConstructorAllArgs<T, i> getStackAllArgsConstructor();

// We could use ArgCount = sizeof the argument list for T, but some of the
// arguments to T might be defaulted, and we have no way of counting those.
template <template <typename...> class T, int ArgCount>
struct TemplateHeapConstructor {
  template <typename... Args>
  struct NoPtrType {
    typedef typename JoinFromTuple<T, typename removePointerFromTupleArgs<typename TupleGrabNargs<
                                          std::tuple<Args...>, ArgCount>::type>::type>::type type;
  };

  template <typename... Args>
  struct Type {
    typedef typename NoPtrType<Args...>::type* type;
  };

  template <typename... Args>
  typename Type<Args...>::type operator()(const Args&... args) {
    typedef typename TemplateHeapConstructor::template NoPtrType<Args...>::type type;
    return new type(args...);
  }
};

// We could use ArgCount = sizeof the argument list for T, but some of the
// arguments to T might be defaulted, and we have no way of counting those.
template <template <typename...> class T, int ArgCount>
struct TemplateHeapConstructorAllArgs {
  template <typename... Args>
  struct NoPtrType {
    typedef typename JoinFromTuple<
        T, typename TupleJoin<
               typename removePointerFromTupleArgs<
                   typename TupleGrabNargs<std::tuple<Args...>, ArgCount>::type>::type,
               typename TupleDropNargs<std::tuple<Args...>, ArgCount>::type>::type>::type type;
  };
  template <typename... Args>
  struct Type {
    typedef typename NoPtrType<Args...>::type* type;
  };

  template <typename... Args>
  typename Type<Args...>::type operator()(const Args&... args) {
    typedef typename TemplateHeapConstructorAllArgs::template NoPtrType<Args...>::type type;
    return new type(args...);
  }
};

template <typename T, int i>
NoArgHeapConstructor<T, i> getHeapConstructor();

template <template <typename...> class T, int i>
TemplateHeapConstructor<T, i> getHeapConstructor();

template <typename T, int i>
NoArgHeapConstructor<T, i> getHeapAllArgsConstructor();

template <template <typename...> class T, int i>
TemplateHeapConstructorAllArgs<T, i> getHeapAllArgsConstructor();

/// Some code for variadic templates. The use of this is that:
/// MakeIntList<5>::type is IntTuple<0,1,2,3,4>
template <int Size>
struct MakeIntList : MakeIntTuple<Size - 1, IntTuple<>> {};

template <class Constructor, typename... Args>
class GenericFactory {
  std::tuple<Args...> t;

public:
  GenericFactory(const Args&... args) : t(std::make_tuple(args...)) {}

private:
  template <typename... CArgs, int... Val>
  typename Constructor::template Type<Args..., CArgs...>::type impl(IntTuple<Val...>,
                                                                    const CArgs&... cargs) {
    Constructor c;
    return c(std::get<Val>(t)..., cargs...);
  }

public:
  template <typename... CArgs>
  typename Constructor::template Type<Args..., CArgs...>::type operator()(const CArgs&... cargs) {
    return impl(typename MakeIntList<SizeOf<Args...>::size>::type(), cargs...);
  }
};

// I want to write this, but not until g++ 4.7...
// template<typename Factory, typename... Args>
// using ObjectResult = decltype(instance<typename
// removeAllPointers<Factory>::type>()(instance<Args>()...));

template <typename Factory, typename... Args>
struct ObjectResult {
  typedef decltype(instance<typename removeAllPointers<Factory>::type>()(instance<Args>()...)) type;
};

template <template <typename...> class T>
T<> NoArgs();

template <typename T>
T NoArgs();

// Annoying part of C++, part x of many.
// There is no way of defining both template<typename T = int> struct S{}; and
// struct P; in a consistent manner.
// As we have to write S<> s; but P p; NOARG_HACK(S) s; and NOARG_HACK(P) p;
// both work.
#define NOARG_HACK(T) decltype(NoArgs<T>())

/** @}
 */

#endif
