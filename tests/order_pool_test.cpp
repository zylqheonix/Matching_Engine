#include "order_pool.hpp"

#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

TEST(OrderPool, AllocateReturnsNonNullPointer) {
  OrderPool pool;
  Order *o = pool.allocate();
  ASSERT_NE(o, nullptr);
  pool.deallocate(o);
}

TEST(OrderPool, FreshAllocationsAreDistinct) {
  OrderPool pool;
  std::unordered_set<Order *> seen;
  for (int i = 0; i < 256; ++i) {
    Order *o = pool.allocate();
    ASSERT_NE(o, nullptr);
    EXPECT_TRUE(seen.insert(o).second)
        << "duplicate pointer on allocation " << i;
  }
}

TEST(OrderPool, AllocatedOrdersCanBeWrittenAndReadBack) {
  OrderPool pool;
  Order *o = pool.allocate();
  ASSERT_NE(o, nullptr);
  o->id = 7;
  o->side = Side::SELL;
  o->quantity = 42;
  o->price = 100;
  o->type = OrderType::LIMIT;
  EXPECT_EQ(o->id, 7u);
  EXPECT_EQ(o->side, Side::SELL);
  EXPECT_EQ(o->quantity, 42u);
  ASSERT_TRUE(o->price.has_value());
  EXPECT_EQ(*o->price, 100u);
  EXPECT_EQ(o->type, OrderType::LIMIT);
  pool.deallocate(o);
}

TEST(OrderPool, DeallocatedSlotIsReusedByNextAllocation) {
  OrderPool pool;
  Order *first = pool.allocate();
  ASSERT_NE(first, nullptr);
  pool.deallocate(first);

  Order *second = pool.allocate();
  EXPECT_EQ(second, first);
  pool.deallocate(second);
}

TEST(OrderPool, ManyAllocationsAcrossChunkBoundaryAreUnique) {
  // chunk_size is 512 in the current pool implementation; allocate more than
  // that to exercise the second-chunk grow path.
  constexpr int kCount = 1500;
  OrderPool pool;
  std::vector<Order *> ptrs;
  ptrs.reserve(kCount);
  std::unordered_set<Order *> seen;
  for (int i = 0; i < kCount; ++i) {
    Order *o = pool.allocate();
    ASSERT_NE(o, nullptr);
    EXPECT_TRUE(seen.insert(o).second);
    ptrs.push_back(o);
  }
  for (Order *o : ptrs) {
    pool.deallocate(o);
  }
}

TEST(OrderPool, ChurnAlternatingAllocateDeallocateIsStable) {
  OrderPool pool;
  std::vector<Order *> ptrs;
  for (int i = 0; i < 10000; ++i) {
    Order *o = pool.allocate();
    ASSERT_NE(o, nullptr);
    o->id = static_cast<uint64_t>(i);
    ptrs.push_back(o);
    if (ptrs.size() >= 64) {
      pool.deallocate(ptrs.front());
      ptrs.erase(ptrs.begin());
    }
  }
  for (Order *o : ptrs) {
    pool.deallocate(o);
  }
}
