// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../mock_error.hpp"
#include "../undefined.hpp"
#include <turtle/constraints.hpp>
#include <turtle/detail/function.hpp>
#include <turtle/detail/void_t.hpp>
#include <boost/test/unit_test.hpp>
#include <functional>
#include <memory>
#include <type_traits>

// static

namespace {
std::function<void()> static_f;

static_assert(std::is_same<void, decltype(mock::detail::function<void()>{}())>::value, "!");
static_assert(std::is_same<int, decltype(mock::detail::function<int()>{}())>::value, "!");
static_assert(std::is_same<void, decltype(mock::detail::function<void(float)>{}(std::declval<float>()))>::value, "!");
static_assert(std::is_same<int, decltype(mock::detail::function<int(float)>{}(std::declval<float>()))>::value, "!");

template<class TFunction, typename... TParams>
using with_call_t = decltype(std::declval<TFunction>().expect().with(std::declval<TParams>()...));
template<class AlwaysVoid, class TFunction, typename... TParams>
struct with_is_callable : std::false_type
{};
template<class TFunction, class... TParams>
struct with_is_callable<mock::detail::void_t<with_call_t<TFunction, TParams...>>, TFunction, TParams...> :
    std::true_type
{};
// Detect if we can call function.expect().with(params)
template<class TFunction, class... TParams>
using with_is_callable_t = with_is_callable<void, TFunction, TParams...>;

// For function with no parameters `with` can never be called
using function_0_args = mock::detail::function<int()>;
static_assert(!with_is_callable_t<function_0_args>::value, "Can't call with()");
static_assert(!with_is_callable_t<function_0_args, int>::value, "Can't call with(int)");
static_assert(!with_is_callable_t<function_0_args, bool()>::value, "Can't call with custom constraint");

// For functions with parameters `with` can be called with a custom constraint (taking all params)
// or exactly as many parameters as the function has (here 1 and 4 respectively)
using function_1_args = mock::detail::function<int(int)>;
static_assert(!with_is_callable_t<function_1_args>::value, "Can't call with()");
static_assert(with_is_callable_t<function_1_args, int>::value, "Can call with(int)");
static_assert(with_is_callable_t<function_1_args, bool(int)>::value, "Can call with custom constraint");
static_assert(!with_is_callable_t<function_1_args, int, int>::value, "Can't call with(int, int)");

using function_4_args = mock::detail::function<int(int, int, int, int)>;
static_assert(!with_is_callable_t<function_4_args>::value, "Can't call with()");
static_assert(with_is_callable_t<function_4_args, bool(int, int, int, int)>::value, "Can call with custom constraint");
static_assert(!with_is_callable_t<function_4_args, int, int>::value, "Can't call with(int, int)");
static_assert(!with_is_callable_t<function_4_args, int, int, int>::value, "Can't call with(int, int, int)");
static_assert(with_is_callable_t<function_4_args, int, int, int, int>::value, "Can call with(int, int, int, int)");
static_assert(!with_is_callable_t<function_4_args, int, int, int, int, int>::value,
              "Can't call with(int, int, int, int, int)");
} // namespace

// functor

BOOST_FIXTURE_TEST_CASE(a_function_can_be_passed_as_functor, mock_error_fixture)
{
    mock::detail::function<void()> f;
    std::function<void()> functor = f;
}

BOOST_FIXTURE_TEST_CASE(a_function_can_be_passed_as_functor_using_std_bind_and_std_ref, mock_error_fixture)
{
    mock::detail::function<void()> f;
    std::function<void()> functor = std::bind(std::ref(f));
}

// invocations

BOOST_FIXTURE_TEST_CASE(triggering_an_unconfigured_function_calls_unexpected_call_error, mock_error_fixture)
{
    {
        mock::detail::function<void()> f;
        CHECK_ERROR(f(), "unexpected call", 0, "?()");
    }
    {
        mock::detail::function<int(int, const std::string&)> f;
        CHECK_ERROR(f(1, "s"), "unexpected call", 0, "?( 1, \"s\" )");
    }
}

