/* A less simple result type
(C) 2017-2019 Niall Douglas <http://www.nedproductions.biz/> (20 commits)
File Created: June 2017


Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License in the accompanying file
Licence.txt or at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Distributed under the Boost Software License, Version 1.0.
    (See accompanying file Licence.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef OUTCOME_BASIC_OUTCOME_HPP
#define OUTCOME_BASIC_OUTCOME_HPP

#include "config.hpp"

#include "basic_result.hpp"
#include "detail/basic_outcome_exception_observers.hpp"
#include "detail/basic_outcome_failure_observers.hpp"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"  // Standardese markup confuses clang
#endif

OUTCOME_V2_NAMESPACE_EXPORT_BEGIN

template <class R, class S, class P, class NoValuePolicy>                                                                            //
OUTCOME_REQUIRES(trait::type_can_be_used_in_basic_result<P> && (std::is_void<P>::value || std::is_default_constructible<P>::value))  //
class basic_outcome;

namespace detail
{
  // May be reused by basic_outcome subclasses to save load on the compiler
  template <class value_type, class error_type, class exception_type> struct outcome_predicates
  {
    using result = result_predicates<value_type, error_type>;

    // Predicate for the implicit constructors to be available
    static constexpr bool implicit_constructors_enabled =                //
    result::implicit_constructors_enabled                                //
    && !detail::is_implicitly_constructible<value_type, exception_type>  //
    && !detail::is_implicitly_constructible<error_type, exception_type>  //
    && !detail::is_implicitly_constructible<exception_type, value_type>  //
    && !detail::is_implicitly_constructible<exception_type, error_type>;

    // Predicate for the value converting constructor to be available.
    template <class T>
    static constexpr bool enable_value_converting_constructor =  //
    implicit_constructors_enabled                                //
    &&result::template enable_value_converting_constructor<T>    //
    && !detail::is_implicitly_constructible<exception_type, T>;  // deliberately less tolerant of ambiguity than result's edition

    // Predicate for the error converting constructor to be available.
    template <class T>
    static constexpr bool enable_error_converting_constructor =  //
    implicit_constructors_enabled                                //
    &&result::template enable_error_converting_constructor<T>    //
    && !detail::is_implicitly_constructible<exception_type, T>;  // deliberately less tolerant of ambiguity than result's edition

    // Predicate for the error condition converting constructor to be available.
    template <class ErrorCondEnum>
    static constexpr bool enable_error_condition_converting_constructor = result::template enable_error_condition_converting_constructor<ErrorCondEnum>  //
                                                                          && !detail::is_implicitly_constructible<exception_type, ErrorCondEnum>;

    // Predicate for the exception converting constructor to be available.
    template <class T>
    static constexpr bool enable_exception_converting_constructor =  //
    implicit_constructors_enabled                                    //
    && !is_in_place_type_t<std::decay_t<T>>::value                   // not in place construction
    && !detail::is_implicitly_constructible<value_type, T> && !detail::is_implicitly_constructible<error_type, T> && detail::is_implicitly_constructible<exception_type, T>;

    // Predicate for the error + exception converting constructor to be available.
    template <class T, class U>
    static constexpr bool enable_error_exception_converting_constructor =                                         //
    implicit_constructors_enabled                                                                                 //
    && !is_in_place_type_t<std::decay_t<T>>::value                                                                // not in place construction
    && !detail::is_implicitly_constructible<value_type, T> && detail::is_implicitly_constructible<error_type, T>  //
    && !detail::is_implicitly_constructible<value_type, U> && detail::is_implicitly_constructible<exception_type, U>;

    // Predicate for the converting copy constructor from a compatible outcome to be available.
    template <class T, class U, class V, class W>
    static constexpr bool enable_compatible_conversion =                                                                                   //
    (std::is_void<T>::value || detail::is_explicitly_constructible<value_type, typename basic_outcome<T, U, V, W>::value_type>)            // if our value types are constructible
    &&(std::is_void<U>::value || detail::is_explicitly_constructible<error_type, typename basic_outcome<T, U, V, W>::error_type>)          // if our error types are constructible
    &&(std::is_void<V>::value || detail::is_explicitly_constructible<exception_type, typename basic_outcome<T, U, V, W>::exception_type>)  // if our exception types are constructible
    ;

    // Predicate for the implicit converting inplace constructor from a compatible input to be available.
    struct disable_inplace_value_error_exception_constructor;
    template <class... Args>
    using choose_inplace_value_error_exception_constructor = std::conditional_t<                                                                                                                                                  //
    ((static_cast<int>(std::is_constructible<value_type, Args...>::value) + static_cast<int>(std::is_constructible<error_type, Args...>::value) + static_cast<int>(std::is_constructible<exception_type, Args...>::value)) > 1),  //
    disable_inplace_value_error_exception_constructor,                                                                                                                                                                            //
    std::conditional_t<                                                                                                                                                                                                           //
    std::is_constructible<value_type, Args...>::value,                                                                                                                                                                            //
    value_type,                                                                                                                                                                                                                   //
    std::conditional_t<                                                                                                                                                                                                           //
    std::is_constructible<error_type, Args...>::value,                                                                                                                                                                            //
    error_type,                                                                                                                                                                                                                   //
    std::conditional_t<                                                                                                                                                                                                           //
    std::is_constructible<exception_type, Args...>::value,                                                                                                                                                                        //
    exception_type,                                                                                                                                                                                                               //
    disable_inplace_value_error_exception_constructor>>>>;
    template <class... Args>
    static constexpr bool enable_inplace_value_error_exception_constructor =  //
    implicit_constructors_enabled && !std::is_same<choose_inplace_value_error_exception_constructor<Args...>, disable_inplace_value_error_exception_constructor>::value;
  };

  // Select whether to use basic_outcome_failure_observers or not
  template <class Base, class R, class S, class P, class NoValuePolicy>
  using select_basic_outcome_failure_observers =  //
  std::conditional_t<trait::is_error_code_available<S>::value && trait::is_exception_ptr_available<P>::value, basic_outcome_failure_observers<Base, R, S, P, NoValuePolicy>, Base>;

  template <class T, class U, class V> constexpr inline const V &extract_exception_from_failure(const failure_type<U, V> &v) { return v.exception(); }
  template <class T, class U, class V> constexpr inline V &&extract_exception_from_failure(failure_type<U, V> &&v) { return static_cast<failure_type<U, V> &&>(v).exception(); }
  template <class T, class U> constexpr inline const U &extract_exception_from_failure(const failure_type<U, void> &v) { return v.error(); }
  template <class T, class U> constexpr inline U &&extract_exception_from_failure(failure_type<U, void> &&v) { return static_cast<failure_type<U, void> &&>(v).error(); }

  template <class T> struct is_basic_outcome
  {
    static constexpr bool value = false;
  };
  template <class R, class S, class T, class N> struct is_basic_outcome<basic_outcome<R, S, T, N>>
  {
    static constexpr bool value = true;
  };
}  // namespace detail

/*! AWAITING HUGO JSON CONVERSION TOOL
type alias template <class T> is_basic_outcome. Potential doc page: `is_basic_outcome<T>`
*/
template <class T> using is_basic_outcome = detail::is_basic_outcome<std::decay_t<T>>;
/*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
template <class T> static constexpr bool is_basic_outcome_v = detail::is_basic_outcome<std::decay_t<T>>::value;

namespace hooks
{
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  template <class T, class... U> constexpr inline void hook_outcome_construction(T * /*unused*/, U &&... /*unused*/) noexcept {}
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  template <class T, class U> constexpr inline void hook_outcome_copy_construction(T * /*unused*/, U && /*unused*/) noexcept {}
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  template <class T, class U> constexpr inline void hook_outcome_move_construction(T * /*unused*/, U && /*unused*/) noexcept {}
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  template <class T, class U, class... Args> constexpr inline void hook_outcome_in_place_construction(T * /*unused*/, in_place_type_t<U> /*unused*/, Args &&... /*unused*/) noexcept {}

  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  template <class R, class S, class P, class NoValuePolicy, class U> constexpr inline void override_outcome_exception(basic_outcome<R, S, P, NoValuePolicy> *o, U &&v) noexcept;
}  // namespace hooks

