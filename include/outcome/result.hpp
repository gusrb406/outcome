#ifndef OUTCOME_RESULT_HPP
#define OUTCOME_RESULT_HPP

#include "error_code_extended.hpp"

#include <type_traits>
#include <utility>  // for in_place_type_t

namespace outcome
{
#if __cplusplus >= 201700
  template <class T> using in_place_type_t = std::in_place_type_t<T>;
  using std::in_place_type;
#else
  template <class T> struct in_place_type_t
  {
    explicit in_place_type_t() = default;
  };
  template <class T> inline constexpr std::in_place_type_t<T> in_place_type{};
#endif
  namespace detail
  {
    template <class T> struct is_in_place_type_t : std::false_type
    {
    };
    template <class U> struct is_in_place_type_t<in_place_type_t<U>> : std::true_type
    {
    };
  }

  namespace policy
  {
#ifdef __cpp_exceptions
    /* \struct error_code_throw_as_system_error
    \brief Policy interpreting EC as a type implementing the `std::error_code` contract
    and any wide attempt to access the successful state throws the `error_code` wrapped into
    a `std::system_error`
    */
    template <class EC> struct error_code_throw_as_system_error
    {
      static_assert(std::is_convertible<std::error_code, EC>::value, "error_type must be convertible into a std::error_code to be used with this ECPolicy");
      //! Returns true if errored
      template <class T, class _EC> static constexpr bool is_errored(T && /*unused*/, _EC &&_error) noexcept { return !_error; }
      //! Performs a narrow check of state, used in the assume_value() functions
      template <class T, class _EC> static constexpr void narrow_value_check(T && /*unused*/, _EC &&_error) noexcept
      {
#if defined(__GNUC__) || defined(__clang__)
        if(_error)
          __builtin_unreachable();
#endif
      }
      //! Performs a narrow check of state, used in the assume_error() functions
      template <class T, class _EC> static constexpr void narrow_error_check(T && /*unused*/, _EC && /*unused*/) noexcept {}
      //! Performs a wide check of state, used in the value() functions
      template <class T, class _EC> static constexpr void wide_value_check(T && /*unused*/, _EC &&_error)
      {
        if(_error)
        {
          throw std::system_error(_error);
        }
      }
      //! Performs a wide check of state, used in the error() functions
      template <class T, class _EC> static constexpr void wide_error_check(T && /*unused*/, _EC && /*unused*/) noexcept {}
    };
#endif
    /* \struct error_code_with_terminate
    \brief Policy interpreting EC as a type implementing the `std::error_code` contract
    and any wide attempt to access the successful state calls `std::terminate`
    */
    template <class EC> struct error_code_with_terminate
    {
      static_assert(std::is_convertible<std::error_code, EC>::value, "error_type must be convertible into a std::error_code to be used with this ECPolicy");
      //! Returns true if errored
      template <class T, class _EC> static constexpr bool is_errored(T && /*unused*/, _EC &&_error) noexcept { return !_error; }
      //! Performs a narrow check of state, used in the assume_value() functions
      template <class T, class _EC> static constexpr void narrow_value_check(T && /*unused*/, _EC &&_error) noexcept
      {
#if defined(__GNUC__) || defined(__clang__)
        if(_error)
          __builtin_unreachable();
#endif
      }
      //! Performs a narrow check of state, used in the assume_error() functions
      template <class T, class _EC> static constexpr void narrow_error_check(T && /*unused*/, _EC && /*unused*/) noexcept {}
      //! Performs a wide check of state, used in the value() functions
      template <class T, class _EC> static constexpr void wide_value_check(T && /*unused*/, _EC &&_error)
      {
        if(_error)
        {
          std::terminate();
        }
      }
      //! Performs a wide check of state, used in the error() functions
      template <class T, class _EC> static constexpr void wide_error_check(T && /*unused*/, _EC && /*unused*/) noexcept {}
    };
  }

  /* \class result
  \brief Provides the result of a success and/or a failure
  \tparam T The type of the successful result. Needs to be DefaultConstructible.
  \tparam EC The type of the failure result. Needs to be DefaultConstructible and be boolean testable e.g. `if(ec)`
  \tparam ECPolicy Policy on how to interpret type EC.
  */
  template <class T, class EC = error_code_extended, class ECPolicy =
#ifdef __cpp_exceptions
                                                     policy::error_code_throw_as_system_error<EC>
#else
                                                     policy::error_code_with_terminate<EC>
#endif
            >
  class result
  {
    static_assert(std::is_default_constructible<T>::value, "value_type must be default constructible");
    static_assert(std::is_default_constructible<EC>::value, "error_type must be default constructible");
    static_assert(std::is_constructible<bool, EC>::value, "error_type must implement boolean testability");

  public:
    //! The result type
    using value_type = T;
    //! The failure type
    using error_type = EC;

  private:
    value_type _value;
    error_type _error;

  public:
    //! Default constructor
    result() = default;
    //! Move constructor
    result(result && /*unused*/) = default;
    //! Copy constructor
    result(const result & /*unused*/) = default;
    //! Move assignment
    result &operator=(result && /*unused*/) = default;
    //! Copy assignment
    result &operator=(const result & /*unused*/) = default;

