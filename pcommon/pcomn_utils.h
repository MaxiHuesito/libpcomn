/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_UTILS_H
#define __PCOMN_UTILS_H
/*******************************************************************************
 FILE         :   pcomn_utils.h
 COPYRIGHT    :   Yakov Markovitch, 1994-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Various supplemental functions & templates

 CREATION DATE:   14 Dec 1994
*******************************************************************************/
/** @file
    Various supplemental functions & templates
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_assert.h>
#include <pcomn_meta.h>
#include <pcomn_integer.h>

#include <iostream>
#include <utility>
#include <functional>
#include <memory>
#include <algorithm>

#include <stddef.h>

namespace pcomn {

template<typename> struct basic_strslice ;

/******************************************************************************/
/** This template class is intended to save an old variable value before it is
    changed and automatically restore upon exiting from the block.
*******************************************************************************/
template<typename T>
class vsaver {
      PCOMN_NONCOPYABLE(vsaver) ;
   public:
      explicit vsaver(T &variable) :
         _saved(variable),
         _var(&variable)
      {}

      vsaver(T &variable, const T &new_value) :
         _saved(variable),
         _var(&variable)
      {
         variable = new_value ;
      }

      ~vsaver()
      {
         if (_var) *_var = _saved ;
      }

      const T &release()
      {
         _var = NULL ;
         return _saved ;
      }

      const T &restore()
      {
         if (_var)
         {
            *_var = _saved ;
            _var = NULL ;
         }
         return _saved ;
      }

      const T &saved() const { return _saved ; }

   private:
      const T  _saved ;
      T *      _var ;
} ;

/******************************************************************************/
/** This template class is intended to temporarily set a bit mask and then automatically
 restore the saved settings in the destructor.
*******************************************************************************/
template<typename T>
class bitsaver {
      PCOMN_NONCOPYABLE(bitsaver) ;
   public:
      bitsaver(T &flags, T mask) :
        _flags(flags),
        _mask(mask),
        _status(flags & mask)
      {}

      ~bitsaver()
      {
         (_flags &= ~_mask) |= _status ;
      }

   private:
      T &      _flags ;
      const T  _mask ;
      const T  _status ;
} ;

/*******************************************************************************
                 template<class T, typename R>
                 class finalizer
*******************************************************************************/
template<class T>
class finalizer {
      PCOMN_NONCOPYABLE(finalizer) ;
      PCOMN_NONASSIGNABLE(finalizer) ;
   public:
      typedef std::function<void (T*)> finalizer_type ;

      finalizer(T &var, const finalizer_type &fn) : _var(var), _finalize(fn) {}
      finalizer(T &var, finalizer_type &&fn) : _var(var), _finalize(fn) {}

      ~finalizer() { finalize() ; }

      void release() { _finalize = finalizer_type() ; }

      void finalize()
      {
         if (_finalize)
         {
            _finalize(&_var) ;
            release() ;
         }
      }

   private:
      T & _var ;
      finalizer_type _finalize ;
} ;

/*******************************************************************************
 Utility functions
*******************************************************************************/
/// Delete the scalar object a pointer points to and assign nullptr to the pointer
///
/// This function nullifies the passed pointer @em first and @em then deletes an object
/// the pointer pointed to before; this allows to work with cyclic pointer structures.
///
template<typename T>
inline T *&clear_delete(T *&ptr)
{
   T *temp = ptr ;
   ptr = 0 ;
   delete temp ;
   return ptr ;
}

/// Delete an array of objects a pointer points to and assign nullptr to the pointer
template<typename T>
inline T *&clear_deletev(T *&vec)
{
   T *temp = vec ;
   vec = 0 ;
   delete [] temp ;
   return vec ;
}

template<typename O, typename V>
inline void outparam_set(O *outparam_ptr, V &&value)
{
   if (outparam_ptr)
      *outparam_ptr = std::forward<V>(value) ;
}

template<typename T, typename P>
constexpr inline auto nullable_get(P &&ptr, T &&default_value) -> decltype(*std::forward<P>(ptr))
{
   return std::forward<P>(ptr) ? *std::forward<P>(ptr) : std::forward<T>(default_value) ;
}

/// Compare two values for equality through pointers to that values or pointer-like
/// objects, taking into account NULL pointers.
///
/// @a x and @a y must be pointers or pointer-like objects (i.e. implementing
/// operator*). @a x and @a y are considered equal if both are NULL, or both are nonnull
/// _and_ referenced values are equal.
///
template<typename P1, typename P2>
constexpr inline auto nullable_eq(const P1 &x, const P2 &y) -> decltype(*x == *y)
{
   return !x == !y && (!x || *x == *y) ;
}