/*! AWAITING HUGO JSON CONVERSION TOOL
type definition template <class R, class S, class P, class NoValuePolicy> basic_outcome. Potential doc page: `basic_outcome<T, EC, EP, NoValuePolicy>`
*/
template <class R, class S, class P, class NoValuePolicy>                                                                            //
OUTCOME_REQUIRES(trait::type_can_be_used_in_basic_result<P> && (std::is_void<P>::value || std::is_default_constructible<P>::value))  //
class OUTCOME_NODISCARD basic_outcome
#if defined(DOXYGEN_IS_IN_THE_HOUSE) || defined(STANDARDESE_IS_IN_THE_HOUSE)
    : public detail::basic_outcome_failure_observers<detail::basic_result_final<R, S, P, NoValuePolicy>, R, S, P, NoValuePolicy>,
      public detail::basic_outcome_exception_observers<detail::basic_result_final<R, S, NoValuePolicy>, R, S, P, NoValuePolicy>,
      public detail::basic_result_final<R, S, NoValuePolicy>
#else
    : public detail::select_basic_outcome_failure_observers<detail::basic_outcome_exception_observers<detail::basic_result_final<R, S, NoValuePolicy>, R, S, P, NoValuePolicy>, R, S, P, NoValuePolicy>
#endif
{
  static_assert(trait::type_can_be_used_in_basic_result<P>, "The exception_type cannot be used");
  static_assert(std::is_void<P>::value || std::is_default_constructible<P>::value, "exception_type must be void or default constructible");
  using base = detail::select_basic_outcome_failure_observers<detail::basic_outcome_exception_observers<detail::basic_result_final<R, S, NoValuePolicy>, R, S, P, NoValuePolicy>, R, S, P, NoValuePolicy>;
  friend struct policy::base;
  template <class T, class U, class V, class W> //
  OUTCOME_REQUIRES(trait::type_can_be_used_in_basic_result<V> && (std::is_void<V>::value || std::is_default_constructible<V>::value))  //
  friend class basic_outcome;
  template <class T, class U, class V, class W, class X> friend constexpr inline void hooks::override_outcome_exception(basic_outcome<T, U, V, W> *o, X &&v) noexcept;  // NOLINT

  struct implicit_constructors_disabled_tag
  {
  };
  struct value_converting_constructor_tag
  {
  };
  struct error_converting_constructor_tag
  {
  };
  struct error_condition_converting_constructor_tag
  {
  };
  struct exception_converting_constructor_tag
  {
  };
  struct error_exception_converting_constructor_tag
  {
  };
  struct explicit_valueorerror_converting_constructor_tag
  {
  };
  struct error_failure_tag
  {
  };
  struct exception_failure_tag
  {
  };

  struct disable_in_place_value_type
  {
  };
  struct disable_in_place_error_type
  {
  };
  struct disable_in_place_exception_type
  {
  };

public:
  using value_type = R;
  using error_type = S;
  using exception_type = P;

  template <class T, class U = S, class V = P, class W = NoValuePolicy> using rebind = basic_outcome<T, U, V, W>;

protected:
  // Requirement predicates for outcome.
  struct predicate
  {
    using base = detail::outcome_predicates<value_type, error_type, exception_type>;

    // Predicate for any constructors to be available at all
    static constexpr bool constructors_enabled = (!std::is_same<std::decay_t<value_type>, std::decay_t<error_type>>::value || (std::is_void<value_type>::value && std::is_void<error_type>::value))             //
                                                 && (!std::is_same<std::decay_t<value_type>, std::decay_t<exception_type>>::value || (std::is_void<value_type>::value && std::is_void<exception_type>::value))  //
                                                 && (!std::is_same<std::decay_t<error_type>, std::decay_t<exception_type>>::value || (std::is_void<error_type>::value && std::is_void<exception_type>::value))  //
    ;

    // Predicate for implicit constructors to be available at all
    static constexpr bool implicit_constructors_enabled = constructors_enabled && base::implicit_constructors_enabled;

    // Predicate for the value converting constructor to be available.
    template <class T>
    static constexpr bool enable_value_converting_constructor =  //
    constructors_enabled                                         //
    && !std::is_same<std::decay_t<T>, basic_outcome>::value      // not my type
    && base::template enable_value_converting_constructor<T>;

    // Predicate for the error converting constructor to be available.
    template <class T>
    static constexpr bool enable_error_converting_constructor =  //
    constructors_enabled                                         //
    && !std::is_same<std::decay_t<T>, basic_outcome>::value      // not my type
    && base::template enable_error_converting_constructor<T>;

    // Predicate for the error condition converting constructor to be available.
    template <class ErrorCondEnum>
    static constexpr bool enable_error_condition_converting_constructor =  //
    constructors_enabled                                                   //
    && !std::is_same<std::decay_t<ErrorCondEnum>, basic_outcome>::value    // not my type
    && base::template enable_error_condition_converting_constructor<ErrorCondEnum>;

    // Predicate for the exception converting constructor to be available.
    template <class T>
    static constexpr bool enable_exception_converting_constructor =  //
    constructors_enabled                                             //
    && !std::is_same<std::decay_t<T>, basic_outcome>::value          // not my type
    && base::template enable_exception_converting_constructor<T>;

    // Predicate for the error + exception converting constructor to be available.
    template <class T, class U>
    static constexpr bool enable_error_exception_converting_constructor =  //
    constructors_enabled                                                   //
    && !std::is_same<std::decay_t<T>, basic_outcome>::value                // not my type
    && base::template enable_error_exception_converting_constructor<T, U>;

    // Predicate for the converting constructor from a compatible input to be available.
    template <class T, class U, class V, class W>
    static constexpr bool enable_compatible_conversion =               //
    constructors_enabled                                               //
    && !std::is_same<basic_outcome<T, U, V, W>, basic_outcome>::value  // not my type
    && base::template enable_compatible_conversion<T, U, V, W>;

    // Predicate for the inplace construction of value to be available.
    template <class... Args>
    static constexpr bool enable_inplace_value_constructor =  //
    constructors_enabled                                      //
    && (std::is_void<value_type>::value                       //
        || std::is_constructible<value_type, Args...>::value);

    // Predicate for the inplace construction of error to be available.
    template <class... Args>
    static constexpr bool enable_inplace_error_constructor =  //
    constructors_enabled                                      //
    && (std::is_void<error_type>::value                       //
        || std::is_constructible<error_type, Args...>::value);

    // Predicate for the inplace construction of exception to be available.
    template <class... Args>
    static constexpr bool enable_inplace_exception_constructor =  //
    constructors_enabled                                          //
    && (std::is_void<exception_type>::value                       //
        || std::is_constructible<exception_type, Args...>::value);

    // Predicate for the implicit converting inplace constructor to be available.
    template <class... Args>
    static constexpr bool enable_inplace_value_error_exception_constructor =  //
    constructors_enabled                                                      //
    &&base::template enable_inplace_value_error_exception_constructor<Args...>;
    template <class... Args> using choose_inplace_value_error_exception_constructor = typename base::template choose_inplace_value_error_exception_constructor<Args...>;
  };

public:
  using value_type_if_enabled = std::conditional_t<std::is_same<value_type, error_type>::value || std::is_same<value_type, exception_type>::value, disable_in_place_value_type, value_type>;
  using error_type_if_enabled = std::conditional_t<std::is_same<error_type, value_type>::value || std::is_same<error_type, exception_type>::value, disable_in_place_error_type, error_type>;
  using exception_type_if_enabled = std::conditional_t<std::is_same<exception_type, value_type>::value || std::is_same<exception_type, error_type>::value, disable_in_place_exception_type, exception_type>;

protected:
  detail::devoid<exception_type> _ptr;

public:
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class Arg, class... Args)
  OUTCOME_TREQUIRES(OUTCOME_TPRED((!predicate::constructors_enabled && sizeof...(Args) >= 0)))
  basic_outcome(Arg && /*unused*/, Args &&... /*unused*/) = delete;  // NOLINT basic_outcome<> with any of the same type is NOT SUPPORTED, see docs!

  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED((predicate::constructors_enabled && !predicate::implicit_constructors_enabled  //
                                   && (detail::is_implicitly_constructible<value_type, T> || detail::is_implicitly_constructible<error_type, T> || detail::is_implicitly_constructible<exception_type, T>) )))
  basic_outcome(T && /*unused*/, implicit_constructors_disabled_tag /*unused*/ = implicit_constructors_disabled_tag()) = delete;  // NOLINT Implicit constructors disabled, use explicit in_place_type<T>, success() or failure(). see docs!

  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_value_converting_constructor<T>))
  constexpr basic_outcome(T &&t, value_converting_constructor_tag /*unused*/ = value_converting_constructor_tag()) noexcept(std::is_nothrow_constructible<value_type, T>::value)  // NOLINT
      : base{in_place_type<typename base::_value_type>, static_cast<T &&>(t)}
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_construction(this, static_cast<T &&>(t));
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_error_converting_constructor<T>))
  constexpr basic_outcome(T &&t, error_converting_constructor_tag /*unused*/ = error_converting_constructor_tag()) noexcept(std::is_nothrow_constructible<error_type, T>::value)  // NOLINT
      : base{in_place_type<typename base::_error_type>, static_cast<T &&>(t)}
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_construction(this, static_cast<T &&>(t));
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class ErrorCondEnum)
  OUTCOME_TREQUIRES(OUTCOME_TEXPR(error_type(make_error_code(ErrorCondEnum()))),  //
                    OUTCOME_TPRED(predicate::template enable_error_condition_converting_constructor<ErrorCondEnum>))
  constexpr basic_outcome(ErrorCondEnum &&t, error_condition_converting_constructor_tag /*unused*/ = error_condition_converting_constructor_tag()) noexcept(noexcept(error_type(make_error_code(static_cast<ErrorCondEnum &&>(t)))))  // NOLINT
      : base{in_place_type<typename base::_error_type>, make_error_code(t)}
  {
    using namespace hooks;
    hook_outcome_construction(this, static_cast<ErrorCondEnum &&>(t));
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_exception_converting_constructor<T>))
  constexpr basic_outcome(T &&t, exception_converting_constructor_tag /*unused*/ = exception_converting_constructor_tag()) noexcept(std::is_nothrow_constructible<exception_type, T>::value)  // NOLINT
      : base()
      , _ptr(static_cast<T &&>(t))
  {
    using namespace hooks;
    this->_state._status |= detail::status_have_exception;
    hook_outcome_construction(this, static_cast<T &&>(t));
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T, class U)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_error_exception_converting_constructor<T, U>))
  constexpr basic_outcome(T &&a, U &&b, error_exception_converting_constructor_tag /*unused*/ = error_exception_converting_constructor_tag()) noexcept(std::is_nothrow_constructible<error_type, T>::value &&std::is_nothrow_constructible<exception_type, U>::value)  // NOLINT
      : base{in_place_type<typename base::_error_type>, static_cast<T &&>(a)}
      , _ptr(static_cast<U &&>(b))
  {
    using namespace hooks;
    this->_state._status |= detail::status_have_exception;
    hook_outcome_construction(this, static_cast<T &&>(a), static_cast<U &&>(b));
  }

  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(convert::value_or_error<basic_outcome, std::decay_t<T>>::enable_result_inputs || !is_basic_result_v<T>),    //
                    OUTCOME_TPRED(convert::value_or_error<basic_outcome, std::decay_t<T>>::enable_outcome_inputs || !is_basic_outcome_v<T>),  //
                    OUTCOME_TEXPR(convert::value_or_error<basic_outcome, std::decay_t<T>>{}(std::declval<T>())))
  constexpr explicit basic_outcome(T &&o, explicit_valueorerror_converting_constructor_tag /*unused*/ = explicit_valueorerror_converting_constructor_tag())  // NOLINT
      : basic_outcome{convert::value_or_error<basic_outcome, std::decay_t<T>>{}(static_cast<T &&>(o))}
  {
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T, class U, class V, class W)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_compatible_conversion<T, U, V, W>))
  constexpr explicit basic_outcome(const basic_outcome<T, U, V, W> &o) noexcept(std::is_nothrow_constructible<value_type, T>::value &&std::is_nothrow_constructible<error_type, U>::value &&std::is_nothrow_constructible<exception_type, V>::value)
      : base{typename base::compatible_conversion_tag(), o}
      , _ptr(o._ptr)
  {
    using namespace hooks;
    hook_outcome_copy_construction(this, o);
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T, class U, class V, class W)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_compatible_conversion<T, U, V, W>))
  constexpr explicit basic_outcome(basic_outcome<T, U, V, W> &&o) noexcept(std::is_nothrow_constructible<value_type, T>::value &&std::is_nothrow_constructible<error_type, U>::value &&std::is_nothrow_constructible<exception_type, V>::value)
      : base{typename base::compatible_conversion_tag(), static_cast<basic_outcome<T, U, V, W> &&>(o)}
      , _ptr(static_cast<typename basic_outcome<T, U, V, W>::exception_type &&>(o._ptr))
  {
    using namespace hooks;
    hook_outcome_move_construction(this, static_cast<basic_outcome<T, U, V, W> &&>(o));
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T, class U, class V)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(detail::result_predicates<value_type, error_type>::template enable_compatible_conversion<T, U, V>))
  constexpr explicit basic_outcome(const basic_result<T, U, V> &o) noexcept(std::is_nothrow_constructible<value_type, T>::value &&std::is_nothrow_constructible<error_type, U>::value &&std::is_nothrow_constructible<exception_type>::value)
      : base{typename base::compatible_conversion_tag(), o}
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_copy_construction(this, o);
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T, class U, class V)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(detail::result_predicates<value_type, error_type>::template enable_compatible_conversion<T, U, V>))
  constexpr explicit basic_outcome(basic_result<T, U, V> &&o) noexcept(std::is_nothrow_constructible<value_type, T>::value &&std::is_nothrow_constructible<error_type, U>::value &&std::is_nothrow_constructible<exception_type>::value)
      : base{typename base::compatible_conversion_tag(), static_cast<basic_result<T, U, V> &&>(o)}
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_move_construction(this, static_cast<basic_result<T, U, V> &&>(o));
  }


  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class... Args)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_inplace_value_constructor<Args...>))
  constexpr explicit basic_outcome(in_place_type_t<value_type_if_enabled> _, Args &&... args) noexcept(std::is_nothrow_constructible<value_type, Args...>::value)
      : base{_, static_cast<Args &&>(args)...}
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_in_place_construction(this, in_place_type<value_type>, static_cast<Args &&>(args)...);
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class U, class... Args)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_inplace_value_constructor<std::initializer_list<U>, Args...>))
  constexpr explicit basic_outcome(in_place_type_t<value_type_if_enabled> _, std::initializer_list<U> il, Args &&... args) noexcept(std::is_nothrow_constructible<value_type, std::initializer_list<U>, Args...>::value)
      : base{_, il, static_cast<Args &&>(args)...}
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_in_place_construction(this, in_place_type<value_type>, il, static_cast<Args &&>(args)...);
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class... Args)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_inplace_error_constructor<Args...>))
  constexpr explicit basic_outcome(in_place_type_t<error_type_if_enabled> _, Args &&... args) noexcept(std::is_nothrow_constructible<error_type, Args...>::value)
      : base{_, static_cast<Args &&>(args)...}
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_in_place_construction(this, in_place_type<error_type>, static_cast<Args &&>(args)...);
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class U, class... Args)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_inplace_error_constructor<std::initializer_list<U>, Args...>))
  constexpr explicit basic_outcome(in_place_type_t<error_type_if_enabled> _, std::initializer_list<U> il, Args &&... args) noexcept(std::is_nothrow_constructible<error_type, std::initializer_list<U>, Args...>::value)
      : base{_, il, static_cast<Args &&>(args)...}
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_in_place_construction(this, in_place_type<error_type>, il, static_cast<Args &&>(args)...);
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class... Args)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_inplace_exception_constructor<Args...>))
  constexpr explicit basic_outcome(in_place_type_t<exception_type_if_enabled> /*unused*/, Args &&... args) noexcept(std::is_nothrow_constructible<exception_type, Args...>::value)
      : base()
      , _ptr(static_cast<Args &&>(args)...)
  {
    using namespace hooks;
    this->_state._status |= detail::status_have_exception;
    hook_outcome_in_place_construction(this, in_place_type<exception_type>, static_cast<Args &&>(args)...);
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class U, class... Args)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_inplace_exception_constructor<std::initializer_list<U>, Args...>))
  constexpr explicit basic_outcome(in_place_type_t<exception_type_if_enabled> /*unused*/, std::initializer_list<U> il, Args &&... args) noexcept(std::is_nothrow_constructible<exception_type, std::initializer_list<U>, Args...>::value)
      : base()
      , _ptr(il, static_cast<Args &&>(args)...)
  {
    using namespace hooks;
    this->_state._status |= detail::status_have_exception;
    hook_outcome_in_place_construction(this, in_place_type<exception_type>, il, static_cast<Args &&>(args)...);
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class A1, class A2, class... Args)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_inplace_value_error_exception_constructor<A1, A2, Args...>))
  constexpr basic_outcome(A1 &&a1, A2 &&a2, Args &&... args) noexcept(noexcept(typename predicate::template choose_inplace_value_error_exception_constructor<A1, A2, Args...>(std::declval<A1>(), std::declval<A2>(), std::declval<Args>()...)))
      : basic_outcome(in_place_type<typename predicate::template choose_inplace_value_error_exception_constructor<A1, A2, Args...>>, static_cast<A1 &&>(a1), static_cast<A2 &&>(a2), static_cast<Args &&>(args)...)
  {
  }

  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  constexpr basic_outcome(const success_type<void> &o) noexcept(std::is_nothrow_default_constructible<value_type>::value)  // NOLINT
      : base{in_place_type<typename base::_value_type>}
  {
    using namespace hooks;
    hook_outcome_copy_construction(this, o);
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(!std::is_void<T>::value && predicate::template enable_compatible_conversion<T, void, void, void>))
  constexpr basic_outcome(const success_type<T> &o) noexcept(std::is_nothrow_constructible<value_type, T>::value)  // NOLINT
      : base{in_place_type<typename base::_value_type>, detail::extract_value_from_success<value_type>(o)}
  {
    using namespace hooks;
    hook_outcome_copy_construction(this, o);
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(!std::is_void<T>::value && predicate::template enable_compatible_conversion<T, void, void, void>))
  constexpr basic_outcome(success_type<T> &&o) noexcept(std::is_nothrow_constructible<value_type, T>::value)  // NOLINT
      : base{in_place_type<typename base::_value_type>, detail::extract_value_from_success<value_type>(static_cast<success_type<T> &&>(o))}
  {
    using namespace hooks;
    hook_outcome_move_construction(this, static_cast<success_type<T> &&>(o));
  }

  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(!std::is_void<T>::value && predicate::template enable_compatible_conversion<void, T, void, void>))
  constexpr basic_outcome(const failure_type<T> &o, error_failure_tag /*unused*/ = error_failure_tag()) noexcept(std::is_nothrow_constructible<error_type, T>::value)  // NOLINT
      : base{in_place_type<typename base::_error_type>, detail::extract_error_from_failure<error_type>(o)}
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_copy_construction(this, o);
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(!std::is_void<T>::value && predicate::template enable_compatible_conversion<void, void, T, void>))
  constexpr basic_outcome(const failure_type<T> &o, exception_failure_tag /*unused*/ = exception_failure_tag()) noexcept(std::is_nothrow_constructible<exception_type, T>::value)  // NOLINT
      : base()
      , _ptr(detail::extract_exception_from_failure<exception_type>(o))
  {
    this->_state._status |= detail::status_have_exception;
    using namespace hooks;
    hook_outcome_copy_construction(this, o);
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T, class U)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(!std::is_void<U>::value && predicate::template enable_compatible_conversion<void, T, U, void>))
  constexpr basic_outcome(const failure_type<T, U> &o) noexcept(std::is_nothrow_constructible<error_type, T>::value &&std::is_nothrow_constructible<exception_type, U>::value)  // NOLINT
      : base{in_place_type<typename base::_error_type>, detail::extract_error_from_failure<error_type>(o)}
      , _ptr(detail::extract_exception_from_failure<exception_type>(o))
  {
    if(!o.has_error())
    {
      this->_state._status &= ~detail::status_have_error;
    }
    if(o.has_exception())
    {
      this->_state._status |= detail::status_have_exception;
    }
    using namespace hooks;
    hook_outcome_copy_construction(this, o);
  }

  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(!std::is_void<T>::value && predicate::template enable_compatible_conversion<void, T, void, void>))
  constexpr basic_outcome(failure_type<T> &&o, error_failure_tag /*unused*/ = error_failure_tag()) noexcept(std::is_nothrow_constructible<error_type, T>::value)  // NOLINT
      : base{in_place_type<typename base::_error_type>, detail::extract_error_from_failure<error_type>(static_cast<failure_type<T> &&>(o))}
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_copy_construction(this, o);
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(!std::is_void<T>::value && predicate::template enable_compatible_conversion<void, void, T, void>))
  constexpr basic_outcome(failure_type<T> &&o, exception_failure_tag /*unused*/ = exception_failure_tag()) noexcept(std::is_nothrow_constructible<exception_type, T>::value)  // NOLINT
      : base()
      , _ptr(detail::extract_exception_from_failure<exception_type>(static_cast<failure_type<T> &&>(o)))
  {
    this->_state._status |= detail::status_have_exception;
    using namespace hooks;
    hook_outcome_copy_construction(this, o);
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T, class U)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(!std::is_void<U>::value && predicate::template enable_compatible_conversion<void, T, U, void>))
  constexpr basic_outcome(failure_type<T, U> &&o) noexcept(std::is_nothrow_constructible<error_type, T>::value &&std::is_nothrow_constructible<exception_type, U>::value)  // NOLINT
      : base{in_place_type<typename base::_error_type>, detail::extract_error_from_failure<error_type>(static_cast<failure_type<T, U> &&>(o))}
      , _ptr(detail::extract_exception_from_failure<exception_type>(static_cast<failure_type<T, U> &&>(o)))
  {
    if(!o.has_error())
    {
      this->_state._status &= ~detail::status_have_error;
    }
    if(o.has_exception())
    {
      this->_state._status |= detail::status_have_exception;
    }
    using namespace hooks;
    hook_outcome_move_construction(this, static_cast<failure_type<T, U> &&>(o));
  }

  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  using base::operator==;
  using base::operator!=;
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T, class U, class V, class W)
  OUTCOME_TREQUIRES(OUTCOME_TEXPR(std::declval<detail::devoid<value_type>>() == std::declval<detail::devoid<T>>()),  //
                    OUTCOME_TEXPR(std::declval<detail::devoid<error_type>>() == std::declval<detail::devoid<U>>()),  //
                    OUTCOME_TEXPR(std::declval<detail::devoid<exception_type>>() == std::declval<detail::devoid<V>>()))
  constexpr bool operator==(const basic_outcome<T, U, V, W> &o) const noexcept(                 //
  noexcept(std::declval<detail::devoid<value_type>>() == std::declval<detail::devoid<T>>())     //
  && noexcept(std::declval<detail::devoid<error_type>>() == std::declval<detail::devoid<U>>())  //
  && noexcept(std::declval<detail::devoid<exception_type>>() == std::declval<detail::devoid<V>>()))
  {
    if((this->_state._status & detail::status_have_value) != 0 && (o._state._status & detail::status_have_value) != 0)
    {
      return this->_state._value == o._state._value;  // NOLINT
    }
    if((this->_state._status & detail::status_have_error) != 0 && (o._state._status & detail::status_have_error) != 0  //
       && (this->_state._status & detail::status_have_exception) != 0 && (o._state._status & detail::status_have_exception) != 0)
    {
      return this->_error == o._error && this->_ptr == o._ptr;
    }
    if((this->_state._status & detail::status_have_error) != 0 && (o._state._status & detail::status_have_error) != 0)
    {
      return this->_error == o._error;
    }
    if((this->_state._status & detail::status_have_exception) != 0 && (o._state._status & detail::status_have_exception) != 0)
    {
      return this->_ptr == o._ptr;
    }
    return false;
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T, class U)
  OUTCOME_TREQUIRES(OUTCOME_TEXPR(std::declval<error_type>() == std::declval<T>()),  //
                    OUTCOME_TEXPR(std::declval<exception_type>() == std::declval<U>()))
  constexpr bool operator==(const failure_type<T, U> &o) const noexcept(  //
  noexcept(std::declval<error_type>() == std::declval<T>()) && noexcept(std::declval<exception_type>() == std::declval<U>()))
  {
    if((this->_state._status & detail::status_have_error) != 0 && (o._state._status & detail::status_have_error) != 0  //
       && (this->_state._status & detail::status_have_exception) != 0 && (o._state._status & detail::status_have_exception) != 0)
    {
      return this->_error == o.error() && this->_ptr == o.exception();
    }
    if((this->_state._status & detail::status_have_error) != 0 && (o._state._status & detail::status_have_error) != 0)
    {
      return this->_error == o.error();
    }
    if((this->_state._status & detail::status_have_exception) != 0 && (o._state._status & detail::status_have_exception) != 0)
    {
      return this->_ptr == o.exception();
    }
    return false;
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T, class U, class V, class W)
  OUTCOME_TREQUIRES(OUTCOME_TEXPR(std::declval<detail::devoid<value_type>>() != std::declval<detail::devoid<T>>()),  //
                    OUTCOME_TEXPR(std::declval<detail::devoid<error_type>>() != std::declval<detail::devoid<U>>()),  //
                    OUTCOME_TEXPR(std::declval<detail::devoid<exception_type>>() != std::declval<detail::devoid<V>>()))
  constexpr bool operator!=(const basic_outcome<T, U, V, W> &o) const noexcept(                 //
  noexcept(std::declval<detail::devoid<value_type>>() != std::declval<detail::devoid<T>>())     //
  && noexcept(std::declval<detail::devoid<error_type>>() != std::declval<detail::devoid<U>>())  //
  && noexcept(std::declval<detail::devoid<exception_type>>() != std::declval<detail::devoid<V>>()))
  {
    if((this->_state._status & detail::status_have_value) != 0 && (o._state._status & detail::status_have_value) != 0)
    {
      return this->_state._value != o._state._value;  // NOLINT
    }
    if((this->_state._status & detail::status_have_error) != 0 && (o._state._status & detail::status_have_error) != 0  //
       && (this->_state._status & detail::status_have_exception) != 0 && (o._state._status & detail::status_have_exception) != 0)
    {
      return this->_error != o._error || this->_ptr != o._ptr;
    }
    if((this->_state._status & detail::status_have_error) != 0 && (o._state._status & detail::status_have_error) != 0)
    {
      return this->_error != o._error;
    }
    if((this->_state._status & detail::status_have_exception) != 0 && (o._state._status & detail::status_have_exception) != 0)
    {
      return this->_ptr != o._ptr;
    }
    return true;
  }
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  OUTCOME_TEMPLATE(class T, class U)
  OUTCOME_TREQUIRES(OUTCOME_TEXPR(std::declval<error_type>() != std::declval<T>()),  //
                    OUTCOME_TEXPR(std::declval<exception_type>() != std::declval<U>()))
  constexpr bool operator!=(const failure_type<T, U> &o) const noexcept(  //
  noexcept(std::declval<error_type>() == std::declval<T>()) && noexcept(std::declval<exception_type>() == std::declval<U>()))
  {
    if((this->_state._status & detail::status_have_error) != 0 && (o._state._status & detail::status_have_error) != 0  //
       && (this->_state._status & detail::status_have_exception) != 0 && (o._state._status & detail::status_have_exception) != 0)
    {
      return this->_error != o.error() || this->_ptr != o.exception();
    }
    if((this->_state._status & detail::status_have_error) != 0 && (o._state._status & detail::status_have_error) != 0)
    {
      return this->_error != o.error();
    }
    if((this->_state._status & detail::status_have_exception) != 0 && (o._state._status & detail::status_have_exception) != 0)
    {
      return this->_ptr != o.exception();
    }
    return true;
  }

  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  constexpr void swap(basic_outcome &o) noexcept((std::is_void<value_type>::value || detail::is_nothrow_swappable<value_type>::value)     //
                                                 && (std::is_void<error_type>::value || detail::is_nothrow_swappable<error_type>::value)  //
                                                 && (std::is_void<exception_type>::value || detail::is_nothrow_swappable<exception_type>::value))
  {
#ifdef __cpp_exceptions
    constexpr bool value_throws = !std::is_void<value_type>::value && !detail::is_nothrow_swappable<value_type>::value;
    constexpr bool error_throws = !std::is_void<error_type>::value && !detail::is_nothrow_swappable<error_type>::value;
    constexpr bool exception_throws = !std::is_void<exception_type>::value && !detail::is_nothrow_swappable<exception_type>::value;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127)  // conditional expression is constant