BOOST_FIXTURE_TEST_CASE(triggering_a_never_expectation_calls_unexpected_call_error, mock_error_fixture)
{
    {
        mock::detail::function<void()> f;
        f.expect().never();
        CHECK_ERROR(f(), "unexpected call", 0, "?()\nv never()");
    }
    {
        mock::detail::function<int(int, const std::string&)> f;
        f.expect().never();
        CHECK_ERROR(f(1, "s"), "unexpected call", 0, "?( 1, \"s\" )\nv never().with( any, any )");
    }
}

BOOST_FIXTURE_TEST_CASE(triggering_an_unlimited_expectation_is_valid, mock_error_fixture)
{
    {
        mock::detail::function<void()> f;
        f.expect();
        f();
        f();
        CHECK_CALLS(2);
    }
    {
        mock::detail::function<void(int, std::string)> f;
        f.expect();
        f(1, "s1");
        f(2, "s2");
        CHECK_CALLS(2);
    }
    {
        mock::detail::function<void(int, const std::string&)> f;
        f.expect();
        f(1, "s1");
        f(2, "s2");
        CHECK_CALLS(2);
    }
}

BOOST_FIXTURE_TEST_CASE(triggering_several_once_expectations_is_valid, mock_error_fixture)
{
    {
        mock::detail::function<void()> f;
        f.expect().once();
        f.expect().once();
        f();
        f();
        CHECK_CALLS(2);
    }
    {
        mock::detail::function<void(int, std::string)> f;
        f.expect().once().with(1, "s1");
        f.expect().once().with(2, "s2");
        f(1, "s1");
        f(2, "s2");
        CHECK_CALLS(2);
    }
    {
        mock::detail::function<void(int, const std::string&)> f;
        f.expect().once().with(1, "s1");
        f.expect().once().with(2, "s2");
        f(1, "s1");
        f(2, "s2");
        CHECK_CALLS(2);
    }
}

BOOST_FIXTURE_TEST_CASE(triggering_a_once_expectation_calls_unexpected_call_error_after_one_call, mock_error_fixture)
{
    {
        mock::detail::function<void()> f;
        f.expect().once();
        f();
        CHECK_ERROR(f(), "unexpected call", 1, "?()\nv once()");
    }
    {
        mock::detail::function<void(int, const std::string&)> f;
        f.expect().once();
        f(1, "s");
        CHECK_ERROR(f(1, "s"), "unexpected call", 1, "?( 1, \"s\" )\nv once().with( any, any )");
    }
}

BOOST_FIXTURE_TEST_CASE(literal_zero_can_be_used_in_function_call_as_pointers, mock_error_fixture)
{
    mock::detail::function<void(int*)> f;
    f.expect().once();
    f(0);
    CHECK_CALLS(1);
}

// verify

BOOST_FIXTURE_TEST_CASE(verifying_an_unconfigured_function_succeeds, mock_error_fixture)
{
    {
        mock::detail::function<void()> f;
        BOOST_CHECK(f.verify());
    }
    {
        mock::detail::function<int(int, const std::string&)> f;
        BOOST_CHECK(f.verify());
    }
}

BOOST_FIXTURE_TEST_CASE(verifying_an_unlimited_expectation_succeeds, mock_error_fixture)
{
    {
        mock::detail::function<void()> f;
        f.expect();
        BOOST_CHECK(f.verify());
        CHECK_CALLS(0);
    }
    {
        mock::detail::function<int(int, const std::string&)> f;
        f.expect();
        BOOST_CHECK(f.verify());
        CHECK_CALLS(0);
    }
}

BOOST_FIXTURE_TEST_CASE(verifying_a_once_expectation_after_one_call_succeeds, mock_error_fixture)
{
    {
        mock::detail::function<void()> f;
        f.expect().once();
        f();
        BOOST_CHECK(f.verify());
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<void(int, const std::string&)> f;
        f.expect().once();
        f(1, "s");
        BOOST_CHECK(f.verify());
        CHECK_CALLS(1);
    }
}

BOOST_FIXTURE_TEST_CASE(verifying_a_once_expectation_before_the_call_fails, mock_error_fixture)
{
    {
        mock::detail::function<void()> f;
        f.expect().once();
        CHECK_ERROR(BOOST_CHECK(!f.verify()), "verification failed", 0, "?\n. once()");
    }
    {
        mock::detail::function<int(int, const std::string&)> f;
        f.expect().once();
        CHECK_ERROR(BOOST_CHECK(!f.verify()), "verification failed", 0, "?\n. once().with( any, any )");
    }
}

