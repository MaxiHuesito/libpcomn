/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_STRING_H
#define __PCOMN_STRING_H
/*******************************************************************************
 FILE         :   pcomn_string.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   String traits and string shim functions.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Jun 2006
*******************************************************************************/
/**@file
 String traits and string shim functions.
 @par
 String traits describe various aspects of string behaviour for heterogenous string
 classes/types.
 @par
 String "shim" functions "normalize" different string interfaces to a common interface.
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_hash.h>
#include <pcomn_meta.h>
#include <pcomn_algorithm.h>

#include <array>

#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <iostream>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <cctype>
#include <cwctype>

#define PCOMN_BUFPRINTF(size, format, ...) \
   (pcomn::bufprintf(std::array<char, (size + 1) >().data(), (size) + 1, (format), ##__VA_ARGS__))

namespace pcomn {

typedef std::vector<std::string> string_vector ;

/*******************************************************************************
                     template<typename C>
                     struct ctype_traits
*******************************************************************************/
template<typename C>
struct ctype_traits { typedef char undefined ; } ;

template<>
struct ctype_traits<char> {
      typedef char char_type ;
      typedef unsigned char uchar_type ;

      static char tolower(char c) { return std::tolower(c) ; }
      static char toupper(char c) { return std::toupper(c) ; }

      static bool isalnum(char_type c) { return std::isalnum(c) ; }
      static bool isalpha(char_type c) { return std::isalpha(c) ; }
      static bool iscntrl(char_type c) { return std::iscntrl(c) ; }
      static bool isdigit(char_type c) { return std::isdigit(c) ; }
      static bool isgraph(char_type c) { return std::isgraph(c) ; }
      static bool islower(char_type c) { return std::islower(c) ; }
      static bool isprint(char_type c) { return std::isprint(c) ; }
      static bool ispunct(char_type c) { return std::ispunct(c) ; }
      static bool isspace(char_type c) { return std::isspace(c) ; }
      static bool isupper(char_type c) { return std::isupper(c) ; }
      static bool isxdigit(char_type c) { return std::isxdigit(c) ; }

      static char *strncpy(char *dest, const char *src, size_t n)
      { return ::strncpy(dest, src, n) ; }

      static bool strcmp(const char *left, const char *right)
      { return ::strcmp(left, right) ; }
} ;

template<>
struct ctype_traits<wchar_t> {
      typedef wchar_t char_type ;
      typedef std::conditional_t<!std::is_signed<wchar_t>::value, wchar_t,
                                 std::conditional_t<(sizeof(wchar_t) < sizeof(unsigned)), unsigned short, unsigned> >
      uchar_type ;

      static char_type tolower(char_type c) { return std::towlower(c) ; }
      static char_type toupper(char_type c) { return std::towupper(c) ; }

      static bool isalnum(char_type c) { return std::iswalnum(c) ; }
      static bool isalpha(char_type c) { return std::iswalpha(c) ; }
      static bool iscntrl(char_type c) { return std::iswcntrl(c) ; }
      static bool isdigit(char_type c) { return std::iswdigit(c) ; }
      static bool isgraph(char_type c) { return std::iswgraph(c) ; }
      static bool islower(char_type c) { return std::iswlower(c) ; }
      static bool isprint(char_type c) { return std::iswprint(c) ; }
      static bool ispunct(char_type c) { return std::iswpunct(c) ; }
      static bool isspace(char_type c) { return std::iswspace(c) ; }
      static bool isupper(char_type c) { return std::iswupper(c) ; }
      static bool isxdigit(char_type c) { return std::iswxdigit(c) ; }

      static wchar_t *strncpy(wchar_t *dest, const wchar_t *src, size_t n)
      { return ::wcsncpy(dest, src, n) ; }

      static bool strcmp(const wchar_t *left, const wchar_t *right)
      { return ::wcscmp(left, right) ; }
} ;