#endif
    if(!exception_throws && !value_throws && !error_throws)
    {
      // Simples
      detail::basic_result_storage_swap<value_throws, error_throws>(*this, o);
      using std::swap;
      swap(this->_ptr, o._ptr);
      return;
    }
    struct _
    {
      basic_outcome &a, &b;
      bool exceptioned{false};
      bool all_good{false};
      ~_()
      {
        if(!all_good)
        {
          // We lost one of the values
          a._state._status |=  detail::status_lost_consistency;
          b._state._status |=  detail::status_lost_consistency;
          return;
        }
        if(exceptioned)
        {
          // The value + error swap threw an exception. Try to swap back _ptr
          try
          {
            strong_swap(all_good, a._ptr, b._ptr);
          }
          catch(...)
          {
            // We lost one of the values
            a._state._status |= detail::status_lost_consistency;
            b._state._status |= detail::status_lost_consistency;
            // throw away second exception
          }

          // Prevent has_value() == has_error() or has_value() == has_exception()
          auto check = [](basic_outcome *t) {
            if(t->has_value() && (t->has_error() || t->has_exception()))
            {
              t->_state._status &= ~(detail::status_have_error | detail::status_have_exception);
              t->_state._status |= detail::status_lost_consistency;
            }
            if(!t->has_value() && !(t->has_error() || t->has_exception()))
            {
              // Choose error, for no particular reason
              t->_state._status |= detail::status_have_error | detail::status_lost_consistency;
            }
          };
          check(&a);
          check(&b);
        }
      }
    } _{*this, o};
    strong_swap(_.all_good, this->_ptr, o._ptr);
    _.exceptioned = true;
    detail::basic_result_storage_swap<value_throws, error_throws>(*this, o);
    _.exceptioned = false;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#else
    detail::basic_result_storage_swap<false, false>(*this, o);
    using std::swap;
    swap(this->_ptr, o._ptr);