/***************************************************************************//**
 Tagged pointer
*******************************************************************************/
/**{@*/
/// Check if T* is assignable to U*, providing that valtypes of T and U are the same
/// (i.e. assigning to base class or void pointer is _not_ allowed)
template<typename T, typename U>
using is_ptr_exact_assignable = ct_and<std::is_assignable<T *&, U *>, std::is_same<valtype_t<T>, valtype_t<U> > > ;

namespace detail {

template<bool dir, typename T, typename U> struct is_pcopy ;
template<typename T, typename U> struct is_pcopy<true, T, U> : is_ptr_exact_assignable<T, U> {} ;
template<typename T, typename U> struct is_pcopy<false, T, U> : is_ptr_exact_assignable<U, T> {} ;

template<bool dir, typename U, typename... Types>
struct count_exact_copyable ;
template<bool dir, typename U>
struct count_exact_copyable<dir, U> : int_constant<0> {} ;
template<bool dir, typename U, typename Head, typename... Tail>
struct count_exact_copyable<dir, U, Head, Tail...> :
         int_constant<((int)!!is_pcopy<dir, Head, U>::value +
                                      count_exact_copyable<dir, U, Tail...>::value)> {} ;

template<int ndx, bool, bool dir, typename U, typename Head, typename... Tail> struct fea ;

template<int ndx, bool dir, typename U, typename Head, typename... Tail>
struct fea<ndx, false, dir, U, Head, Tail...> :
         fea<ndx + 1, is_pcopy<dir, Head, U>::value, dir, U, Tail...> {} ;

template<int ndx, bool dir, typename Head, typename U>
struct fea<ndx, false, dir, U, Head> :
         int_constant<is_pcopy<dir, Head, U>::value ? ndx : -1> {} ;
template<int ndx, bool dir, typename U, typename Head, typename... Tail>
struct fea<ndx, true, dir, U, Head, Tail...> : int_constant<ndx - 1> {} ;

template<bool dir, typename U, typename... Types>
struct find_exact_copyable : fea<0, false, dir, U, Types...> {} ;
template<bool dir, typename U>
struct find_exact_copyable<dir, U> : int_constant<-1> {} ;
} ;

/***************************************************************************//**
 Tagged union of two pointers with sizof(tagged_ptr_union)==sizeof(void *).

 The alignment of the memory pointed to by any of union members MUST be at least 2
*******************************************************************************/
template<typename T1, typename T2, typename... T>
struct tagged_ptr_union {
   private:
      typedef std::tuple<T1 *, T2 *, T *...> tag_tuple ;

      template<bool dir, typename U>
      using is_exact_copyable = bool_constant<detail::count_exact_copyable<dir, U, T1, T2, T...>::value> ;
      template<typename U>
      using putndx = detail::find_exact_copyable<true, U, T1, T2, T...> ;
      template<typename U>
      using getndx = detail::find_exact_copyable<false, U, T1, T2, T...> ;

      // The _minimum_ alignment of all pointed to types.
      // Determines maximum number of distinct items in the union, e.g. tagged_ptr_union
      // cannot discriminate between more than element_alignment items
      static constexpr size_t element_alignment = ct_min<size_t, alignof(T1), alignof(T2), alignof(T)...>::value ;
      static constexpr intptr_t ptr_mask = -(1 << bitop::ct_log2ceil<element_alignment>::value) ;

      // The second static_assert would be enough, but we use this extra check to get
      // more detailed error messages
      static_assert(element_alignment > 1,
                    "Types pointed to by pcomn::tagged_ptr_union must have alignment at least 2") ;
      static_assert(std::tuple_size<tag_tuple>::value <= element_alignment,
                    "Too many items in tagged_ptr_union: the number of distinct items cannot be greater "
                    "than the minimum element alignment") ;

   public:
      template<size_t I>
      using element_type = typename std::tuple_element<I, tag_tuple>::type ;

      typedef element_type<0> first_type ;
      typedef element_type<1> second_type ;

      /// Default constructor creates NULL pointer
      constexpr tagged_ptr_union() : _data{0} {}

      constexpr tagged_ptr_union(nullptr_t) : _data{0} {}

      template<typename U>
      constexpr tagged_ptr_union(U *v, std::enable_if_t<is_exact_copyable<true, U>::value, Instantiate> = {})
         : _data{(uintptr_t)v | getndx<U>::value}
      {}

