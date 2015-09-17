/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMMON_H
#define __PCOMMON_H
/*******************************************************************************
 FILE         :   pcommon.h
 COPYRIGHT    :   Yakov Markovitch, 1996-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Common definitions for PCOMMON library

 CREATION DATE:   27 June 1996
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_macros.h>
#include <pcomn_def.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/*******************************************************************************
 Typedefs for different flag set types
*******************************************************************************/
typedef uint64_t  flags64_t ;
typedef uint32_t  flags32_t ;
typedef uint16_t  flags16_t ;
typedef uint8_t   flags8_t ;

typedef uintptr_t flags_t ;

typedef flags32_t bigflag_t ;

struct ___offsTest { char _c ; double _l ; } ;

/*******************************************************************************
 C++ part
*******************************************************************************/
#ifdef __cplusplus
#include <stdexcept>
#include <system_error>

/*******************************************************************************
 Debug macros
*******************************************************************************/
#ifdef PCOMN_COMPILER_GNU
#include <cxxabi.h>
#include <array>
#include <string.h>

namespace pcomn {

inline const char *demangle(const char *mangled, char *buf, size_t buflen)
{
   int status = 0 ;
   return abi::__cxa_demangle(mangled, buf, &buflen, &status) ;
}

template<typename T>
inline const char *demangled_typename_(std::true_type, char *buf, size_t buflen)
{
  demangle(typeid(typename std::add_pointer<typename std::remove_reference<T>::type>::type).name(),
           buf, buflen) ;
  if (buflen > 1)
  {
    const size_t len = strlen(buf) ;
    if (len && buf[len - 1] == '*')
      buf[len - 1] = 0 ;
  }
  return buf ;
}

template<typename T>
inline const char *demangled_typename_(std::false_type, char *buf, size_t buflen)
{
  return demangle(typeid(T).name(), buf, buflen) ;
}

template<typename T>
inline const char *demangled_typename(char *buf, size_t buflen)
{
  return demangled_typename_<T>
    (std::integral_constant<bool,
     std::is_class<typename std::remove_reference<T>::type>::value ||
     std::is_union<typename std::remove_reference<T>::type>::value>(),
     buf, buflen) ;
}
}

#define PCOMN_DEMANGLE(name) (::pcomn::demangle((name), std::array<char, 1024>().begin(), 1024))
#define PCOMN_CLASSNAME(type) (pcomn::demangled_typename<type >(std::array<char, 1024>().begin(), 1024))

#else

namespace pcomn {

template<typename T>
inline const char *demangled_typename_(std::false_type)
{
  return typeid(T).name() ;
}

template<typename T>
inline const char *demangled_typename_(std::true_type)
{
   return demangled_typename_<typename std::add_pointer<typename std::remove_reference<T>::type>::type>(std::false_type()) ;
}

template<typename T>
inline const char *demangled_typename()
{
  return demangled_typename_<T>
    (std::integral_constant<bool,
     std::is_class<typename std::remove_reference<T>::type>::value ||
     std::is_union<typename std::remove_reference<T>::type>::value>()) ;
}

}

#define PCOMN_DEMANGLE(name) (name)
#define PCOMN_CLASSNAME(type) (pcomn::demangled_typename<type >())

#endif

#define PCOMN_TYPENAME(type_or_value) PCOMN_DEMANGLE(typeid(type_or_value).name())

#define PCOMN_DEREFTYPENAME(value) ((value) ? PCOMN_TYPENAME(*(value)) : PCOMN_TYPENAME((value)))

#define HEXOUT(k) "0x" << std::hex << (k) << std::dec
#define PTROUT(k) '[' << (const void *)(k) << ']'
#define EXPROUT(e) #e" = " << (e)
#define STDEXCEPTOUT(x) PCOMN_TYPENAME(x) << "('" << (x).what() << "')"