#endif
  }

  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  failure_type<error_type, exception_type> as_failure() const &
  {
    if(this->has_error() && this->has_exception())
    {
      return failure_type<error_type, exception_type>(this->assume_error(), this->assume_exception());
    }
    if(this->has_exception())
    {
      return failure_type<error_type, exception_type>(in_place_type<exception_type>, this->assume_exception());
    }
    return failure_type<error_type, exception_type>(in_place_type<error_type>, this->assume_error());
  }

  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  failure_type<error_type, exception_type> as_failure() &&
  {
    if(this->has_error() && this->has_exception())
    {
      return failure_type<error_type, exception_type>(static_cast<S &&>(this->assume_error()), static_cast<P &&>(this->assume_exception()));
    }
    if(this->has_exception())
    {
      return failure_type<error_type, exception_type>(in_place_type<exception_type>, static_cast<P &&>(this->assume_exception()));
    }
    return failure_type<error_type, exception_type>(in_place_type<error_type>, static_cast<S &&>(this->assume_error()));
  }
};

/*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
OUTCOME_TEMPLATE(class T, class U, class V,  //
                 class R, class S, class P, class N)
OUTCOME_TREQUIRES(OUTCOME_TEXPR(std::declval<basic_outcome<R, S, P, N>>() == std::declval<basic_result<T, U, V>>()))
constexpr inline bool operator==(const basic_result<T, U, V> &a, const basic_outcome<R, S, P, N> &b) noexcept(  //
noexcept(std::declval<basic_outcome<R, S, P, N>>() == std::declval<basic_result<T, U, V>>()))
{
  return b == a;
}
/*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
OUTCOME_TEMPLATE(class T, class U, class V,  //
                 class R, class S, class P, class N)
OUTCOME_TREQUIRES(OUTCOME_TEXPR(std::declval<basic_outcome<R, S, P, N>>() != std::declval<basic_result<T, U, V>>()))
constexpr inline bool operator!=(const basic_result<T, U, V> &a, const basic_outcome<R, S, P, N> &b) noexcept(  //
noexcept(std::declval<basic_outcome<R, S, P, N>>() != std::declval<basic_result<T, U, V>>()))
{
  return b != a;
}
/*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
template <class R, class S, class P, class N> inline void swap(basic_outcome<R, S, P, N> &a, basic_outcome<R, S, P, N> &b) noexcept(noexcept(a.swap(b)))
{
  a.swap(b);
}

namespace hooks
{
  /*! AWAITING HUGO JSON CONVERSION TOOL
SIGNATURE NOT RECOGNISED
*/
  template <class R, class S, class P, class NoValuePolicy, class U> constexpr inline void override_outcome_exception(basic_outcome<R, S, P, NoValuePolicy> *o, U &&v) noexcept
  {
    o->_ptr = static_cast<U &&>(v);  // NOLINT
    o->_state._status |= detail::status_have_exception;
  }
}  // namespace hooks

