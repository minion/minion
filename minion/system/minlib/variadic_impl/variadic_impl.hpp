#ifndef MINLIB_BITS_H_JFIRK
#define MINLIB_BITS_H_JFIRK

// A selection of bits of helpful C++ code.

template<int...> 
struct IntTuple {};

template<int I, typename IntTuple> 
struct MakeIntTuple;

template<int I, int... Vals> 
struct MakeIntTuple<I, IntTuple<Vals...> > 
{ typedef typename MakeIntTuple<I-1, IntTuple<I, Vals...>>::type type; };

template<int... Vals> 
struct MakeIntTuple<-1, IntTuple<Vals...>> 
{ typedef IntTuple<Vals...> type; };

template<typename T>
struct drop_ref<T&>
{ typedef T type; };

template<typename T>
struct drop_ref<const T&>
{ typedef T type; };

template<typename T>
struct drop_ref<T&&>
{ typedef T type; };

template<typename T>
struct drop_ref<const T&&>
{ typedef T type; };


template<template<typename...> class T, typename... Args>
struct JoinFromTuple<T, std::tuple<Args...>>
{ typedef T<Args...> type; };

template<typename... Args1, typename... Args2>
struct TupleJoin<std::tuple<Args1...>, std::tuple<Args2...> >
{ typedef std::tuple<Args1..., Args2...> type; };

template<typename Arg, typename... Args>
struct TupleAddArgFront<std::tuple<Args...>, Arg>
{ typedef std::tuple<Arg, Args...> type; };

template<typename Arg,typename... Args>
struct TupleFront<std::tuple<Arg, Args...> >
{ typedef Arg type; };

template<int i, typename Arg, typename... Args>
struct TupleGrabNargs<std::tuple<Arg, Args...>, i>
{ typedef typename TupleAddArgFront<typename TupleGrabNargs<std::tuple<Args...>, i-1>::type, Arg>::type type; };

template<typename... Args>
struct TupleGrabNargs<std::tuple<Args...>, 0>
{ typedef typename std::tuple<> type; };

// This overload doesn't look that useful, we need it because the compiler can't choose between the
// above two options
template<typename Arg, typename... Args>
struct TupleGrabNargs<std::tuple<Arg,Args...>, 0>
{ typedef typename std::tuple<> type; };

template<int i, typename Arg, typename... Args>
struct TupleDropNargs<std::tuple<Arg,Args...>, i>
{ typedef typename TupleDropNargs<std::tuple<Args...>, i-1>:: type type; };

template<typename... Args>
struct TupleDropNargs<std::tuple<Args...>, 0>
{ typedef std::tuple<Args...> type; };

template<typename Arg, typename... Args>
struct TupleDropNargs<std::tuple<Arg, Args...>, 0>
{ typedef std::tuple<Arg, Args...> type; };

template<>
struct SizeOf<>
{ static const int size = 0; };

template<typename T>
struct SizeOf<T>
{ static const int size = 1; };

template<typename T, typename... Args>
struct SizeOf<T, Args...>
{ static const int size = 1 + SizeOf<Args...>::size; };



template<typename T>
struct removePointer<T*>
{ typedef T type; };

template<typename T>
struct removeAllPointers<T*>
{ typedef typename removeAllPointers<T>::type type; };

template<typename T>
struct removeAllPointers
{ typedef T type; };

template<typename... Args>
struct removePointerFromTupleArgs<std::tuple<Args...>>
{ typedef std::tuple<typename removePointer<Args>::type...> type; };

template<typename T>
struct NoArgHeapConstructor<T, 0>
{
    template<typename... Args>
    T* operator()(const Args&... args) { return new T(args...); }

    template<typename... Args>
    struct Type
    { typedef T* type; };
};

template<typename T>
struct NoArgStackConstructor<T, 0>
{
    template<typename... Args>
    T operator()(const Args&... args) { return T(args...); }

    template<typename... Args>
    struct Type
    { typedef T type; };
};

template<typename Base>
struct CommonType<Base>
{ typedef Base type; };

template<typename Base, typename T>
struct CommonType<Base, T>
{ typedef T type; };

template<typename Base, typename T, typename U, typename... Args>
struct CommonType<Base, T, U, Args...>
{ typedef Base type; };

template<typename Base, typename T, typename... Args>
struct CommonType<Base, T, T, Args...>
{ typedef typename CommonType<Base, T, Args...>::type type; };



#endif