    //! Converting constructor to value_type
    template <class T, typename = typename std::enable_if<                                  //
                       !std::is_same<typename std::decay_t<T>::type, result>::value         // not my type
                       && !detail::is_in_place_type_t<typename std::decay<T>::type>::value  // not in place construction
                       && std::is_constructible<value_type, T>::value && !std::is_constructible<error_type, T>::value>::type>
    constexpr result(T &&t) noexcept(noexcept(value_type(std::forward<T>(t))))
        : _value(std::forward<T>(t))
    {
    }
    //! Converting constructor to error_type
    template <class T, typename = typename std::enable_if<                                  //
                       !std::is_same<typename std::decay_t<T>::type, result>::value         // not my type
                       && !detail::is_in_place_type_t<typename std::decay<T>::type>::value  // not in place construction
                       && !std::is_constructible<value_type, T>::value && std::is_constructible<error_type, T>::value>::type>
    constexpr result(T &&t) noexcept(noexcept(error_type(std::forward<T>(t))))
        : _error(std::forward<T>(t))
    {
    }
    //! In place constructor to value_type
    template <class... Args, typename = typename std::enable_if<std::is_constructible<value_type, Args...>::value>::type>
    constexpr explicit result(in_place_type_t<value_type>, Args &&... args) noexcept(noexcept(value_type(std::forward<Args>(args)...)))
        : _value(std::forward<Args>(args)...)
    {
    }
    //! In place constructor to value_type
    template <class U, class... Args, typename = typename std::enable_if<std::is_constructible<value_type, std::initializer_list<U>>::value>::type>
    constexpr explicit result(in_place_type_t<value_type>, std::initializer_list<U> il, Args &&... args) noexcept(noexcept(value_type(il, std::forward<Args>(args)...)))
        : _value(il, std::forward<Args>(args)...)
    {
    }
    //! In place constructor to error_type
    template <class... Args, typename = typename std::enable_if<std::is_constructible<error_type, Args...>::value>::type>
    constexpr explicit result(in_place_type_t<error_type>, Args &&... args) noexcept(noexcept(error_type(std::forward<Args>(args)...)))
        : _error(std::forward<Args>(args)...)
    {
    }
    //! In place constructor to error_type
    template <class U, class... Args, typename = typename std::enable_if<std::is_constructible<error_type, std::initializer_list<U>>::value>::type>
    constexpr explicit result(in_place_type_t<error_type>, std::initializer_list<U> il, Args &&... args) noexcept(noexcept(error_type(il, std::forward<Args>(args)...)))
        : _value(il, std::forward<Args>(args)...)
    {
    }

    //! True if has value
    constexpr explicit operator bool() const noexcept { return !ECPolicy::is_errored(_value, _error); }
    //! True if has value
    constexpr bool has_value() const noexcept { return !ECPolicy::is_errored(_value, _error); }
    //! True if has error
    constexpr bool has_error() const noexcept { return ECPolicy::is_errored(_value, _error); }

    //! Access value directly
    value_type &assume_value() & noexcept
    {
      ECPolicy::narrow_value_check(_value, _error);
      return _value;
    }
    const value_type &assume_value() const &noexcept
    {
      ECPolicy::narrow_value_check(_value, _error);
      return _value;
    }
    value_type &&assume_value() && noexcept
    {
      ECPolicy::narrow_value_check(_value, _error);
      return _value;
    }
    const value_type &&assume_value() const &&noexcept
    {
      ECPolicy::narrow_value_check(_value, _error);
      return _value;
    }
    //! Access error directly
    error_type &assume_error() & noexcept
    {
      ECPolicy::narrow_error_check(_value, _error);
      return _error;
    }
    const error_type &assume_error() const &noexcept
    {
      ECPolicy::narrow_error_check(_value, _error);
      return _error;
    }
    error_type &&assume_error() && noexcept
    {
      ECPolicy::narrow_error_check(_value, _error);
      return _error;
    }
    const error_type &&assume_error() const &&noexcept
    {
      ECPolicy::narrow_error_check(_value, _error);
      return _error;
    }

    //! Access value if no error
    value_type &value() &
    {
      ECPolicy::wide_value_check(_value, _error);
      return _value;
    }
    const value_type &value() const &
    {
      ECPolicy::wide_value_check(_value, _error);
      return _value;
    }
    value_type &&value() &&
    {
      ECPolicy::wide_value_check(_value, _error);
      return _value;
    }
    const value_type &&value() const &&
    {
      ECPolicy::wide_value_check(_value, _error);
      return _value;
    }
    //! Access error
    error_type &error() & noexcept
    {
      ECPolicy::wide_error_check(_value, _error);
      return _error;
    }
    const error_type &error() const &noexcept
    {
      ECPolicy::wide_error_check(_value, _error);
      return _error;
    }
    error_type &&error() && noexcept
    {
      ECPolicy::wide_error_check(_value, _error);
      return _error;
    }
    const error_type &&error() const &&noexcept
    {
      ECPolicy::wide_error_check(_value, _error);
      return _error;
    }
  };
}
#endif
