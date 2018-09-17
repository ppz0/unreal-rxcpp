// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

/*! \file rx-take.hpp

    \brief For the first count items from this observable emit them from the new observable that is returned.

    \tparam Count the type of the items counter.

    \param t the number of items to take.

    \return An observable that emits only the first t items emitted by the source Observable, or all of the items from the source observable if that observable emits fewer than t items.

    \sample
    \snippet take.cpp take sample
    \snippet output.txt take sample
*/

#if !defined(RXCPP_OPERATORS_RX_IS_VALID_HPP)
#define RXCPP_OPERATORS_RX_IS_VALID_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class... AN>
struct is_valid_invalid_arguments {};

template<class... AN>
struct is_valid_invalid : public rxo::operator_base<is_valid_invalid_arguments<AN...>> {
    using type = observable<is_valid_invalid_arguments<AN...>, is_valid_invalid<AN...>>;
};
template<class... AN>
using is_valid_invalid_t = typename is_valid_invalid<AN...>::type;

template<class T, class Object>
struct is_valid
{
    typedef rxu::decay_t<T> source_value_type;
		typedef rxu::decay_t<Object> object_type;
		TWeakObjectPtr<UObject> test;

    is_valid(object_type o)
        : test(o)
    {
    }

    template<class Subscriber>
    struct take_while_observer
    {
        typedef take_while_observer<Subscriber> this_type;
        typedef source_value_type value_type;
        typedef rxu::decay_t<Subscriber> dest_type;
        typedef observer<value_type, this_type> observer_type;
        dest_type dest;
        TWeakObjectPtr<UObject> test;

        take_while_observer(dest_type d, TWeakObjectPtr<UObject> t)
                : dest(std::move(d))
                , test(std::move(t))
        {
        }
        void on_next(source_value_type v) const {
            if (test.IsValid()) {
                dest.on_next(v);
            } else {
                dest.on_completed();
            }
        }
        void on_error(std::exception_ptr e) const {
            dest.on_error(e);
        }
        void on_completed() const {
            dest.on_completed();
        }

        static subscriber<value_type, observer_type> make(dest_type d, TWeakObjectPtr<UObject> t) {
            return make_subscriber<value_type>(d, this_type(d, std::move(t)));
        }
    };

    template<class Subscriber>
    auto operator()(Subscriber dest) const
    -> decltype(take_while_observer<Subscriber>::make(std::move(dest), test)) {
        return  take_while_observer<Subscriber>::make(std::move(dest), test);
    }
};

}

/*! @copydoc rx-is_valid.hpp
*/
template<class... AN>
auto is_valid(AN&&... an)
    ->     operator_factory<is_valid_tag, AN...> {
    return operator_factory<is_valid_tag, AN...>(std::make_tuple(std::forward<AN>(an)...));
}

}

template<>
struct member_overload<is_valid_tag>
{
    template<class Observable, class Object,
            class SourceValue = rxu::value_type_t<Observable>,
            class TakeWhileValidObject = rxo::detail::is_valid<SourceValue, rxu::decay_t<Object>>>
    static auto member(Observable&& o, Object&& p)
    -> decltype(o.template lift<SourceValue>(TakeWhileValidObject(std::forward<Object>(p)))) {
        return      o.template lift<SourceValue>(TakeWhileValidObject(std::forward<Object>(p)));
    }

    template<class... AN>
    static operators::detail::is_valid_invalid_t<AN...> member(AN...) {
        std::terminate();
        return {};
        static_assert(sizeof...(AN) == 10000, "take takes (optional Count)");
    }
};

}

#endif