      /// Void pointer conversion: if @em any of union members is set, returns it value
      /// as a void pointer
      constexpr operator const void *() const { return (const void *)(as_intptr() & ptr_mask) ; }

      template<size_t ndx>
      constexpr element_type<ndx> get() const
      {
         return this->get_(int_constant<ndx>()) ;
      }

      template<typename U>
      constexpr element_type<getndx<U>::value> get() const
      {
         return this->get_(int_constant<getndx<U>::value>()) ;
      }

      template<size_t ndx>
      tagged_ptr_union &set(element_type<ndx> v)
      {
         this->set_(v, int_constant<ndx>()) ;
         return *this ;
      }

      tagged_ptr_union &operator=(const tagged_ptr_union &) = default ;

      tagged_ptr_union &operator=(nullptr_t)
      {
         _data.first = nullptr ;
         return *this ;
      }

      template<typename U>
      std::enable_if_t<is_exact_copyable<true, U>::value, tagged_ptr_union &>
      operator=(U *v)
      {
         this->set_(v, putndx<U>()) ;
         return *this ;
      }

      /// Get the index of the type of the pointer currently held by the union
      constexpr unsigned type_ndx() const { return (unsigned)tag() ; }

      static constexpr unsigned type_maxndx() { return std::tuple_size<tag_tuple>::value - 1 ; }

   private:
      union {
            uintptr_t   other ;
            first_type  first ;
      } _data ;

      constexpr intptr_t as_intptr() const { return _data.other ; }
      constexpr intptr_t tag() const { return as_intptr() &~ ptr_mask ; }

      constexpr uintptr_t mask(intptr_t ndx) const
      {
         return (as_intptr() & ptr_mask) &~ -(tag() ^ ndx) ;
      }

      template<int ndx>
      constexpr element_type<ndx> get_(int_constant<ndx>) const
      {
         return reinterpret_cast<element_type<ndx> >(mask(ndx)) ;
      }
      template<int ndx, typename U>
      void set_(U *v, int_constant<ndx>)
      {
         NOXCHECK(!((intptr_t)v &~ ptr_mask)) ;

         _data.other = (uintptr_t)v | ndx ;
      }
} ;

template<typename T1, typename T2, typename... T>
using tagged_cptr_union = tagged_ptr_union<const T1, const T2, const T ...> ;

/**}@*/

/***************************************************************************//**
 Strong typedef is a type wrapper that guarentees that two types are distinguised
 even when they share the same underlying implementation.

 Unlike typedef, strong typedef _does_ create a new type.
*******************************************************************************/
/**{@*/
template<typename Data, typename Tag, bool literal=std::is_literal_type<Data>::value> struct tdef ;

template<typename Data, typename Tag>
using strong_typedef = tdef<Data, Tag> ;

template<typename Data, typename Tag>
struct tdef<Data, Tag, true> {

      constexpr tdef() = default ;
      explicit constexpr tdef(const Data &v) : _data(v) {}
      constexpr tdef(const tdef &) = default ;
      constexpr tdef(tdef &&) = default ;

      tdef &operator=(const tdef &) = default ;
      tdef &operator=(tdef &&) = default ;
      tdef &operator=(const Data &rhs) { _data = rhs ; return *this ;}
      tdef &operator=(Data &&rhs) { _data = std::move(rhs) ; return *this ;}

      constexpr operator Data() const { return _data ; }
      operator Data &() & { return _data ; }
      operator Data &&() && { return std::move(_data) ; }

      friend constexpr bool operator==(const tdef &x, const tdef &y) { return x._data == y._data ; }
      friend constexpr bool operator<(const tdef &x, const tdef &y) { return x._data <  y._data ; }

      PCOMN_DEFINE_RELOP_FUNCTIONS(friend constexpr, tdef) ;
      friend std::ostream &operator<<(std::ostream &os, const tdef &v) { return os << v._data ; }

   private:
      Data _data {} ;
} ;

template<typename Data, typename Tag>
struct tdef<Data, Tag, false> {
      tdef() = default ;
      explicit tdef(const Data &v) : _data(v) {}
      tdef(const tdef &) = default ;
      tdef(tdef &&) = default ;

      tdef &operator=(const tdef &) = default ;
      tdef &operator=(tdef &&) = default ;
      tdef &operator=(const Data &rhs) { _data = rhs ; return *this ;}
      tdef &operator=(Data &&rhs) { _data = std::move(rhs) ; return *this ;}

      operator const Data &() const { return _data ; }
      operator Data &() & { return _data ; }
      operator Data &&() && { return std::move(_data) ; }