#if defined(__GLIBC__)
#include <errno.h>
#define PCOMN_PROGRAM_SHORTNAME (program_invocation_short_name)
#define PCOMN_PROGRAM_FULLNAME  (program_invocation_name)
#elif defined(PCOMN_COMPILER_MS) || defined(PCOMN_COMPILER_BORLAND)
#include <string.h>
#ifdef PCOMN_COMPILER_BORLAND
#define __argv _argv
#endif
#define PCOMN_PROGRAM_SHORTNAME (strrchr(*__argv, '\\') ? strrchr(*__argv, '\\') + 1 : *__argv)
#define PCOMN_PROGRAM_FULLNAME  (*__argv)
#endif

/// @macro PCOMN_NONCONST_OFFSETOF(TYPE, MEMBER)
/// offsetof() for non-POD objects
///
/// According to the ISO C++ Standard, offsetof() for non-POD types is incorrect. As a
/// result, some compilers (particularly, GCC 4.x) issue warnings in such cases.
/// While not fatal, it is quite disturbing. To avoid GCC complaints, we don't use NULL
/// as a "straw pointer". I suppose, 256 holds for _any_ alignment requirements.
#if defined(PCOMN_COMPILER_GNU)
#define PCOMN_NONCONST_OFFSETOF(TYPE, MEMBER)                           \
   (static_cast<size_t>(                                                \
      reinterpret_cast<size_t>(                                         \
         &reinterpret_cast <const volatile char &>(reinterpret_cast<TYPE *>(256)->MEMBER))) - 256)
#else
#define PCOMN_NONCONST_OFFSETOF(TYPE, MEMBER) (offsetof(TYPE, MEMBER))
#endif