/*******************************************************************************
                     template<typename C>
                     struct cstring_traits
*******************************************************************************/
template<typename C>
struct cstring_traits {
      typedef C               char_type ;
      typedef C *             type ;
      typedef const C *       const_type ;
      typedef size_t          size_type ;
      // The definition of hash_type is essentially not necessary, but allows to
      // employ SFINAE while defining hasher functions
      typedef size_t          hash_type ;

      PCOMN_CTVALUE(bool, has_std_read, false) ;
      PCOMN_CTVALUE(bool, has_std_write, false) ;

      static size_type len(const_type s) { return std::char_traits<char_type>::length(s) ; }
      static constexpr const char_type *cstr(const_type s) { return s ; }
      static hash_type hash(const_type s) { return hashFNV(s, len(s)) ; }
} ;

/*******************************************************************************
                     template<typename S, bool immutable>
                     struct stdstring_traits
*******************************************************************************/
template<typename S, bool immutable>
struct stdstring_traits {
      typedef S                           type ;
      typedef const S                     const_type ;
      typedef typename type::value_type   char_type ;
      typedef typename type::size_type    size_type ;
      typedef size_t                      hash_type ;

      PCOMN_CTVALUE(bool, has_std_read, true) ;
      PCOMN_CTVALUE(bool, has_std_write, !immutable) ;

      static size_type len(const_type &s) { return s.size() ; }
      static const char_type *cstr(const_type &s) { return s.c_str() ; }
      static hash_type hash(const_type &s) { return hashFNV(cstr(s), len(s) * sizeof(char_type)) ; }
} ;


/*******************************************************************************/
/** String traits for any string with c_str() member.
  Assumes c_str() return pointer to zero-delimited chartype sequence, which
  correctly represent the string.
*******************************************************************************/
template<typename Str, typename Char, typename Size = size_t>
struct anystring_traits {
      typedef Char      char_type ;
      typedef Str       type ;
      typedef const Str const_type ;
      typedef Size      size_type ;
      // The definition of hash_type is essentially not necessary, but allows to
      // employ SFINAE while defining hasher functions
      typedef size_t    hash_type ;

      PCOMN_CTVALUE(bool, has_std_read, false) ;
      PCOMN_CTVALUE(bool, has_std_write, false) ;

      static size_type len(const_type &s) { return std::char_traits<char_type>::length(cstr(s)) ; }
      static constexpr const char_type *cstr(const_type &s) { return s.c_str() ; }
      static hash_type hash(const_type &s) { return hashFNV(cstr(s), len(s)) ; }
} ;

/******************************************************************************/
/** Safe/smart pointer to char, interpreted as a C string
*******************************************************************************/
template<typename P>
struct pstring_traits {
      typedef typename std::remove_reference<decltype(*P().get())>::type  char_type ;
      typedef P               type ;
      typedef const type      const_type ;
      typedef size_t          size_type ;
      typedef size_t          hash_type ;

      PCOMN_CTVALUE(bool, has_std_read, false) ;
      PCOMN_CTVALUE(bool, has_std_write, false) ;

      static size_type len(const type &s) { return std::char_traits<char_type>::length(cstr(s)) ; }
      static constexpr const char_type *cstr(const type &s) { return s.get() ; }
      static hash_type hash(const type &s) { return hashFNV(cstr(s), len(s)) ; }
} ;

/*******************************************************************************
                     template<typename S>
                     struct string_traits
*******************************************************************************/
template<typename S>
struct string_traits { typedef char undefined ; } ;

template<typename S>
struct string_traits<S &> : string_traits<S> {} ;

template<>
struct string_traits<char *> : cstring_traits<char> {} ;
template<>
struct string_traits<const char *> : cstring_traits<char> {} ;
template<>
struct string_traits<wchar_t *> : cstring_traits<wchar_t> {} ;
template<>
struct string_traits<const wchar_t *> : cstring_traits<wchar_t> {} ;

