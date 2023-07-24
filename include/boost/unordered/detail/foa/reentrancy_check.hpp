/* Copyright 2023 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See https://www.boost.org/libs/unordered for library home page.
 */

#ifndef BOOST_UNORDERED_DETAIL_FOA_REENTRANCY_CHECK_HPP
#define BOOST_UNORDERED_DETAIL_FOA_REENTRANCY_CHECK_HPP

#include <boost/assert.hpp>
#include <utility>

#if !defined(BOOST_UNORDERED_DISABLE_REENTRANCY_CHECK)
#if defined(BOOST_UNORDERED_ENABLE_REENTRANCY_CHECK_HANDLER)||        \
    (defined(BOOST_UNORDERED_ENABLE_REENTRANCY_CHECK_DEBUG_HANDLER)&& \
     !defined(NDEBUG))||                                              \
    !defined(BOOST_ASSERT_IS_VOID)
#define BOOST_UNORDERED_REENTRANCY_CHECK
#endif
#endif

#if defined(BOOST_UNORDERED_REENTRANCY_CHECK)
#if defined(BOOST_UNORDERED_ENABLE_REENTRANCY_CHECK_HANDLER)||     \
    defined(BOOST_UNORDERED_ENABLE_REENTRANCY_CHECK_DEBUG_HANDLER)

#include <boost/config.hpp> /* BOOST_LIKELY */

namespace boost
{
  void boost_unordered_reentrancy_check_failed();
}

#define BOOST_UNORDERED_REENTRANCY_CHECK_ASSERT_MSG(expr,msg) \
(BOOST_LIKELY(!!(expr))?((void)0):                            \
  ::boost::boost_unordered_reentrancy_check_failed())

#else

#define BOOST_UNORDERED_REENTRANCY_CHECK_ASSERT_MSG(expr,msg) \
BOOST_ASSERT_MSG(expr,msg)

#endif
#endif

namespace boost{
namespace unordered{
namespace detail{
namespace foa{

#if defined(BOOST_UNORDERED_REENTRANCY_CHECK)

class entry_trace
{
public:
  entry_trace(const void* px_):px{px_}
  {
    BOOST_ASSERT(px!=nullptr);
    BOOST_UNORDERED_REENTRANCY_CHECK_ASSERT_MSG(
      !find(px),"reentrancy not allowed");
    header()=this;
  }

  entry_trace(const entry_trace&)=delete;
  entry_trace& operator=(const entry_trace&)=delete;
  ~entry_trace(){clear();}

  void clear()
  {
    if(px){
      header()=next;
      px=nullptr;
    }
  }
  
private:
  static entry_trace*& header()
  {
    thread_local entry_trace *pe=nullptr;
    return pe;
  }

  static bool find(const void* px)
  {
    for(auto pe=header();pe;pe=pe->next){
      if(pe->px==px)return true;
    }
    return false;
  }

  const void  *px;
  entry_trace *next=header();
};

template<typename LockGuard>
struct reentrancy_checked
{
  template<typename... Args>
  reentrancy_checked(const void* px,Args&&... args):
    tr{px},lck{std::forward<Args>(args)...}{}

  void unlock()
  {
    lck.unlock();
    tr.clear();
  }

  entry_trace tr;
  LockGuard   lck;
};

#else

template<typename LockGuard>
struct reentrancy_checked
{
  template<typename... Args>
  reentrancy_checked(const void*,Args&&... args):
    lck{std::forward<Args>(args)...}{}

  void unlock(){lck.unlock();}

  LockGuard lck;
};

#endif

} /* namespace foa */
} /* namespace detail */
} /* namespace unordered */
} /* namespace boost */

#endif