BOOST_FIXTURE_TEST_CASE(verifying_a_once_expectation_after_a_verify_and_one_call_succeeds, mock_error_fixture)
{
    mock::detail::function<void()> f;
    f.expect().once();
    CHECK_ERROR(BOOST_CHECK(!f.verify()), "verification failed", 0, "?\n. once()");
    f();
    BOOST_CHECK(f.verify());
    CHECK_CALLS(1);
}

// reset

BOOST_FIXTURE_TEST_CASE(triggering_a_reset_function_calls_unexpected_call_error, mock_error_fixture)
{
    {
        mock::detail::function<void()> f;
        f.expect();
        f.reset();
        CHECK_ERROR(f(), "unexpected call", 0, "?()");
    }
    {
        mock::detail::function<int(int, const std::string&)> f;
        f.expect();
        f.reset();
        CHECK_ERROR(f(1, "s"), "unexpected call", 0, "?( 1, \"s\" )");
    }
}

BOOST_FIXTURE_TEST_CASE(verifying_a_reset_function_succeeds, mock_error_fixture)
{
    {
        mock::detail::function<void()> f;
        f.expect();
        f.reset();
        BOOST_CHECK(f.verify());
    }
    {
        mock::detail::function<int(int, const std::string&)> f;
        f.expect();
        f.reset();
        BOOST_CHECK(f.verify());
    }
}

// constraints

BOOST_FIXTURE_TEST_CASE(
  triggering_an_expectation_with_wrong_parameter_value_in_equal_constraint_calls_unexpected_call_error,
  mock_error_fixture)
{
    {
        mock::detail::function<void(int)> f;
        f.expect().with(42);
        CHECK_ERROR(f(43), "unexpected call", 0, "?( 43 )\n. unlimited().with( 42 )");
    }
    {
        mock::detail::function<int(int, const std::string&)> f;
        f.expect().with(42, "expected");
        CHECK_ERROR(
          f(42, "actual"), "unexpected call", 0, "?( 42, \"actual\" )\n. unlimited().with( 42, \"expected\" )");
    }
}

BOOST_FIXTURE_TEST_CASE(
  triggering_an_expectation_with_wrong_parameter_value_in_equal_or_less_constraint_calls_unexpected_call_error,
  mock_error_fixture)
{
    mock::detail::function<void(int)> f;
    f.expect().with(mock::equal(42) || mock::less(42));
    f(41);
    f(42);
    CHECK_ERROR(f(43), "unexpected call", 2, "?( 43 )\n. unlimited().with( ( equal( 42 ) || less( 42 ) ) )");
}

BOOST_FIXTURE_TEST_CASE(
  triggering_an_expectation_with_wrong_parameter_value_in_equal_and_not_less_constraint_calls_unexpected_call_error,
  mock_error_fixture)
{
    mock::detail::function<void(int)> f;
    f.expect().with(mock::equal(42) && !mock::less(41));
    f(42);
    CHECK_ERROR(f(43), "unexpected call", 1, "?( 43 )\n. unlimited().with( ( equal( 42 ) && ! less( 41 ) ) )");
}

namespace {
class my_interface
{
public:
    my_interface() = default;
    my_interface(const my_interface&) = delete;
    my_interface& operator=(const my_interface&) = delete;
    virtual ~my_interface() = default;

private:
    virtual void my_method() = 0;
};
class my_implementation : public my_interface
{
    virtual void my_method() {}
};
} // namespace

BOOST_FIXTURE_TEST_CASE(passing_call_values_by_reference_is_transparent, mock_error_fixture)
{
    {
        mock::detail::function<void(my_interface&)> f;
        my_implementation imp;
        f.expect().with(mock::same(imp));
        f(imp);
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<void(const my_interface&)> f;
        my_implementation imp;
        f.expect().with(mock::same(imp));
        f(imp);
        CHECK_CALLS(1);
    }
}

namespace {
bool custom_constraint(int)
{
    return false;
}
} // namespace