/*******************************************************************************
 pcomn namespace
*******************************************************************************/
namespace pcomn {

/******************************************************************************/
/** Base class for boolean tags
*******************************************************************************/
struct bool_value {
      constexpr bool_value() : _value(false) {}
      explicit constexpr bool_value(bool value) : _value(value) {}
      explicit constexpr operator bool() const { return _value ; }
   private:
      const bool _value ;
} ;

/******************************************************************************/
/** A tag type to specify whether to raise exception on error for functions
 that allow to indicate failure with a special return value.
*******************************************************************************/
struct RaiseError : bool_value { using bool_value::bool_value ; } ;

static const RaiseError DONT_RAISE_ERROR (false) ;
static const RaiseError RAISE_ERROR      (true) ;


/******************************************************************************/
/** Not-a-pointer: something, which is both not NULL and is not a valid pointer
*******************************************************************************/
template<typename T = void>
struct not_a_pointer { static constexpr T * const value = reinterpret_cast<T *>(~(intptr_t())) ; } ;

template<typename T>
constexpr T * const not_a_pointer<T>::value ;

constexpr void * const NaP = not_a_pointer<>::value ;

/*******************************************************************************
 Argument-checking exception-throw macros
*******************************************************************************/
/// Check that an argument is nonzero and throw std::invalid_argument otherwise.
/// @ingroup ExceptionMacros
/// @param arg
/// @return @a arg value.
/// @note Expression !(arg) should be valid and should return some @em unspecified-boolean-type.
/// @throw std::invalid_argument
///
/// Since the macro returns the value of its argument, it can be used as a 'filter', e.g.
/// @code
/// int bar(void *data, size_t length) ;
/// int foo(const char *str)
/// {
///    return bar(str, strlen(PCOMN_ENSURE_ARG(str))) ;
/// }
/// @endcode
#define PCOMN_ENSURE_ARG(arg) (::pcomn::ensure_arg<std::invalid_argument>((arg), #arg, __FUNCTION__))

#define PCOMN_ENSURE_ARGX(arg, exception) (::pcomn::ensure_arg<exception>((arg), #arg, __FUNCTION__))

#define PCOMN_ASSERT_ARG(assertion) \
   (::pcomn::ensure_arg_assertion<std::invalid_argument>((assertion), #assertion, __FUNCTION__))

#define PCOMN_ASSERT_ARGX(assertion, exception) \
   (::pcomn::ensure_arg_assertion<exception>((assertion), #assertion, __FUNCTION__))

/*******************************************************************************
 Out-of-line exception throw
*******************************************************************************/
template<class X, typename... XArgs>
__noreturn __noinline
void throw_exception(XArgs&& ...args) { throw X(std::forward<XArgs>(args)...) ; }

template<typename Msg>
__noreturn __noinline
void throw_system_error(std::errc errcode, const Msg &msg)
{
   throw_exception<std::system_error>(std::make_error_code(errcode), msg) ;
}

template<typename Msg>
__noreturn __noinline
void throw_system_error(int errno_code, const Msg &msg)
{
   throw_exception<std::system_error>(errno_code, std::system_category(), msg) ;
}

template<class X, typename... XArgs>
inline void conditional_throw(bool test, XArgs && ...args)
{
   if (unlikely(test))
      throw_exception<X>(std::forward<XArgs>(args)...) ;
}

template<class X, typename V, typename... XArgs>
inline void ensure(V &&value, XArgs && ...args)
{
   conditional_throw<X>(!value, std::forward<XArgs>(args)...) ;
}

/// Check whether the value is not zero.
/// If !value is true, throw X.
template<class X, typename V, typename... XArgs>
inline V &&ensure_nonzero(V &&value, XArgs && ...args)
{
   ensure<X>(std::forward<V>(value), std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

namespace detail {
template<typename X>
__noinline __noreturn
void throw_arg_null(const char *arg_name, const char *function_name)
{
   char message[512] ;
   const char *aname = arg_name, *quote = "'" ;
   if (unlikely(!arg_name))
      aname = quote = "" ;
   snprintf(message, sizeof message, "Invalid (NULL) argument %s%s%s%s%s.",
            quote, aname, quote, function_name ? " is passed to " : "", function_name ? function_name : "") ;
   throw_exception<X>(message) ;
}

template<typename X>
__noinline __noreturn
void throw_arg_assert(const char *assertion_text, const char *function_name)
{
   char message[512] ;
   if (unlikely(!assertion_text))
      assertion_text = "" ;
   if (unlikely(!function_name))
      function_name = "" ;
   snprintf(message, sizeof message, "Arguments assertion '%s' failed%s%s.",
            assertion_text, *function_name ? " in " : "", function_name) ;
   throw_exception<X>(message) ;
}
}

template<class X, typename V>
inline V &&ensure_arg(V &&value, const char *arg_name, const char *function_name)
{
   if (unlikely(!value))
      detail::throw_arg_null<X>(arg_name, function_name) ;
   return std::forward<V>(value) ;
}

template<class X>
inline void ensure_arg_assertion(bool assertion, const char *assertion_text, const char *function_name)
{
   if (unlikely(!assertion))
      detail::throw_arg_assert<X>(assertion_text, function_name) ;
}

/// Ensure that a value is between specified minimum and maximum (inclusive).
/// @throw X(message) if value is out of range.
template<class X, typename V, typename B, typename... XArgs>
inline V &&ensure_range(V &&value, B &&minval, B &&maxval, XArgs && ...args)
{
   ensure<X>(!(value < minval || maxval < value), std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

template<class X, typename V, typename B, typename... XArgs>
inline V &&ensure_lt(V &&value, B &&bound, XArgs && ...args)
{
   ensure<X>(value < bound, std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

template<class X, typename V, typename B, typename... XArgs>
inline V &&ensure_le(V &&value, B &&bound, XArgs && ...args)
{
   ensure<X>(!(bound < value), std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

template<class X, typename V, typename B, typename... XArgs>
inline V &&ensure_gt(V &&value, B &&bound, XArgs && ...args)
{
   ensure<X>(bound < value, std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

template<class X, typename V, typename B, typename... XArgs>
inline V &&ensure_ge(V &&value, B &&bound, XArgs && ...args)
{
   ensure<X>(!(value < bound), std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

template<class X, typename V, typename B, typename... XArgs>
inline V &&ensure_eq(V &&value, B &&bound, XArgs && ...args)
{
   ensure<X>(value == bound, std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

template<class X, typename V, typename B, typename... XArgs>
inline V &&ensure_ne(V &&value, B &&bound, XArgs && ...args)
{
   ensure<X>(!(value == bound), std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

template<typename M>
inline void ensure_precondition(bool precondition, const M &message)
{
   ensure<std::invalid_argument>(precondition, message) ;
}

/******************************************************************************/
/** A destruction policy for use by std::unique_ptr for objects allocated
 with malloc

 Uses ::free() to deallocate memory.
*******************************************************************************/
struct malloc_delete {
      void operator()(const void *ptr) const noexcept { ::free(const_cast<void *>(ptr)) ; }
} ;

/*******************************************************************************
 friend_cast<>
 Hack which allows public access to protected class members.
 Works for most sane compilers (at least for all widely used).
*******************************************************************************/
/// @cond
namespace detail {
template <class Friend, class Befriended>
class FriendOf : public Befriended { friend Friend ; } ;
} // end of namespace pcomn::detail
/// @endcond
} // end of namespace pcomn

template <class Friend, class T>
inline pcomn::detail::FriendOf<Friend, T> *friend_cast(T *object)
{
   return static_cast<pcomn::detail::FriendOf<Friend, T> *>(object) ;
}

template <class Friend, class T>
inline pcomn:: detail::FriendOf<Friend, T> *friend_cast(T *object, const Friend &)
{
   return friend_cast<Friend>(object) ;
}

constexpr int P_CURRENT_ALIGNMENT = sizeof (___offsTest)-sizeof(double) ;

/*******************************************************************************

*******************************************************************************/
#define PCOMN_NONCOPYABLE(Class)   Class(const Class&) = delete
#define PCOMN_NONASSIGNABLE(Class) void operator=(const Class&) = delete

/// Given that there is '<' operator for the type, define all remaining ordering operators
/// as global functions.
#define PCOMN_DEFINE_ORDER_FUNCTIONS(pfx, type)                         \
   pfx inline bool operator>(const type &lhs, const type &rhs) { return rhs < lhs ; } \
   pfx inline bool operator<=(const type &lhs, const type &rhs) { return !(rhs < lhs) ; } \
   pfx inline bool operator>=(const type &lhs, const type &rhs) { return !(lhs < rhs) ; }

/// Given that there are operators '==' and '<' for the type, define through them all
/// remaining relational operators as global functions.
#define PCOMN_DEFINE_RELOP_FUNCTIONS(pfx, type)                         \
   pfx inline bool operator!=(const type &lhs, const type &rhs) { return !(lhs == rhs) ; } \
   PCOMN_DEFINE_ORDER_FUNCTIONS(P_PASS(pfx), P_PASS(type))

/// Given that there is '<' operator for the type, define all remaining ordering operators
/// as type methods.
#define PCOMN_DEFINE_ORDER_METHODS(type)                          \
   bool operator>(const type &rhs) { return rhs < *this ; }       \
   bool operator<=(const type &rhs) { return !(rhs < *this) ; }   \
   bool operator>=(const type &rhs) { return !(*this < rhs) ; }

/// Given that there are operators '==' and '<' for the type, define through them all
/// remaining relational operators as type methods.
#define PCOMN_DEFINE_RELOP_METHODS(type)                          \
   bool operator!=(const type &rhs) { return !(*this == rhs) ; }  \
   PCOMN_DEFINE_ORDER_METHODS(P_PASS(type))

/// Define operators '+' and '-' for the type through corresponding augmented operations
/// '+=' and '-='.
#define PCOMN_DEFINE_ADDOP_FUNCTIONS(pfx, type)                         \
   pfx inline type operator+(const type &lhs, const type &rhs) { type ret (lhs) ; return std::move(ret += rhs) ; } \
   pfx inline type operator-(const type &lhs, const type &rhs) { type ret (lhs) ; return std::move(ret -= rhs) ; }

#define PCOMN_DEFINE_NONASSOC_ADDOP_FUNCTIONS(pfx, type, rhstype)       \
   pfx inline type operator+(const type &lhs, const rhstype &rhs) { type ret (lhs) ; return std::move(ret += rhs) ; } \
   pfx inline type operator-(const type &lhs, const rhstype &rhs) { type ret (lhs) ; return std::move(ret -= rhs) ; }

/// Define operators '+' and '-' for the type through corresponding augmented operations
/// '+=' and '-='.
#define PCOMN_DEFINE_ADDOP_METHODS(type)                                \
   type operator+(const type &rhs) const { type ret (*this) ; return std::move(ret += rhs) ; } \
   type operator-(const type &rhs) const { type ret (*this) ; return std::move(ret -= rhs) ; }

#define PCOMN_DEFINE_NONASSOC_ADDOP_METHODS(type, rhstype)              \
   type operator+(const rhstype &rhs) const { type ret (*this) ; return std::move(ret += rhs) ; } \
   type operator-(const rhstype &rhs) const { type ret (*this) ; return std::move(ret -= rhs) ; }

/// Define post(inc|dec)rement.
#define PCOMN_DEFINE_POSTCREMENT(type, op) \
   type operator op(int) { type result (*this) ; op *this ; return result ; }

/// Define both postincrement and postdecrement.
#define PCOMN_DEFINE_POSTCREMENT_METHODS(type) \
   PCOMN_DEFINE_POSTCREMENT(type, ++)          \
   PCOMN_DEFINE_POSTCREMENT(type, --)

/// Providing there is @a type ::swap, define namespace-level swap overload @a type
#define PCOMN_DEFINE_SWAP(type, ...) \
   __VA_ARGS__ inline void swap(type &lhs, type &rhs) { lhs.swap(rhs) ; }

/// For a type without state, define operators '==' and '!=' that are invariantly 'true'
/// and 'false' respectively
///
/// I.e. we actually say "since object of type T is, in fact, just a namespace, consider
/// any two T objects equal".
#define PCOMN_DEFINE_INVARIANT_EQ(prefix, type)                         \
   prefix inline constexpr bool operator==(const type &, const type &) { return true ; } \
   prefix inline constexpr bool operator!=(const type &, const type &) { return false ; }

#define PCOMN_DEFINE_INVARIANT_PRINT(prefix, type)                      \
   prefix inline std::ostream &operator<<(std::ostream &os, const type &) { return os << PCOMN_CLASSNAME(type) ; }

#define PCOMN_DEFINE_PRANGE(type, ...)                                  \
   __VA_ARGS__ inline auto pbegin(type &x) -> decltype(&*std::begin(x)) { return std::begin(x) ; } \
   __VA_ARGS__ inline auto pbegin(const type &x) -> decltype(&*std::begin(x)) { return std::begin(x) ; } \
   __VA_ARGS__ inline auto pend(type &x) -> decltype(&*std::end(x)) { return std::end(x) ; } \
   __VA_ARGS__ inline auto pend(const type &x) -> decltype(&*std::end(x)) { return std::end(x) ; }

/******************************************************************************/
/** Swap wrapper that calls std::swap on the arguments, but may also use ADL
 (Argument-Dependent Lookup, Koenig lookup) to use a specialised form.

 Use this instead of std::swap to enable namespace-level swap functions, defined in the
 same namespaces as classes they defined for. Note that if there is no such function
 for T, this wrapper will fallback to std::swap().
*******************************************************************************/
template<typename T>
inline void pcomn_swap(T &a, T &b)
{
   using std::swap ;
   swap(a, b) ;
}

#else    // no __cplusplus

#define _max(a,b)    (((a) > (b)) ? (a) : (b))
#define _min(a,b)    (((a) < (b)) ? (a) : (b))

#define P_CURRENT_ALIGNMENT (sizeof (___offsTest)-sizeof(double))

#endif   // __cplusplus

#endif /* __PCOMMON_H */