      friend bool operator==(const tdef &x, const tdef &y) { return x._data == y._data ; }
      friend bool operator<(const tdef &x, const tdef &y)  { return x._data <  y._data ; }

      PCOMN_DEFINE_RELOP_FUNCTIONS(friend, tdef) ;
      friend std::ostream &operator<<(std::ostream &os, const tdef &v) { return os << v._data ; }

   private:
      Data _data {} ;
} ;
/**}@*/

/******************************************************************************/
/**
*******************************************************************************/
template<size_t n, size_t alignment = 1>
struct auto_buffer final {

      explicit auto_buffer(size_t sz) :
         _data(sz <= sizeof(_buf)
               ? reinterpret_cast<char *>(&_buf)
               : (*reinterpret_cast<char **>(&_buf) = new char[sz]))
      {}

      ~auto_buffer()
      {
         if ((void *)_data != &_buf)
            delete [] _data ;
      }

      char *get() const { return _data ; }

   private:
      std::aligned_storage_t<n, ct_max<size_t, alignment, alignof(char *)>::value> _buf ;
      char * const _data ;

      PCOMN_NONCOPYABLE(auto_buffer) ;
      PCOMN_NONASSIGNABLE(auto_buffer) ;
} ;

/***************************************************************************//**
 Output stream with "embedded" stream buffer, the memory buffer is a C array
 the member of of the class.

 The template argument defines (fixed) size of the output buffer.
 This class never makes dynamic allocations.
*******************************************************************************/
template<size_t sz>
class bufstr_ostream : private std::basic_streambuf<char>, public std::ostream {
   public:
      /// Create the stream with embedded buffer of @a sz bytes.
      bufstr_ostream() noexcept ;

      /// Get the pouinter to internal buffer memory
      const char *str() const { return _buffer ; }
      char *str() { return _buffer ; }

      bufstr_ostream &reset()
      {
         clear() ;
         setp(_buffer + 0, _buffer + sizeof _buffer - 1) ;
         return *this ;
      }

   private:
      char _buffer[sz] ;
} ;

/******************************************************************************/
/** Output stream with external fixed-size stream buffer .
*******************************************************************************/
template<>
class bufstr_ostream<0> : private std::basic_streambuf<char>, public std::ostream {
      PCOMN_NONCOPYABLE(bufstr_ostream) ;
      PCOMN_NONASSIGNABLE(bufstr_ostream) ;
   public:
      bufstr_ostream(char *buf, size_t bufsize) noexcept :
         std::ostream(static_cast<std::basic_streambuf<char> *>(this)),
         _buffer(buf), _bufsz(bufsize)
      {
         setp(_buffer, _buffer + _bufsz) ;
      }

      bufstr_ostream() noexcept :
         std::ostream(static_cast<std::basic_streambuf<char> *>(this))
      {}

      /// Get the pouinter to internal buffer memory
      const char *str() const { return _buffer ; }
      char *str() { return _buffer ; }

      bufstr_ostream &reset()
      {
         clear() ;
         setp(_buffer, _buffer + _bufsz) ;
         return *this ;
      }

   private:
      char *   _buffer = nullptr ;
      size_t   _bufsz  = 0 ;
} ;

/*******************************************************************************
 bufstr_ostream<size_t>
*******************************************************************************/
template<size_t sz>
bufstr_ostream<sz>::bufstr_ostream() noexcept :
   std::ostream(static_cast<std::basic_streambuf<char> *>(this))
{
   *_buffer = _buffer[sizeof _buffer - 1] = 0 ;
   reset() ;
}

/******************************************************************************/
/** Input stream from a memory buffer
*******************************************************************************/
class imemstream : private std::basic_streambuf<char>, public std::istream {
      typedef std::istream ancestor ;
   public:
      using ancestor::char_type ;
      using ancestor::int_type ;
      using ancestor::pos_type ;
      using ancestor::off_type ;
      using ancestor::traits_type ;

      imemstream() noexcept :
         ancestor(static_cast<std::basic_streambuf<char> *>(this)),
         _size(0), _data("")
      {
         setstate(eofbit) ;
      }

      imemstream(const void *data, size_t size) noexcept :
         ancestor(static_cast<std::basic_streambuf<char> *>(this)),
         _size(size), _data(static_cast<const char *>(data))
      {
         reset() ;
      }

      template<size_t n>
      imemstream(const char (&data)[n]) noexcept :
         imemstream(std::begin(data), n - (!data[n-1]))
      {}