BOOST_FIXTURE_TEST_CASE(triggering_an_expectation_with_failing_custom_constraint_calls_unexpected_call_error,
                        mock_error_fixture)
{
    {
        mock::detail::function<void(int)> f;
        f.expect().with(&custom_constraint);
        CHECK_ERROR(f(42), "unexpected call", 0, "?( 42 )\n. unlimited().with( ? )");
    }
    {
        mock::detail::function<int(int, const std::string&)> f;
        f.expect().with(&custom_constraint, "actual");
        CHECK_ERROR(f(42, "actual"), "unexpected call", 0, "?( 42, \"actual\" )\n. unlimited().with( ?, \"actual\" )");
    }
}

// BOOST_FIXTURE_TEST_CASE( literal_zero_can_be_used_in_place_of_null_pointers_in_constraints, mock_error_fixture )
//{
//   mock::detail::function< void( int* ) > f;
//   f.expect().with( 0 );
//   f.reset();
//   CHECK_CALLS( 1 );
//}

BOOST_FIXTURE_TEST_CASE(nullptr_can_be_used_in_place_of_null_pointers_in_constraints, mock_error_fixture)
{
    mock::detail::function<void(int*)> f;
    f.expect().with(nullptr);
    f(0);
    CHECK_CALLS(1);
}

BOOST_FIXTURE_TEST_CASE(unique_ptr_is_supported_as_parameter, mock_error_fixture)
{
    mock::detail::function<void(std::unique_ptr<int>)> f;
    f.expect();
    std::unique_ptr<int> p(new int);
    f(std::move(p));
    CHECK_CALLS(1);
}

// result handling

BOOST_FIXTURE_TEST_CASE(triggering_an_expectation_with_no_return_set_calls_missing_action, mock_error_fixture)
{
    {
        mock::detail::function<int()> f;
        f.expect();
        CHECK_ERROR(f(), "missing action", 0, "?()\n. unlimited()");
    }
    {
        mock::detail::function<int&()> f;
        f.expect();
        CHECK_ERROR(f(), "missing action", 0, "?()\n. unlimited()");
    }
    {
        mock::detail::function<const std::string&()> f;
        f.expect();
        CHECK_ERROR(f(), "missing action", 0, "?()\n. unlimited()");
    }
}

BOOST_FIXTURE_TEST_CASE(triggering_an_expectation_returns_the_set_value, mock_error_fixture)
{
    {
        mock::detail::function<int()> f;
        f.expect().returns(42);
        BOOST_CHECK_EQUAL(42, f());
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<int()> f;
        const int i = 42;
        f.expect().returns(i);
        BOOST_CHECK_EQUAL(i, f());
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<int()> f;
        int i = 42;
        f.expect().returns(std::ref(i));
        i = 43;
        BOOST_CHECK_EQUAL(43, f());
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<int&()> f;
        f.expect().returns(42);
        BOOST_CHECK_EQUAL(42, f());
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<int&()> f;
        const int result = 42;
        f.expect().returns(result);
        BOOST_CHECK_EQUAL(result, f());
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<int&()> f;
        int i = 42;
        f.expect().returns(i);
        i = 43;
        BOOST_CHECK_EQUAL(42, f());
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<int&()> f;
        int i = 42;
        f.expect().returns(std::ref(i));
        i = 43;
        BOOST_CHECK_EQUAL(43, f());
        BOOST_CHECK_EQUAL(12, f() = 12);
        BOOST_CHECK_EQUAL(12, i);
        CHECK_CALLS(2);
    }
    {
        mock::detail::function<std::string()> f;
        f.expect().returns("result");
        BOOST_CHECK_EQUAL("result", f());
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<const std::string&()> f;
        f.expect().returns("result");
        BOOST_CHECK_EQUAL("result", f());
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<int*()> f;
        f.expect().returns(0);
        BOOST_CHECK(!f());
        CHECK_CALLS(1);
    }
#ifdef BOOST_MSVC // this produces a warning with gcc
    {
        mock::detail::function<int* const()> f;
        int i = 0;
        f.expect().returns(&i);
        BOOST_CHECK_EQUAL(&i, f());
        CHECK_CALLS(1);
    }
#endif
    {
        mock::detail::function<int()> f;
        f.expect().returns(0);
        BOOST_CHECK_EQUAL(0, f());
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<int&()> f;
        f.expect().returns(0);
        BOOST_CHECK_EQUAL(0, f());
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<unsigned int()> f;
        f.expect().returns(0);
        BOOST_CHECK_EQUAL(0u, f());
        CHECK_CALLS(1);
    }
}