template<size_t N>
struct string_traits<char [N]> : cstring_traits<char> {} ;
template<size_t N>
struct string_traits<const char [N]> : cstring_traits<char> {} ;
template<>
struct string_traits<char[]> : cstring_traits<char> {} ;
template<>
struct string_traits<const char[]> : cstring_traits<char> {} ;
template<size_t N>
struct string_traits<wchar_t [N]> : cstring_traits<wchar_t> {} ;
template<size_t N>
struct string_traits<const wchar_t [N]> : cstring_traits<wchar_t> {} ;
template<typename Char, class Traits, class A>
struct string_traits<std::basic_string<Char, Traits, A> > :
         stdstring_traits<std::basic_string<Char, Traits, A>, false> {} ;

template<typename D>
struct string_traits<std::unique_ptr<char[], D> > : pstring_traits<std::unique_ptr<char[], D> > {} ;
template<typename D>
struct string_traits<std::unique_ptr<const char[], D> > : pstring_traits<std::unique_ptr<const char[], D> > {} ;

/// @cond
namespace detail {
template<typename S>
static typename string_traits<S>::undefined _is_string(S**) ;
template<typename S>
static typename string_traits<S>::type *_is_string(S**) ;

template<typename S, typename C, bool IsStrng> struct
_is_strchar : public std::false_type {} ;
template<typename S, typename C> struct
_is_strchar<S, C, true> : public std::is_same<typename string_traits<S>::char_type, C> {} ;

template<typename S1, typename S2, bool bothStrings> struct
_is_compatible_str : public std::false_type {} ;
template<typename S1, typename S2> struct
_is_compatible_str<S1, S2, true> :
         public std::is_same<typename string_traits<S1>::char_type,
                             typename string_traits<S2>::char_type> {} ;

} // end of namespace pcomn::detail
/// @endcond

template<typename C> struct
is_char : bool_constant<std::is_same<C, char>::value || std::is_same<C, wchar_t>::value> {} ;

template<typename S> struct
is_string : bool_constant<sizeof detail::_is_string(autoval<S**>()) == sizeof(void *)> {} ;

template<typename S, typename C> struct
is_strchar : detail::_is_strchar<S, C, is_string<S>::value> {} ;

template<typename S, typename C> struct
is_string_or_char : bool_constant<is_char<C>::value || is_strchar<S, C>::value> {} ;

template<typename S1, typename S2> struct
is_compatible_strings : detail::_is_compatible_str<S1, S2, is_string<S1>::value && is_string<S2>::value> {} ;

template<typename S, typename Type> struct
enable_if_string : std::enable_if<is_string<S>::value, Type> {} ;

template<typename S, typename Type> struct
disable_if_string : std::enable_if<!is_string<S>::value, Type> {} ;

template<typename S, typename Char, typename Type> struct
enable_if_strchar : std::enable_if<is_strchar<S, Char>::value, Type> {} ;

template<typename S, typename Char, typename Type> struct
disable_if_strchar : std::enable_if<!is_strchar<S, Char>::value, Type> {} ;

template<typename S, typename Other, typename Type> struct
enable_if_other_string : std::enable_if<is_string<Other>::value && !std::is_base_of<S, Other>::value, Type> {} ;

template<typename S1, typename S2, typename Type> struct
enable_if_compatible_strings : std::enable_if<is_compatible_strings<S1, S2>::value, Type> {} ;

template<typename S, typename T>
using enable_if_string_t = typename enable_if_string<S, T>::type ;

template<typename S, typename T>
using disable_if_string_t = typename disable_if_string<S, T>::type ;

template<typename S, typename C, typename T>
using enable_if_strchar_t = typename enable_if_strchar<S, C, T>::type ;

template<typename S, typename C, typename T>
using disable_if_strchar_t = typename disable_if_strchar<S, C, T>::type ;

