/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_OMANIP_H
#define __PCOMN_OMANIP_H
/*******************************************************************************
 FILE         :   pcomn_omanip.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Output manipulators

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 May 2000
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_iterator.h>

#include <iostream>
#include <iomanip>
#include <functional>
#include <type_traits>

#include <stdio.h>

namespace pcomn {

enum class NoOut { nout } ;
const NoOut nout = NoOut::nout ;

inline std::ostream &operator<<(std::ostream &os, NoOut) { return os ; }

template<typename> struct omanip ;

template<typename F, typename... Args>
auto make_omanip(F &&fn, Args &&...args)
   -> omanip<decltype(std::bind(std::forward<F>(fn), std::placeholders::_1, std::forward<Args>(args)...))> ;

/*******************************************************************************

*******************************************************************************/
#define PCOMN_DERIVE_OMANIP(basecall) decltype(basecall) { return basecall ; }
#define PCOMN_MAKE_OMANIP(...) decltype(pcomn::make_omanip(__VA_ARGS__)) { return pcomn::make_omanip(__VA_ARGS__) ; }

/******************************************************************************/
/* Universal ostream manipulator
*******************************************************************************/
template<typename Bind>
struct omanip final {
      static_assert(std::is_same<typename std::remove_reference<typename std::result_of<Bind(std::ostream &)>::type>::type, std::ostream>::value,
                    "Invalid pcomn::omanip template argument: must be callable with std::ostream & argument and return std::ostream &") ;

      omanip(omanip &&other) : _fn(std::move(other._fn)) {}

      omanip(const omanip &) = delete ;
      omanip &operator=(const omanip &) = delete ;
      omanip &operator=(omanip &&) = delete ;

      std::ostream &operator()(std::ostream &os) const { return _fn(os) ; }

      template<typename F, typename... Args>
      friend auto make_omanip(F &&fn, Args &&...args)
         -> omanip<decltype(std::bind(std::forward<F>(fn), std::placeholders::_1, std::forward<Args>(args)...))> ;

   private:
      mutable Bind _fn ;

      omanip(Bind &&f) : _fn(std::move(f)) {}
} ;

template<typename F, typename... Args>
inline auto make_omanip(F &&fn, Args &&...args)
   -> omanip<decltype(std::bind(std::forward<F>(fn), std::placeholders::_1, std::forward<Args>(args)...))>
{
   return std::bind(std::forward<F>(fn), std::placeholders::_1, std::forward<Args>(args)...) ;
}

template<typename F>
inline std::ostream &operator<<(std::ostream &os, const omanip<F> &manip)
{
   return manip(os) ;
}

/*******************************************************************************
 Various ostream manipulators
*******************************************************************************/
namespace detail {
template<typename T>
using decayed = typename std::decay<T>::type ;

template<typename InputIterator, typename Before, typename After>
std::ostream &print_sequence(std::ostream &os, InputIterator begin, InputIterator end,
                             const Before &before, const After &after)
{
   for (; begin != end ; ++begin)
      os << before << *begin << after ;
   return os ;
}
}

template<typename InputIterator, typename Before, typename After>
inline auto osequence(InputIterator begin, InputIterator end, const Before &before, const After &after)
   ->PCOMN_MAKE_OMANIP(detail::print_sequence<InputIterator, detail::decayed<Before>, detail::decayed<After> >,
                       begin, end, detail::decayed<Before>(before), detail::decayed<After>(after)) ;


template<typename InputIterator, typename After>
inline auto osequence(InputIterator begin, InputIterator end, const After &after)
   ->PCOMN_DERIVE_OMANIP(osequence(begin, end, nout, after)) ;

template<typename InputIterator>
inline auto osequence(InputIterator begin, InputIterator end)
   ->PCOMN_DERIVE_OMANIP(osequence(begin, end, nout, '\n')) ;

template<class Container, typename Before, typename After>
inline auto ocontainer(const Container &container, const Before &before, const After &after)
   ->PCOMN_DERIVE_OMANIP(osequence(std::begin(container), std::end(container), before, after)) ;

template<class Container, typename After>
inline auto ocontainer(const Container &container, const After &after)
   ->PCOMN_DERIVE_OMANIP(osequence(std::begin(container), std::end(container), after)) ;

template<class Container>
inline auto ocontainer(const Container &container)
   ->PCOMN_DERIVE_OMANIP(osequence(std::begin(container), std::end(container))) ;

namespace detail {
template<typename InputIterator, typename Delim>
inline std::ostream &print_sequence_delimited(std::ostream &os, InputIterator begin, InputIterator end,
                                              const Delim &delim)
{
    for (bool first = true ; begin != end ; ++begin)
    {
        if (!first)
            os << delim ;
        else
           first = false ;
        os << *begin ;
    }
    return os ;
}

template<typename InputIterator>
std::ostream &print_sequence_delimited(std::ostream &os, InputIterator begin, InputIterator end)
{
   return print_sequence_delimited(os, begin, end, ", ") ;
}

}

template<typename InputIterator, typename Delim>
inline auto oseqdelim(InputIterator begin, InputIterator end, const Delim &delim)
   ->PCOMN_MAKE_OMANIP(detail::print_sequence_delimited<InputIterator, detail::decayed<Delim> >,
                       begin, end, detail::decayed<Delim>(delim)) ;

template<typename InputIterator>
inline auto oseqdelim(InputIterator begin, InputIterator end)
   ->PCOMN_MAKE_OMANIP(detail::print_sequence_delimited<InputIterator>, begin, end) ;

template<class Container, typename Delim>
inline auto ocontdelim(const Container &container, const Delim &delim)
   ->PCOMN_DERIVE_OMANIP(oseqdelim(std::begin(container), std::end(container), delim)) ;

template<class Container>
inline auto ocontdelim(const Container &container)
   ->PCOMN_DERIVE_OMANIP(oseqdelim(std::begin(container), std::end(container)))

inline char *hrsize(unsigned long long sz, char *buf)
{
   if (sz < KiB)
      sprintf(buf, "%lluB", sz) ;
   else if (sz < MiB)
      sprintf(buf, "%.1fK", sz/(KiB * 1.0)) ;
   else if (sz < GiB)
      sprintf(buf, "%.1fM", sz/(MiB * 1.0)) ;
   else
      sprintf(buf, "%.1fG", sz/(GiB * 1.0)) ;
   return buf ;
}

namespace detail {
template<typename T>
std::ostream &print_hrsize(std::ostream &os, const T &sz)
{
   char buf[64] ;
   return os << hrsize(sz, buf) ;
}

template<typename T>
std::ostream &print_quoted_string(std::ostream &os, const T &str)
{
   return os << '\'' << str << '\'' ;
}
}

template<typename T>
inline auto ohrsize(const T &sz)
   ->PCOMN_MAKE_OMANIP(detail::print_hrsize<unsigned long long>, (unsigned long long)(sz)) ;

template<typename T>
inline auto ostrq(const T &str)
   ->PCOMN_MAKE_OMANIP(detail::print_quoted_string<T>, std::cref(str)) ;

} // end of namespace pcomn

#endif /* __PCOMN_OMANIP_H */