      /// Get the pointer to internal buffer memory
      const char *data() const { return _data ; }

      imemstream &reset()
      {
         clear(_size ? goodbit : eofbit) ;
         char * const start = const_cast<char *>(_data) ;
         setg(start, start, start + _size) ;
         return *this ;
      }

   private:
      const size_t         _size ;
      const char *  const  _data ;
} ;

/******************************************************************************/
/** Implements stream output into memory buffer (std::stream)

 Allows @em moving out the resulting std::string, thus avoiding extra result buffer
 copying. This is in contrast to std::ostringstream, whih allows only copying out
 the resulting string buffer.
*******************************************************************************/
class omemstream : private std::basic_streambuf<char>, public std::ostream {
      typedef std::ostream                ancestor ;
      typedef std::basic_streambuf<char>  streambuf_type ;
   public:
      using ancestor::char_type ;
      using ancestor::int_type ;
      using ancestor::pos_type ;
      using ancestor::off_type ;
      using ancestor::traits_type ;

      /// Default constructor starts with an empty string buffer.
      omemstream() : ancestor(static_cast<streambuf_type *>(this)) {}

      /// Start with an existing string buffer
      /// @param initstr A string slice to copy as a starting buffer.
      ///
      explicit omemstream(const basic_strslice<char> &initstr) ;

      /// Get constant reference to internal memory buffer
      basic_strslice<char> str() const ;

      /// Move out the resulting string
      ///
      /// After this call the internal buffer becomes empty
      ///
      std::string checkout()
      {
         const char * const tail = pbase() + _data.size() ;
         _data.append(tail, pptr() - tail) ;
         reset_buf() ;
         return std::move(_data) ;
      }

   private:
      std::string _data ;

      enum : size_t {
         init_capacity =
         #ifdef PCOMN_COMPILER_MS
         16
         #else
         64
         #endif
      } ;

   protected:
      void init_buf()
      {
         char * const pstart = const_cast<char *>(_data.data()) ;
         setp(pstart, pstart + _data.capacity()) ;
         pbump((unsigned)_data.size()) ;
         setg(pstart, pstart, pstart) ;
      }
      void reset_buf()
      {
         setp(nullptr, nullptr) ;
         setg(nullptr, nullptr, nullptr) ;
      }

      int_type overflow(int_type c = traits_type::eof()) override
      {
         if (traits_type::eq_int_type(c, traits_type::eof()))
            return traits_type::not_eof(c) ;

         const size_t capacity = _data.capacity() ;
         const size_t max_size = _data.max_size() ;

         if (pptr() >= epptr())
         {
            if (capacity >= max_size - 1)
               return traits_type::eof() ;

            std::string new_buffer ;
            // Grow buffer
            new_buffer.reserve(midval<size_t>(init_capacity, max_size, 2 * (capacity + 1)) - 1) ;
            if (pbase())
               new_buffer.assign(pbase(), epptr() - pbase()) ;
            _data.swap(new_buffer) ;
            init_buf() ;
         }
         *pptr() = traits_type::to_char_type(c) ;
         pbump(1) ;
         return c ;
      }
} ;

/******************************************************************************/
/** Returns the result of streaming arg into pcomn::omemstream

 The result is the same as for boost::lexical_cast<std::string>(arg),
 but way more efficient due to pcomn::omemstream instead of std::stringstream
*******************************************************************************/
template<typename T>
__noinline
disable_if_t<std::is_convertible<T, std::string>::value, std::string>
string_cast(T &&arg)
{
   pcomn::omemstream os ;
   os << std::forward<T>(arg) ;
   return os.checkout() ;
}

template<typename T>
inline
std::enable_if_t<std::is_convertible<T, std::string>::value, std::string>
string_cast(T &&arg)
{
   return {std::forward<T>(arg)} ;
}

inline std::ostream &print_values(std::ostream &os) { return os ; }

template<typename T, typename... Tn>
inline std::ostream &print_values(std::ostream &os, T &&a1, Tn &&...an)
{
   return print_values(os << std::forward<T>(a1), std::forward<Tn>(an)...) ;
}

template<typename T1, typename T2, typename... Tn>
__noinline
std::string string_cast(T1 &&a1, T2 &&a2, Tn &&...an)
{
   pcomn::omemstream os ;
   print_values(os, std::forward<T1>(a1), std::forward<T2>(a2), std::forward<Tn>(an)...) ;
   return os.checkout() ;
}

} // end of pcomn namespace

#endif /* __PCOMN_UTILS_H */
