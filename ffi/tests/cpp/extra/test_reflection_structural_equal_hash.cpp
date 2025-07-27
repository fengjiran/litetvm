//
// Created by 赵丹 on 25-7-22.
//
#include "../testing_object.h"
#include "ffi/container/array.h"
#include "ffi/container/map.h"
#include "ffi/object.h"
#include "ffi/reflection/registry.h"
#include "ffi/reflection/structural_equal.h"
#include "ffi/reflection/structural_hash.h"
#include "ffi/string.h"

#include <gtest/gtest.h>

namespace {

using namespace litetvm::ffi;
using namespace litetvm::ffi::testing;
namespace refl = reflection;

TEST(StructuralEqualHash, Array) {
    auto structural_cmp = refl::StructuralEqual();
    auto structural_hash = refl::StructuralHash();
    Array<int> a = {1, 2, 3};
    Array<int> b = {1, 2, 3};
    Array<int> c = {1, 3};

    EXPECT_TRUE(structural_cmp(a, b));
    EXPECT_EQ(structural_hash(a), structural_hash(b));
    EXPECT_FALSE(structural_cmp(a, c));
    EXPECT_NE(structural_hash(a), structural_hash(c));

    auto diff_a_c = refl::StructuralEqual::GetFirstMismatch(a, c);
    // first directly interepret diff,
    EXPECT_TRUE(diff_a_c.has_value());
    EXPECT_EQ((*diff_a_c).get<0>()[0]->kind, refl::AccessKind::kArrayIndex);
    EXPECT_EQ((*diff_a_c).get<1>()[0]->kind, refl::AccessKind::kArrayIndex);
    EXPECT_EQ((*diff_a_c).get<0>()[0]->key.cast<int64_t>(), 1);
    EXPECT_EQ((*diff_a_c).get<1>()[0]->key.cast<int64_t>(), 1);
    EXPECT_EQ((*diff_a_c).get<0>().size(), 1);
    EXPECT_EQ((*diff_a_c).get<1>().size(), 1);

    // use structural equal for checking in future parts
    // given we have done some basic checks above by directly interepret diff,
    Array<int> d = {1, 2};
    auto diff_a_d = refl::StructuralEqual::GetFirstMismatch(a, d);
    auto expected_diff_a_d = refl::AccessPathPair(refl::AccessPath({refl::AccessStep::ArrayIndex(2)}),
                                                  refl::AccessPath({refl::AccessStep::ArrayIndexMissing(2)}));
    // then use structural equal to check it
    EXPECT_TRUE(structural_cmp(diff_a_d, expected_diff_a_d));
}

TEST(StructuralEqualHash, Map) {
    auto structural_cmp = refl::StructuralEqual();
    auto structural_hash = refl::StructuralHash();
    // same map but different insertion order
    Map<String, int> a = {{"a", 1}, {"b", 2}, {"c", 3}};
    Map<String, int> b = {{"b", 2}, {"c", 3}, {"a", 1}};
    EXPECT_TRUE(structural_cmp(a, b));
    EXPECT_EQ(structural_hash(a), structural_hash(b));

    Map<String, int> c = {{"a", 1}, {"b", 2}, {"c", 4}};
    EXPECT_FALSE(structural_cmp(a, c));
    EXPECT_NE(structural_hash(a), structural_hash(c));

    auto diff_a_c = refl::StructuralEqual::GetFirstMismatch(a, c);
    auto expected_diff_a_c = refl::AccessPathPair(refl::AccessPath({refl::AccessStep::MapKey("c")}),
                                                  refl::AccessPath({refl::AccessStep::MapKey("c")}));
    EXPECT_TRUE(diff_a_c.has_value());
    EXPECT_TRUE(structural_cmp(diff_a_c, expected_diff_a_c));
}

TEST(StructuralEqualHash, NestedMapArray) {
    auto structural_cmp = refl::StructuralEqual();
    auto structural_hash = refl::StructuralHash();
    Map<String, Array<Any>> a = {{"a", {1, 2, 3}}, {"b", {4, "hello", 6}}};
    Map<String, Array<Any>> b = {{"a", {1, 2, 3}}, {"b", {4, "hello", 6}}};
    EXPECT_TRUE(structural_cmp(a, b));
    EXPECT_EQ(structural_hash(a), structural_hash(b));

    Map<String, Array<Any>> c = {{"a", {1, 2, 3}}, {"b", {4, "world", 6}}};
    EXPECT_FALSE(structural_cmp(a, c));
    EXPECT_NE(structural_hash(a), structural_hash(c));

    auto diff_a_c = refl::StructuralEqual::GetFirstMismatch(a, c);
    auto expected_diff_a_c = refl::AccessPathPair(refl::AccessPath({refl::AccessStep::MapKey("b"),
                                                                    refl::AccessStep::ArrayIndex(1)}),
                                                  refl::AccessPath({refl::AccessStep::MapKey("b"),
                                                                    refl::AccessStep::ArrayIndex(1)}));
    EXPECT_TRUE(diff_a_c.has_value());
    EXPECT_TRUE(structural_cmp(diff_a_c, expected_diff_a_c));

    Map<String, Array<Any>> d = {{"a", {1, 2, 3}}};
    auto diff_a_d = refl::StructuralEqual::GetFirstMismatch(a, d);
    auto expected_diff_a_d = refl::AccessPathPair(refl::AccessPath({refl::AccessStep::MapKey("b")}),
                                                  refl::AccessPath({refl::AccessStep::MapKeyMissing("b")}));
    EXPECT_TRUE(diff_a_d.has_value());
    EXPECT_TRUE(structural_cmp(diff_a_d, expected_diff_a_d));

    auto diff_d_a = refl::StructuralEqual::GetFirstMismatch(d, a);
    auto expected_diff_d_a = refl::AccessPathPair(refl::AccessPath({refl::AccessStep::MapKeyMissing("b")}),
                                                  refl::AccessPath({refl::AccessStep::MapKey("b")}));
}

TEST(StructuralEqualHash, FreeVar) {
    TVar a = TVar("a");
    TVar b = TVar("b");
    EXPECT_TRUE(refl::StructuralEqual::Equal(a, b, /*map_free_vars=*/true));
    EXPECT_FALSE(refl::StructuralEqual::Equal(a, b));

    EXPECT_NE(refl::StructuralHash()(a), refl::StructuralHash()(b));
    EXPECT_EQ(refl::StructuralHash::Hash(a, /*map_free_vars=*/true),
              refl::StructuralHash::Hash(b, /*map_free_vars=*/true));
}

TEST(StructuralEqualHash, FuncDefAndIgnoreField) {
    TVar x = TVar("x");
    TVar y = TVar("y");
    // comment fields are ignored
    TFunc fa = TFunc({x}, {TInt(1), x}, "comment a");
    TFunc fb = TFunc({y}, {TInt(1), y}, "comment b");

    TFunc fc = TFunc({x}, {TInt(1), TInt(2)}, "comment c");
    EXPECT_TRUE(refl::StructuralEqual()(fa, fb));
    EXPECT_EQ(refl::StructuralHash()(fa), refl::StructuralHash()(fb));

    EXPECT_FALSE(refl::StructuralEqual()(fa, fc));
    auto diff_fa_fc = refl::StructuralEqual::GetFirstMismatch(fa, fc);
    auto expected_diff_fa_fc = refl::AccessPathPair(refl::AccessPath({
                                                            refl::AccessStep::ObjectField("body"),
                                                            refl::AccessStep::ArrayIndex(1),
                                                    }),
                                                    refl::AccessPath({
                                                            refl::AccessStep::ObjectField("body"),
                                                            refl::AccessStep::ArrayIndex(1),
                                                    }));
    EXPECT_TRUE(diff_fa_fc.has_value());
    EXPECT_TRUE(refl::StructuralEqual()(diff_fa_fc, expected_diff_fa_fc));
}

TEST(StructuralEqualHash, CustomTreeNode) {
    TVar x = TVar("x");
    TVar y = TVar("y");
    // comment fields are ignored
    TCustomFunc fa = TCustomFunc({x}, {TInt(1), x}, "comment a");
    TCustomFunc fb = TCustomFunc({y}, {TInt(1), y}, "comment b");

    TCustomFunc fc = TCustomFunc({x}, {TInt(1), TInt(2)}, "comment c");

    EXPECT_TRUE(refl::StructuralEqual()(fa, fb));
    EXPECT_EQ(refl::StructuralHash()(fa), refl::StructuralHash()(fb));

    EXPECT_FALSE(refl::StructuralEqual()(fa, fc));
    auto diff_fa_fc = refl::StructuralEqual::GetFirstMismatch(fa, fc);
    auto expected_diff_fa_fc = refl::AccessPathPair(refl::AccessPath({
                                                            refl::AccessStep::ObjectField("body"),
                                                            refl::AccessStep::ArrayIndex(1),
                                                    }),
                                                    refl::AccessPath({
                                                            refl::AccessStep::ObjectField("body"),
                                                            refl::AccessStep::ArrayIndex(1),
                                                    }));
    EXPECT_TRUE(diff_fa_fc.has_value());
    EXPECT_TRUE(refl::StructuralEqual()(diff_fa_fc, expected_diff_fa_fc));
}

}// namespace