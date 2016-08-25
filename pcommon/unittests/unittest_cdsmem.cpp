/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_cdsmem.cpp
 COPYRIGHT    :   Yakov Markovitch, 2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests of memory managers for concurrent data structures.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   23 Aug 2016
*******************************************************************************/
#include <pcomn_cdsmem.h>
#include <pcomn_iterator.h>
#include <pcomn_unittest.h>

#include "pcomn_testcds.h"

#include <thread>
#include <numeric>

using namespace pcomn ;

/*******************************************************************************
 class CDSMemTests
*******************************************************************************/
class CDSMemTests : public CppUnit::TestFixture {

   private:
      void Test_Block_Malloc_Allocator() ;
      void Test_Block_Page_Allocator() ;
      void Test_Concurrent_Freestack_SingleThread() ;
      void Test_Freepool_Ring_SingleThread() ;

      CPPUNIT_TEST_SUITE(CDSMemTests) ;

      CPPUNIT_TEST(Test_Block_Malloc_Allocator) ;
      CPPUNIT_TEST(Test_Block_Page_Allocator) ;
      CPPUNIT_TEST(Test_Concurrent_Freestack_SingleThread) ;
      CPPUNIT_TEST(Test_Freepool_Ring_SingleThread) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 CDSMemTests
*******************************************************************************/
void CDSMemTests::Test_Block_Malloc_Allocator()
{
   CPPUNIT_LOG_EXCEPTION_MSG((malloc_block_allocator{0}), std::invalid_argument, "size:0") ;
   CPPUNIT_LOG_EXCEPTION_MSG((malloc_block_allocator{0, 16}), std::invalid_argument, "size:0") ;
   CPPUNIT_LOG_RUN((malloc_block_allocator{1, 3})) ;

   //malloc_block_allocator mallocator0 ;
}

void CDSMemTests::Test_Block_Page_Allocator()
{
}

void CDSMemTests::Test_Concurrent_Freestack_SingleThread()
{
   CPPUNIT_LOG_IS_FALSE(std::is_copy_constructible<concurrent_freestack>::value) ;
   CPPUNIT_LOG_IS_FALSE(std::is_move_constructible<concurrent_freestack>::value) ;
   CPPUNIT_LOG_IS_FALSE(std::is_copy_assignable<concurrent_freestack>::value) ;
   CPPUNIT_LOG_IS_FALSE(std::is_move_assignable<concurrent_freestack>::value) ;

   concurrent_freestack zero_stack (0U) ;

   CPPUNIT_LOG_EQUAL(concurrent_freestack(concurrent_freestack::max_size_limit()).max_size(), concurrent_freestack::max_size_limit()) ;
   CPPUNIT_LOG_EQ(concurrent_freestack(1).max_size(), 1) ;
   CPPUNIT_LOG_EXCEPTION(concurrent_freestack(concurrent_freestack::max_size_limit() + 1), std::length_error) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQ(zero_stack.size(), 0) ;
   CPPUNIT_LOG_ASSERT(zero_stack.empty()) ;
   CPPUNIT_LOG_IS_NULL(zero_stack.pop()) ;
   CPPUNIT_LOG_EQ(zero_stack.size(), 0) ;
   CPPUNIT_LOG_IS_FALSE(zero_stack.push(nullptr)) ;
   CPPUNIT_LOG_EQ(zero_stack.size(), 0) ;
   CPPUNIT_LOG_ASSERT(zero_stack.empty()) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EXCEPTION(concurrent_freestack{nullptr}, std::invalid_argument) ;

   unsigned msz1 ;
   CPPUNIT_LOG_RUN(msz1 = concurrent_freestack::max_size_limit() + 1) ;
   CPPUNIT_LOG_EXCEPTION(concurrent_freestack{&msz1}, std::length_error) ;

   CPPUNIT_LOG_RUN(msz1 = 1) ;

   concurrent_freestack one_stack {&msz1} ;

   CPPUNIT_LOG_EQ(one_stack.max_size(), 1) ;
   CPPUNIT_LOG_RUN(msz1 = concurrent_freestack::max_size_limit() + 1) ;
   CPPUNIT_LOG_EQ(one_stack.max_size(), concurrent_freestack::max_size_limit()) ;
   CPPUNIT_LOG_RUN(msz1 = 1) ;
   CPPUNIT_LOG_EQ(one_stack.max_size(), 1) ;

   CPPUNIT_LOG(std::endl) ;
   typedef std::pair<void *, uintptr_t> test_item ;
   test_item items[32] ;
   for (test_item &i: items)
      i.second = &i - items ;

   CPPUNIT_LOG_EQ(one_stack.size(), 0) ;
   CPPUNIT_LOG_ASSERT(one_stack.empty()) ;
   CPPUNIT_LOG_IS_NULL(one_stack.pop()) ;
   CPPUNIT_LOG_EQ(one_stack.size(), 0) ;
   CPPUNIT_LOG_ASSERT(one_stack.empty()) ;

   CPPUNIT_LOG_ASSERT(one_stack.push(items + 2)) ;
   CPPUNIT_LOG_IS_FALSE(one_stack.push(items + 5)) ;

   CPPUNIT_LOG_IS_FALSE(one_stack.empty()) ;
   CPPUNIT_LOG_EQ(one_stack.size(), 1) ;

   CPPUNIT_LOG_EQUAL(items[2], test_item(0, 2)) ;
   CPPUNIT_LOG_EQUAL(items[5], test_item(0, 5)) ;
   CPPUNIT_LOG_EQ(one_stack.pop(), items + 2) ;

   CPPUNIT_LOG_EQ(one_stack.size(), 0) ;
   CPPUNIT_LOG_ASSERT(one_stack.empty()) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_ASSERT(one_stack.push(items + 6)) ;
   CPPUNIT_LOG_IS_FALSE(one_stack.push(items + 8)) ;
   CPPUNIT_LOG_EQUAL(items[8], test_item(0, 8)) ;

   CPPUNIT_LOG_RUN(msz1 = 4) ;
   CPPUNIT_LOG_ASSERT(one_stack.push(items + 8)) ;
   CPPUNIT_LOG_EQUAL(items[8], test_item(items + 6, 8)) ;

   CPPUNIT_LOG_EQ(one_stack.size(), 2) ;
   CPPUNIT_LOG_IS_FALSE(one_stack.empty()) ;

   CPPUNIT_LOG_ASSERT(one_stack.push(items + 3)) ;
   CPPUNIT_LOG_ASSERT(one_stack.push(items + 4)) ;

   CPPUNIT_LOG_IS_FALSE(one_stack.push(items + 5)) ;

   CPPUNIT_LOG_EQ(one_stack.size(), 4) ;
   CPPUNIT_LOG_IS_FALSE(one_stack.empty()) ;

   CPPUNIT_LOG_EQUAL(items[6], test_item(0, 6)) ;
   CPPUNIT_LOG_EQUAL(items[8], test_item(items + 6, 8)) ;
   CPPUNIT_LOG_EQUAL(items[5], test_item(0, 5)) ;
   CPPUNIT_LOG_EQUAL(items[4], test_item(items + 3, 4)) ;
}

void CDSMemTests::Test_Freepool_Ring_SingleThread()
{
   typedef concurrent_freepool_ring<> ring ;

   //malloc_block_allocator mallocator ;
   //ring r01
}

/*******************************************************************************

*******************************************************************************/
int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(CDSMemTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, nullptr, "Tests of CRQ and LCRQ") ;
}
