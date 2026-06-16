#include "price_level.hpp"

#include <vector>

#include <gtest/gtest.h>

namespace {

Order make(uint64_t id, uint64_t price, uint64_t qty) {
  Order o{};
  o.id = id;
  o.side = Side::BUY;
  o.quantity = qty;
  o.price = price;
  o.type = OrderType::LIMIT;
  return o;
}

std::vector<uint64_t> ids_in_order(const PriceLevel &level) {
  std::vector<uint64_t> ids;
  for (const Order *o = level.front(); o != nullptr; o = o->next) {
    ids.push_back(o->id);
  }
  return ids;
}

} // namespace

TEST(PriceLevel, DefaultStateIsEmpty) {
  PriceLevel level(100);
  EXPECT_TRUE(level.empty());
  EXPECT_EQ(level.front(), nullptr);
  EXPECT_EQ(level.price(), 100u);
}

TEST(PriceLevel, PushBackSingleOrderBecomesHead) {
  PriceLevel level(100);
  Order o = make(1, 100, 5);
  level.push_back(&o);
  EXPECT_FALSE(level.empty());
  ASSERT_EQ(level.front(), &o);
  EXPECT_EQ(o.next, nullptr);
  EXPECT_EQ(o.prev, nullptr);
}

TEST(PriceLevel, PushBackMaintainsFifoOrdering) {
  PriceLevel level(100);
  Order a = make(1, 100, 1);
  Order b = make(2, 100, 2);
  Order c = make(3, 100, 3);
  level.push_back(&a);
  level.push_back(&b);
  level.push_back(&c);
  EXPECT_EQ(ids_in_order(level), (std::vector<uint64_t>{1, 2, 3}));
  EXPECT_EQ(a.prev, nullptr);
  EXPECT_EQ(a.next, &b);
  EXPECT_EQ(b.prev, &a);
  EXPECT_EQ(b.next, &c);
  EXPECT_EQ(c.prev, &b);
  EXPECT_EQ(c.next, nullptr);
}

TEST(PriceLevel, PopFrontEmptyReturnsNull) {
  PriceLevel level(100);
  EXPECT_EQ(level.pop_front(), nullptr);
  EXPECT_TRUE(level.empty());
}

TEST(PriceLevel, PopFrontReturnsHeadAndClearsLinks) {
  PriceLevel level(100);
  Order a = make(1, 100, 1);
  Order b = make(2, 100, 2);
  level.push_back(&a);
  level.push_back(&b);
  Order *popped = level.pop_front();
  ASSERT_EQ(popped, &a);
  EXPECT_EQ(popped->next, nullptr);
  EXPECT_EQ(popped->prev, nullptr);
  EXPECT_EQ(level.front(), &b);
  EXPECT_EQ(b.prev, nullptr);
}

TEST(PriceLevel, PopFrontAllOrdersLeavesEmpty) {
  PriceLevel level(100);
  Order a = make(1, 100, 1);
  Order b = make(2, 100, 2);
  level.push_back(&a);
  level.push_back(&b);
  EXPECT_EQ(level.pop_front(), &a);
  EXPECT_EQ(level.pop_front(), &b);
  EXPECT_TRUE(level.empty());
  EXPECT_EQ(level.pop_front(), nullptr);
}

TEST(PriceLevel, EraseHeadRelinksAndKeepsTail) {
  PriceLevel level(100);
  Order a = make(1, 100, 1);
  Order b = make(2, 100, 2);
  Order c = make(3, 100, 3);
  level.push_back(&a);
  level.push_back(&b);
  level.push_back(&c);

  level.erase(&a);
  EXPECT_EQ(ids_in_order(level), (std::vector<uint64_t>{2, 3}));
  EXPECT_EQ(b.prev, nullptr);
  EXPECT_EQ(a.next, nullptr);
  EXPECT_EQ(a.prev, nullptr);
}

TEST(PriceLevel, EraseTailRelinksAndKeepsHead) {
  PriceLevel level(100);
  Order a = make(1, 100, 1);
  Order b = make(2, 100, 2);
  Order c = make(3, 100, 3);
  level.push_back(&a);
  level.push_back(&b);
  level.push_back(&c);

  level.erase(&c);
  EXPECT_EQ(ids_in_order(level), (std::vector<uint64_t>{1, 2}));
  EXPECT_EQ(b.next, nullptr);
  EXPECT_EQ(c.next, nullptr);
  EXPECT_EQ(c.prev, nullptr);

  // Push back again to confirm tail was updated correctly.
  Order d = make(4, 100, 4);
  level.push_back(&d);
  EXPECT_EQ(ids_in_order(level), (std::vector<uint64_t>{1, 2, 4}));
  EXPECT_EQ(b.next, &d);
  EXPECT_EQ(d.prev, &b);
}

TEST(PriceLevel, EraseMiddleRelinksNeighbors) {
  PriceLevel level(100);
  Order a = make(1, 100, 1);
  Order b = make(2, 100, 2);
  Order c = make(3, 100, 3);
  level.push_back(&a);
  level.push_back(&b);
  level.push_back(&c);

  level.erase(&b);
  EXPECT_EQ(ids_in_order(level), (std::vector<uint64_t>{1, 3}));
  EXPECT_EQ(a.next, &c);
  EXPECT_EQ(c.prev, &a);
  EXPECT_EQ(b.next, nullptr);
  EXPECT_EQ(b.prev, nullptr);
}

TEST(PriceLevel, EraseOnlyOrderEmptiesLevel) {
  PriceLevel level(100);
  Order a = make(1, 100, 1);
  level.push_back(&a);
  level.erase(&a);
  EXPECT_TRUE(level.empty());
  EXPECT_EQ(level.front(), nullptr);
}

TEST(PriceLevel, PushBackAfterEraseRebuildsListCleanly) {
  PriceLevel level(100);
  Order a = make(1, 100, 1);
  Order b = make(2, 100, 2);
  level.push_back(&a);
  level.erase(&a);
  level.push_back(&b);
  EXPECT_EQ(ids_in_order(level), (std::vector<uint64_t>{2}));
  EXPECT_EQ(b.prev, nullptr);
  EXPECT_EQ(b.next, nullptr);
}

TEST(PriceLevel, PushBackNullptrIsNoOp) {
  PriceLevel level(100);
  level.push_back(nullptr);
  EXPECT_TRUE(level.empty());
  EXPECT_EQ(level.front(), nullptr);
}