OUTCOME_V2_NAMESPACE_END

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "detail/basic_outcome_exception_observers_impl.hpp"

#if !defined(NDEBUG)
OUTCOME_V2_NAMESPACE_BEGIN
// Check is trivial in all ways except default constructibility and standard layout
// static_assert(std::is_trivial<basic_outcome<int, long, double, policy::all_narrow>>::value, "outcome<int> is not trivial!");
// static_assert(std::is_trivially_default_constructible<basic_outcome<int, long, double, policy::all_narrow>>::value, "outcome<int> is not trivially default constructible!");
static_assert(std::is_trivially_copyable<basic_outcome<int, long, double, policy::all_narrow>>::value, "outcome<int> is not trivially copyable!");
static_assert(std::is_trivially_assignable<basic_outcome<int, long, double, policy::all_narrow>, basic_outcome<int, long, double, policy::all_narrow>>::value, "outcome<int> is not trivially assignable!");
static_assert(std::is_trivially_destructible<basic_outcome<int, long, double, policy::all_narrow>>::value, "outcome<int> is not trivially destructible!");
static_assert(std::is_trivially_copy_constructible<basic_outcome<int, long, double, policy::all_narrow>>::value, "outcome<int> is not trivially copy constructible!");
static_assert(std::is_trivially_move_constructible<basic_outcome<int, long, double, policy::all_narrow>>::value, "outcome<int> is not trivially move constructible!");
static_assert(std::is_trivially_copy_assignable<basic_outcome<int, long, double, policy::all_narrow>>::value, "outcome<int> is not trivially copy assignable!");
static_assert(std::is_trivially_move_assignable<basic_outcome<int, long, double, policy::all_narrow>>::value, "outcome<int> is not trivially move assignable!");
// Can't be standard layout as non-static member data is defined in more than one inherited class
// static_assert(std::is_standard_layout<basic_outcome<int, long, double, policy::all_narrow>>::value, "outcome<int> is not a standard layout type!");
OUTCOME_V2_NAMESPACE_END
#endif

#endif