namespace {
struct base
{
    virtual ~base() = default;
    virtual void f() = 0;
};
struct derived : base
{
    virtual void f() {}
};
} // namespace

BOOST_FIXTURE_TEST_CASE(triggering_an_expectation_moves_the_set_lvalue, mock_error_fixture)
{
    mock::detail::function<int()> f;
    int i = 3;
    f.expect().moves(i);
    BOOST_CHECK_NO_THROW(f());
    CHECK_CALLS(1);
}

BOOST_FIXTURE_TEST_CASE(triggering_an_expectation_moves_the_set_const_lvalue, mock_error_fixture)
{
    mock::detail::function<int()> f;
    const int i = 3;
    f.expect().moves(i);
    BOOST_CHECK_NO_THROW(f());
    CHECK_CALLS(1);
}

BOOST_FIXTURE_TEST_CASE(triggering_an_expectation_moves_the_set_rvalue, mock_error_fixture)
{
    mock::detail::function<int()> f;
    f.expect().moves(3);
    BOOST_CHECK_NO_THROW(f());
    CHECK_CALLS(1);
}

BOOST_FIXTURE_TEST_CASE(triggering_an_expectation_moves_the_set_unique_ptr_lvalue, mock_error_fixture)
{
    mock::detail::function<std::unique_ptr<int>()> f;
    std::unique_ptr<int> p(new int);
    f.expect().moves(std::move(p));
    BOOST_CHECK_NO_THROW(f());
    CHECK_CALLS(1);
}

BOOST_FIXTURE_TEST_CASE(triggering_an_expectation_moves_the_set_unique_ptr_rvalue, mock_error_fixture)
{
    {
        mock::detail::function<std::unique_ptr<int>()> f;
        f.expect().moves(std::unique_ptr<int>(new int));
        BOOST_CHECK_NO_THROW(f());
        CHECK_CALLS(1);
    }
}

BOOST_FIXTURE_TEST_CASE(triggering_an_expectation_returns_the_set_shared_ptr_value, mock_error_fixture)
{
    {
        mock::detail::function<std::shared_ptr<base>()> f;
        f.expect().returns(new derived);
        BOOST_CHECK_NO_THROW(f());
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<const std::shared_ptr<base>&()> f;
        f.expect().returns(new derived);
        BOOST_CHECK_NO_THROW(f());
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<std::shared_ptr<base>&()> f;
        f.expect().returns(new derived);
        BOOST_CHECK_NO_THROW(f());
        CHECK_CALLS(1);
    }
}

BOOST_FIXTURE_TEST_CASE(triggering_an_expectation_returns_by_reference, mock_error_fixture)
{
    {
        mock::detail::function<base&()> f;
        derived b;
        f.expect().returns(std::ref(b));
        BOOST_CHECK_NO_THROW(f());
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<base&()> f;
        derived b;
        f.expect().returns(b);
        BOOST_CHECK_NO_THROW(f());
        CHECK_CALLS(1);
    }
    {
        mock::detail::function<undefined&()> f;
        f.expect().returns(std::ref(get_undefined()));
        f.reset();
    }
}

namespace {
int custom_result()
{
    return 42;
}
} // namespace

BOOST_FIXTURE_TEST_CASE(triggering_an_expectation_calls_the_custom_functor, mock_error_fixture)
{
    mock::detail::function<int()> f;
    f.expect().calls(&custom_result);
    BOOST_CHECK_EQUAL(42, f());
    CHECK_CALLS(1);
}

namespace {
int custom_result_with_parameter(int i)
{
    return i;
}
} // namespace

BOOST_FIXTURE_TEST_CASE(triggering_an_expectation_calls_the_custom_functor_with_parameters, mock_error_fixture)
{
    mock::detail::function<int(int)> f;
    f.expect().calls(&custom_result_with_parameter);
    BOOST_CHECK_EQUAL(42, f(42));
    CHECK_CALLS(1);
}

