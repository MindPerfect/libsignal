/** \file Std_t.cc 
 * Test definitions for the std23 tests.
 *
 * (c) Copyright by MindPerfect Technologies
 * All rights reserved, see COPYRIGHT file for details.
 *
 * $Id$
 *
 *
 */

#include "function.h"
#include "function_ref.h"
#include "move_only_function.h"
// Std includes
// Google Test
#include <gtest/gtest.h>

using namespace std;
using namespace std23;




TEST(function, Base)
{
	{
	 	std23::function<int()> fn;
		ASSERT_TRUE(!fn);
		ASSERT_TRUE(fn==nullptr);
		ASSERT_TRUE(nullptr==fn);
	}
	{
		std23::function fn = [] { return 42; };
		ASSERT_TRUE(bool(fn));
		ASSERT_TRUE(fn!=nullptr);
		ASSERT_TRUE(nullptr!=fn);
	}
}


constexpr auto call = [](function_ref<int()> fr) { return fr(); };
inline int f() { return 0; }


template<auto N> struct int_c //-: detail::op
{
    using value_type = decltype(N);
    static constexpr auto value = N;

    [[nodiscard]] constexpr operator value_type() const { return N; }
    [[nodiscard]] constexpr auto get() const { return N; }
};

inline constexpr int_c<0> free_function;
inline constexpr int_c<1> function_template;
inline constexpr int_c<2> non_const;


TEST(function_ref, Base)
{
	ASSERT_TRUE(call(f) == free_function);
}


TEST(move_only_function, Base)
{
	{
		std23::move_only_function<int()> fn; // Empty
		ASSERT_TRUE(!fn);
		ASSERT_TRUE(fn==nullptr);
		ASSERT_TRUE(nullptr==fn);
	}
	{
		std23::move_only_function<int()> fn = f; // Not empty
		ASSERT_TRUE(bool(fn));
		ASSERT_TRUE(fn!=nullptr);
		ASSERT_TRUE(nullptr!=fn);

		// When called
		ASSERT_TRUE(fn()==0);
	}
}