template<typename S, typename O, typename T>
using enable_if_other_string_t = typename enable_if_other_string<S, O, T>::type ;

template<typename S1, typename S2, typename T>
using enable_if_compatible_strings_t = typename enable_if_compatible_strings<S1, S2, T>::type ;

/*******************************************************************************
 pcomn::str
*******************************************************************************/
namespace str {

template<typename S>
inline constexpr const typename ::pcomn::string_traits<S>::char_type *cstr(const S &str)
{
   return ::pcomn::string_traits<S>::cstr(str) ;
}

template<typename S>
inline typename ::pcomn::string_traits<S>::size_type len(const S &str)
{
   return ::pcomn::string_traits<S>::len(str) ;
}

/// @cond
namespace detail {
template<typename S, typename C>
inline std::basic_string<C> stdstr_(const S &str, ::pcomn::string_traits<std::basic_string<C> > *)
{
   return std::basic_string<C>(str) ;
}
template<typename S>
inline std::basic_string<typename ::pcomn::string_traits<S>::char_type>
stdstr_(const S &str, void *)
{
   return std::basic_string<typename ::pcomn::string_traits<S>::char_type>(cstr(str), len(str)) ;
}
}
/// @endcond

template<typename S>
inline std::basic_string<typename ::pcomn::string_traits<S>::char_type>
stdstr(const S &str)
{
   return detail::stdstr_(str, (::pcomn::string_traits<S> *)NULL) ;
}

template<typename S>
inline typename ::pcomn::string_traits<S>::char_type *
strnew(const S &str)
{
   typedef typename ::pcomn::string_traits<S>::char_type char_type ;
   const char_type *src = cstr(str) ;
   if (!src)
      return NULL ;
   const size_t sz = len(str) ;
   char_type *result = new char_type[sz + 1] ;
   memcpy(result, src, sz * sizeof(char_type)) ;
   result[sz] = 0 ;
   return result ;
}

template<typename S>
inline typename ::pcomn::string_traits<S>::hash_type hash(const S &str)
{
   return ::pcomn::string_traits<S>::hash(str) ;
}

template<typename S>
inline typename ::pcomn::enable_if_string<S, bool>::type is_empty(const S &str)
{
   return !len(str) ;
}

template<typename T, typename U>
inline std::enable_if_t<pcomn::is_compatible_strings<T, U>::value, bool>
is_equal(const T &lhs, const U &rhs)
{
   typedef typename pcomn::string_traits<T>::char_type char_type ;
   const char_type * const lc = pcomn::str::cstr(lhs) ;
   const char_type * const rc = pcomn::str::cstr(rhs) ;
   size_t length ;
   return
      lc == rc ||
      (length = len(lhs)) == len(rhs) && !std::char_traits<char_type>::compare(lc, rc, length) ;
}

template<typename T, typename U>
inline std::enable_if_t<pcomn::is_compatible_strings<T, U>::value, bool>
startswith(const T &lhs, const U &rhs)
{
   const size_t rsz = len(rhs) ;
   if (len(lhs) < rsz)
      return false ;
   const typename pcomn::string_traits<U>::char_type * const rc = pcomn::str::cstr(rhs) ;
   return std::equal(rc, rc + rsz, cstr(lhs)) ;
}

template<typename T, typename U>
inline std::enable_if_t<pcomn::is_compatible_strings<T, U>::value, bool>
endswith(const T &lhs, const U &rhs)
{
   const size_t lsz = len(lhs) ;
   const size_t rsz = len(rhs) ;
   if (lsz < rsz)
      return false ;
   const typename pcomn::string_traits<U>::char_type * const rc = pcomn::str::cstr(rhs) ;
   return std::equal(rc, rc + rsz, cstr(lhs) + (lsz - rsz)) ;
}

/*******************************************************************************
 Trimming functions (lstrip/rstrip/strip)
*******************************************************************************/
// FIXME: bad kluge, should use something like isspace()
/// @cond
namespace detail {
template<typename C> struct ws ;
template<> struct ws<char> { static const char *spaces() { return " \n\r\t\f\v" ; } } ;
template<> struct ws<wchar_t> { static const wchar_t *spaces() { return L" \n\r\t\f\v" ; } } ;
template<> struct ws<const char> : ws<char> {} ;
template<> struct ws<const wchar_t> : ws<wchar_t> {} ;
} ;
/// @endcond

template<class S>
inline std::enable_if_t<string_traits<S>::has_std_write, S &>
lstrip_inplace(S &s, const typename string_traits<S>::char_type *chrs)
{
   return s.erase(0, s.find_first_not_of(chrs)) ;
}

template<class S>
inline std::enable_if_t<string_traits<S>::has_std_write, S &>
lstrip_inplace(S &s)
{
   return lstrip_inplace(s, detail::ws<typename string_traits<S>::char_type>::spaces()) ;
}

template<class S>
inline std::enable_if_t<string_traits<S>::has_std_write, S &>
rstrip_inplace(S &s, const typename string_traits<S>::char_type *chrs)
{
   typename S::size_type last = s.find_last_not_of(chrs) ;
   if (last == S::npos)
      last = 0 ;
   else
      ++last ;
   return s.erase(last) ;
}

template<class S>
inline std::enable_if_t<string_traits<S>::has_std_write, S &>
rstrip_inplace(S &s)
{
   return rstrip_inplace(s, detail::ws<typename string_traits<S>::char_type>::spaces()) ;
}

template<class S>
inline std::enable_if_t<string_traits<S>::has_std_write, S &>
strip_inplace(S &s)
{
   return lstrip_inplace(rstrip_inplace(s)) ;
}

/*******************************************************************************
 Upper/lower conversions
*******************************************************************************/
/// @cond
namespace detail {
template<typename Converter, typename S>
inline S &&convert_inplace_stdstr(S &&s, Converter &converter,
                                size_t offs = 0, size_t size = std::string::npos)
{
   auto begin (s.begin()) ;
   std::transform(begin + offs,
                  (size == std::string::npos ? s.end() : begin + size), begin + offs,
                  converter) ;
   return std::forward<S>(s) ;
}
} // end of namespace pcomn::str::detail
/// @endcond

template<typename Converter, typename S>
inline std::enable_if_t<string_traits<S>::has_std_write, S &&>
convert_inplace(S &&s, Converter converter, size_t offs = 0, size_t size = std::string::npos)
{
   return detail::convert_inplace_stdstr(std::forward<S>(s), converter, offs, size) ;
}

template<typename Converter, typename Char>
inline std::enable_if_t<is_char<Char>::value, Char *>
convert_inplace(Char *s, Converter converter, size_t offs = 0, size_t size = (size_t)-1)
{
   std::transform(s + offs, (size == (size_t)-1 ? s + len(s) : s + size), s + offs, converter) ;
   return s ;
}

template<typename Buf, typename Converter, typename S>
inline std::enable_if_t<string_traits<S>::has_std_read, S>
convert_copy(const S &s, Converter converter, size_t offs = 0, size_t size = std::string::npos)
{
   Buf buf(s) ;
   return S(convert_inplace(buf, converter, offs, size)) ;
}

template<typename S>
inline S &&to_lower_inplace(S &&s, size_t offs = 0,
                            enable_if_string_t<std::remove_reference_t<S>, size_t> size = std::string::npos)
{
   convert_inplace(std::forward<S>(s), ctype_traits<typename string_traits<S>::char_type>::tolower, offs, size) ;
   return std::forward<S>(s) ;
}

template<typename Char, size_t n>
inline Char *to_lower_inplace(Char (&s)[n], size_t offs = 0,
                              std::enable_if_t<is_char<Char>::value, size_t> size = std::string::npos)
{
   return convert_inplace(s, ctype_traits<Char>::tolower, offs, size) ;
}

template<typename S>
inline S &&to_upper_inplace(S &&s, size_t offs = 0,
                            enable_if_string_t<std::remove_reference_t<S>, size_t> size = std::string::npos)
{
   convert_inplace(std::forward<S>(s), ctype_traits<typename string_traits<S>::char_type>::toupper, offs, size) ;
   return std::forward<S>(s) ;
}

template<typename Char, size_t n>
inline Char *to_upper_inplace(Char (&s)[n], size_t offs = 0,
                              std::enable_if_t<is_char<Char>::value, size_t> size = std::string::npos)
{
   return convert_inplace(s, ctype_traits<Char>::toupper, offs, size) ;
}

template<typename C>
inline std::basic_string<C> to_lower(const std::basic_string<C> &s)
{
   return convert_copy<std::basic_string<C> >(s, ctype_traits<C>::tolower) ;
}

template<typename C>
inline std::basic_string<C> to_lower(std::basic_string<C> &&s)
{
   return std::move(to_lower_inplace(std::move(s)))  ;
}

template<typename C>
inline std::basic_string<C> to_upper(const std::basic_string<C> &s)
{
   return convert_copy<std::basic_string<C> >(s, ctype_traits<C>::toupper) ;
}

template<typename C>
inline std::basic_string<C> to_upper(std::basic_string<C> &&s)
{
   return std::move(to_upper_inplace(std::move(s)))  ;
}

/*******************************************************************************
 Narrow/wide string literals.
*******************************************************************************/
template<typename Char>
const Char *select_cstring(const char *, const wchar_t *) ;
template<>
inline const char *select_cstring<char>(const char *s, const wchar_t *) { return s ; }
template<>
inline const wchar_t *select_cstring<wchar_t>(const char *, const wchar_t *s) { return s ; }

/// Provides C-string "literal" with required character type (narrow or wide).
#define P_CSTR(char_type, string_literal) \
   (::pcomn::str::select_cstring< char_type >((string_literal), (L##string_literal)))

} // end of namespace pcomn::str


/******************************************************************************/
/** Provides static constant empty string of type @a S.
 This allows to avoid construction of an empty string object in every expression
 it is needed.
 E.g. pcomn::emptystr<std::string>::value
*******************************************************************************/
template<class S> struct emptystr { static const S value ; } ;
template<class S> const S emptystr<S>::value ;

template<size_t n> struct emptystr<char[n]> { static const char value[n] ; } ;
template<size_t n> const char emptystr<char[n]>::value[n] = "" ;

template<size_t n> struct emptystr<wchar_t[n]> { static const wchar_t value[n] ; } ;
template<size_t n> const wchar_t emptystr<wchar_t[n]>::value[n] = L"" ;

/*******************************************************************************
 Global basic string functions (like strip, etc.)
*******************************************************************************/
/// Get the position of the first occurrence of a character in a string.
/// @param s Any string that has read inrerface like std::string (std::string,
/// std::wstring, pcomn::immutable_string, etc.).
/// @param c         The character to find
/// @param from_pos  The position in @a s to start search
template<typename S>
inline typename std::enable_if
<string_traits<S>::has_std_read,
 ssize_t>::type stringchr(const S &s,
                          typename string_traits<S>::char_type c,
                          typename string_traits<S>::size_type from_pos = 0)
{
   return s.find(c, from_pos) ;
}

template<typename S>
inline typename disable_if
<string_traits<S>::has_std_read,
 ssize_t>::type stringchr(const S &s,
                          typename string_traits<S>::char_type c,
                          typename string_traits<S>::size_type from_pos = 0)
{
   typedef typename string_traits<S>::size_type size_type ;
   typedef typename string_traits<S>::char_type char_type ;
   const size_type length = str::len(s) ;
   if (from_pos >= length) return -1 ;
   const char_type * const begin = str::cstr(s) ;
   const char_type * const cptr =
      std::char_traits<char_type>::find(begin, length, c) ;
   return cptr ? int(cptr - begin) : -1 ;
}

/*******************************************************************************
 strchr/strrchr, overloaded for both char and wchar_t
*******************************************************************************/
inline char *cstrchr(const char *s, int c) { return const_cast<char *>(strchr(s, c)) ; }
inline wchar_t *cstrchr(const wchar_t *s, int c) { return const_cast<wchar_t *>(wcschr(s, c)) ; }
inline char *cstrrchr(const char *s, int c) { return const_cast<char *>(strrchr(s, c)) ; }
inline wchar_t *cstrrchr(const wchar_t *s, int c) { return const_cast<wchar_t *>(wcsrchr(s, c)) ; }

/******************************************************************************/
/** pcomn::hasher(str) is used as a default implementation of the hash function
 in pcomn::hash_fn functor (and thus in pcomn::hashtable, too).
*******************************************************************************/
template<typename S>
inline typename ::pcomn::string_traits<S>::hash_type hasher(const S &str)
{
   return ::pcomn::str::hash(str) ;
}

/*******************************************************************************
 Unsafe format functions for std::string
*******************************************************************************/
std::string strprintf(const char *format, ...) PCOMN_ATTR_PRINTF(1, 2) ;
std::string &strappendf(std::string &s, const char *format, ...) PCOMN_ATTR_PRINTF(2, 3) ;

namespace detail {
template<size_t initsize = 7>
__noinline std::string strvprintf_(const char *format, va_list args)
{
   va_list tmp ;
   va_copy(tmp, args) ;
   std::string result ('A', initsize) ;
   const size_t actual_size = vsnprintf(&*result.begin(), initsize + 1, format, tmp) ;
   result.resize(actual_size) ;
   va_end(tmp) ;
   if (actual_size > initsize)
      vsnprintf(&*result.begin(), actual_size + 1, format, args) ;

   return std::move(result) ;
}

template<nullptr_t>
__noinline std::string &strvappendf_(std::string &s, const char *format, va_list args)
{
   va_list tmp ;
   va_copy(tmp, args) ;

   char dummy[1] ;
   if (const size_t append_size = vsnprintf(dummy, 1, format, tmp))
   {
      const size_t old_size = s.size() ;
      s.resize(old_size + append_size) ;
      va_end(tmp) ;
      vsnprintf(&*s.begin() + old_size, append_size + 1, format, args) ;
   }
   else
      va_end(tmp) ;

   return s ;
}
}

inline std::string strvprintf(const char *format, va_list args)
{
   return detail::strvprintf_(format, args) ;
}

inline std::string strprintf(const char *format, ...)
{
   va_list parm ;
   va_start(parm, format) ;
   std::string result (strvprintf(format, parm)) ;
   va_end(parm) ;

   return std::move(result) ;
}

inline std::string &strvappendf(std::string &s, const char *format, va_list args)
{
   return detail::strvappendf_<nullptr>(s, format, args) ;
}

inline std::string &strappendf(std::string &s, const char *format, ...)
{
   va_list parm ;
   va_start(parm, format) ;
   strvappendf(s, format, parm) ;
   va_end(parm) ;
   return s ;
}

/*******************************************************************************
 Unsafe format functions for char[]
*******************************************************************************/
char *bufprintf(char *buf, size_t n, const char *format, ...) PCOMN_ATTR_PRINTF(3, 4) ;

inline char *bufprintf(char *buf, size_t n, const char *format, ...)
{
   va_list parm ;
   va_start(parm, format) ;
   vsnprintf(buf, n, format, parm) ;
   va_end(parm) ;

   return buf ;
}

template<size_t n>
char *bufprintf(char (&buf)[n], const char *format, ...) PCOMN_ATTR_PRINTF(2, 3) ;

template<size_t n>
inline char *bufprintf(char (&buf)[n], const char *format, ...)
{
   va_list parm ;
   va_start(parm, format) ;
   vsnprintf(buf, n, format, parm) ;
   va_end(parm) ;

   return buf ;
}

/*******************************************************************************
 Convert strings with escaped characters into normal strings and back, escape and
 unescape characters.
*******************************************************************************/
template<typename C>
inline typename std::char_traits<C>::char_type
esc2chr(C c)
{
   switch (c)
   {
      case 'a' : return '\a' ;
      case 'b' : return '\b' ;
      case 'f' : return '\f' ;
      case 'n' : return '\n' ;
      case 'r' : return '\r' ;
      case 't' : return '\t' ;
      case 'v' : return '\v' ;
      case '0' : return '\0' ;
   }
   return c ;
}

template<typename C>
inline typename std::char_traits<C>::char_type
chr2esc(C c)
{
   switch (c)
   {
      case '\a' : return 'a' ;
      case '\b' : return 'b' ;
      case '\f' : return 'f' ;
      case '\n' : return 'n' ;
      case '\r' : return 'r' ;
      case '\t' : return 't' ;
      case '\v' : return 'v' ;
      case 0    : return '0' ;
   }
   return c ;
}

template<typename C, typename InputIterator>
inline C unescape_char(C ch, InputIterator &iter, InputIterator end)
{
   return ch == '\\' && ++iter != end
      ? esc2chr(*iter)
      : ch ;
}

template<typename C, typename InputRange>
inline C unescape_char(C ch, InputRange &irange)
{
   return ch == '\\' && ++irange
      ? esc2chr(*irange)
      : ch ;
}

template<typename C, typename OutputIterator>
inline OutputIterator escape_char(C ch, C delimiter, OutputIterator result)
{
   const C escaped = chr2esc(ch) ;
   if (escaped != ch || ch == delimiter || ch == '\\')
   {
      *result = '\\' ;
      ++result ;
   }
   *result = escaped ;
   return ++result ;
}

template<typename InputIterator, typename OutputIterator, typename C>
inline OutputIterator escape_range(InputIterator begin, InputIterator end,
                                   OutputIterator out, C delimiter)
{
   for (; begin != end ; ++begin)
      out = escape_char(*begin, delimiter, out) ;
   return out ;
}

/*******************************************************************************
 narrow_output
*******************************************************************************/
/// Output a sequence of wide characters into a narrow-character ostream.
template<class StreamTraits, typename InputIterator>
std::basic_ostream<char, StreamTraits> &
narrow_output(std::basic_ostream<char, StreamTraits> &os,
              InputIterator begin, InputIterator end)
{
   char buf[256] ;
   char * const endbuf = buf + sizeof(buf) - 1 ;
   while(begin != end)
   {
      char *pc ;
      for (pc = buf ; pc != endbuf && begin != end ; ++pc, ++begin)
         *pc = std::wcout.narrow(*begin, '?') ;
      *pc = 0 ;
      os << buf ;
   }
   return os ;
}

} // end of namespace pcomn

namespace std {

inline ::pcomn::string_traits<string>::hash_type hasher(const string &str)
{
   return ::pcomn::str::hash(str) ;
}

template<class StreamTraits, class Traits, class Allocator>
inline std::basic_ostream<char, StreamTraits> &
operator<<(std::basic_ostream<char, StreamTraits> &os,
           const std::basic_string<wchar_t, Traits, Allocator> &str)
{
   const wchar_t *begin = str.c_str() ;
   return pcomn::narrow_output(os, begin, begin + str.size()) ;
}

} // end of namespace std

namespace pcomn { using std::hasher ; } // end of namespace pcomn

#define PCOMN_ENABLE_CTR_IF_STRCHAR(S, C) pcomn::enable_if_strchar_t<S, C, pcomn::Instantiate> = {}

#endif /* __PCOMN_STRING_H */