BOOST_FIXTURE_TEST_CASE(triggering_an_expectation_calls_the_custom_functor_without_parameters_thanks_to_std_bind,
                        mock_error_fixture)
{
    mock::detail::function<int(int)> f;
    f.expect().calls(std::bind(&custom_result));
    BOOST_CHECK_EQUAL(42, f(17));
    CHECK_CALLS(1);
}

BOOST_FIXTURE_TEST_CASE(triggering_an_expectation_throws_the_set_exception, mock_error_fixture)
{
    mock::detail::function<int()> f;
    f.expect().throws(std::runtime_error("some exception"));
    try
    {
        f();
    } catch(std::runtime_error& f)
    {
        BOOST_CHECK_EQUAL("some exception", f.what());
        CHECK_CALLS(1);
        return;
    }
    BOOST_FAIL("should have thrown");
}

// multiple expectations

BOOST_FIXTURE_TEST_CASE(expecting_twice_a_single_expectation_makes_it_callable_twice, mock_error_fixture)
{
    {
        mock::detail::function<void()> f;
        f.expect().once();
        f.expect().once();
        f();
        f();
        CHECK_ERROR(f(), "unexpected call", 2, "?()\nv once()\nv once()");
    }
    {
        mock::detail::function<void(const std::string&)> f;
        f.expect().once().with("first");
        f.expect().once().with("second");
        f("first");
        f("second");
        CHECK_ERROR(
          f("third"), "unexpected call", 2, "?( \"third\" )\nv once().with( \"first\" )\nv once().with( \"second\" )");
    }
}

BOOST_FIXTURE_TEST_CASE(best_expectation_is_selected_first, mock_error_fixture)
{
    {
        mock::detail::function<void(int)> f;
        f.expect().once().with(1);
        f.expect().once().with(2);
        f(2);
        f(1);
        CHECK_ERROR(f(3), "unexpected call", 2, "?( 3 )\nv once().with( 1 )\nv once().with( 2 )");
    }
    {
        mock::detail::function<void(const std::string&)> f;
        f.expect().once().with("first");
        f.expect().once().with("second");
        f("second");
        f("first");
        CHECK_ERROR(
          f("third"), "unexpected call", 2, "?( \"third\" )\nv once().with( \"first\" )\nv once().with( \"second\" )");
    }
}

// error report

namespace {
template<typename T>
std::string to_string(const T& t)
{
    std::stringstream s;
    s << t;
    return s.str();
}
} // namespace

BOOST_FIXTURE_TEST_CASE(expectation_can_be_serialized_to_be_human_readable, mock_error_fixture)
{
    {
        mock::detail::function<void(int)> f;
        f.expect().once().with(1);
        f.expect().once().with(mock::close(3, 1));
        int target = 0;
        f.expect().once().with(mock::retrieve(target));
        f.expect().once().with(mock::same(target));
        BOOST_CHECK_NO_THROW(f(2));
        std::string expected = "?\n";                   // Not in a current call context
        expected += ". once().with( 1 )\n";             // Unmet/Unverified expectation with value
        expected += "v once().with( close( 3, 1 ) )\n"; // Verified expectation with constraint
        target = 42;
        // (Unverified) expectation with retrieve/same constraint print current value
        expected += ". once().with( retrieve( 42 ) )\n";
        expected += ". once().with( same( 42 ) )";
        BOOST_CHECK_EQUAL(expected, to_string(f));
        CHECK_CALLS(1);
        f.reset();
    }
    {
        mock::detail::function<void(int)> f;
        f.expect().never().with(1);
        const std::string expected = "?\n"
                                     "v never().with( 1 )";
        BOOST_CHECK_EQUAL(expected, to_string(f));
        f.reset();
    }
    {
        mock::detail::function<void(const std::string&)> f;
        f.expect().never().with(mock::less("first"));
        f.expect().exactly(2).with("second");
        BOOST_CHECK_NO_THROW(f("second"));
        {
            const std::string expected = "?\n"
                                         "v never().with( less( \"first\" ) )\n"
                                         ". exactly( 1/2 ).with( \"second\" )";
            BOOST_CHECK_EQUAL(expected, to_string(f));
        }
        BOOST_CHECK_NO_THROW(f("second"));
        {
            const std::string expected = "?\n"
                                         "v never().with( less( \"first\" ) )\n"
                                         "v exactly( 2/2 ).with( \"second\" )";
            BOOST_CHECK_EQUAL(expected, to_string(f));
            CHECK_CALLS(2);
        }
        f.reset();
    }
    {
        mock::detail::function<void(int)> f;
        f.expect().once();
        const std::string expected = "?\n"
                                     ". once().with( any )";
        BOOST_CHECK_EQUAL(expected, to_string(f));
        f.reset();
    }
    {
        mock::detail::function<void(int)> f;
        f.expect().once().with(mock::any);
        const std::string expected = "?\n"
                                     ". once().with( any )";
        BOOST_CHECK_EQUAL(expected, to_string(f));
        f.reset();
    }
    {
        mock::detail::function<void(int)> f;
        f.expect().once();
        const std::string expected = "?\n"
                                     ". once().with( any )";
        BOOST_CHECK_EQUAL(expected, to_string(f));
        f.reset();
    }
    {
        mock::detail::function<void(int)> f;
        f.expect().once().with(&custom_constraint);
        const std::string expected = "?\n"
                                     ". once().with( ? )";
        BOOST_CHECK_EQUAL(expected, to_string(f));
        f.reset();
    }
}

BOOST_FIXTURE_TEST_CASE(expectation_with_remaining_untriggered_matches_upon_destruction_calls_untriggered_expectation,
                        mock_error_fixture)
{
    auto f = std::make_unique<mock::detail::function<void()>>();
    f->expect().once();
    CHECK_ERROR(f.reset(), "untriggered expectation", 0, "?\n. once()");
}

BOOST_FIXTURE_TEST_CASE(
  verifying_expectation_with_remaining_matches_disables_the_automatic_verification_upon_destruction,
  mock_error_fixture)
{
    mock::detail::function<void()> f;
    f.expect().once();
    CHECK_ERROR(f.verify(), "verification failed", 0, "?\n. once()");
}

BOOST_FIXTURE_TEST_CASE(triggering_unexpected_call_call_disables_the_automatic_verification_upon_destruction,
                        mock_error_fixture)
{
    mock::detail::function<void()> f;
    CHECK_ERROR(f(), "unexpected call", 0, "?()");
}

BOOST_FIXTURE_TEST_CASE(adding_an_expectation_reactivates_the_verification_upon_destruction, mock_error_fixture)
{
    auto f = std::make_unique<mock::detail::function<void()>>();
    CHECK_ERROR((*f)(), "unexpected call", 0, "?()");
    f->expect().once();
    CHECK_ERROR(f.reset(), "untriggered expectation", 0, "?\n. once()");
}

BOOST_FIXTURE_TEST_CASE(throwing_an_exception_disables_the_automatic_verification_upon_destruction, mock_error_fixture)
{
    try
    {
        mock::detail::function<void()> f;
        f.expect().once();
        throw std::exception();
    } catch(std::exception&)
    {}
}

BOOST_FIXTURE_TEST_CASE(adding_file_and_line_number_information, mock_error_fixture)
{
    mock::detail::function<void()> f;
    f.expect("file name", 42).once();
    CHECK_ERROR(f.verify(), "verification failed", 0, "?\n. once()");
    BOOST_CHECK_EQUAL("file name", mock_error_data.last_file);
    BOOST_CHECK_EQUAL(42, mock_error_data.last_line);
}

#ifdef MOCK_THREAD_SAFE

#    include <boost/thread.hpp>

namespace {
void iterate(mock::detail::function<int()>& f)
{
    f.expect().once().returns(0);
    try
    {
        f();
    } catch(...)
    {}
}
} // namespace

BOOST_FIXTURE_TEST_CASE(function_is_thread_safe, mock_error_fixture)
{
    mock::detail::function<int()> f;
    boost::thread_group group;
    for(int i = 0; i < 100; ++i)
        group.create_thread([&f]() { iterate(f); });
    group.join_all();
    CHECK_CALLS(100);
}

#endif // MOCK_THREAD_SAFE
