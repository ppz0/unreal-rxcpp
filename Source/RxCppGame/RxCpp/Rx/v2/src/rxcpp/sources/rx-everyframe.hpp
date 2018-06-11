// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_EVERYFRAME_HPP)
#define RXCPP_SOURCES_RX_EVERYFRAME_HPP

#include "../rx-includes.hpp"

/*! \file rx-everyframe.hpp

    \brief Returns an observable that emits a sequential integer every specified time everyframe, on the specified scheduler.

    \tparam Coordination  the type of the scheduler (optional)

    \param  period   period between emitted values
    \param  cn       the scheduler to use for scheduling the items (optional)

    \return  Observable that sends a sequential integer each time everyframe

    \sample
    \snippet everyframe.cpp everyframe sample
    \snippet output.txt everyframe sample

    \sample
    \snippet everyframe.cpp immediate everyframe sample
    \snippet output.txt immediate everyframe sample

    \sample
    \snippet everyframe.cpp threaded everyframe sample
    \snippet output.txt threaded everyframe sample

    \sample
    \snippet everyframe.cpp threaded immediate everyframe sample
    \snippet output.txt threaded immediate everyframe sample
*/

namespace rxcpp {

namespace sources {

namespace detail {

template<class Coordination>
struct everyframe : public source_base<long>
{
    typedef everyframe<Coordination> this_type;

    typedef rxu::decay_t<Coordination> coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;

    struct everyframe_initial_type
    {
        everyframe_initial_type(rxsc::scheduler::clock_type::time_point i, rxsc::scheduler::clock_type::duration p, long c, coordination_type cn)
            : initial(i)
            , period(p)
            , count(c)
            , coordination(std::move(cn))
        {
        }
        rxsc::scheduler::clock_type::time_point initial;
        rxsc::scheduler::clock_type::duration period;
        long count;
        coordination_type coordination;
    };
    everyframe_initial_type initial;

    everyframe(rxsc::scheduler::clock_type::time_point i, rxsc::scheduler::clock_type::duration p, long c, coordination_type cn)
        : initial(i, p, c, std::move(cn))
    {
    }
    template<class Subscriber>
    void on_subscribe(Subscriber o) const {
        static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

        // creates a worker whose lifetime is the same as this subscription
        auto coordinator = initial.coordination.create_coordinator(o.get_subscription());

        auto controller = coordinator.get_worker();

        auto counter = std::make_shared<long>(0);

        auto count = initial.count;

        auto producer = [o, counter, count](const rxsc::schedulable&) {
            ++(*counter);

            if ((*counter) % count == 0)
                o.on_next(*counter);
        };

        auto selectedProducer = on_exception(
            [&](){return coordinator.act(producer);},
            o);
        if (selectedProducer.empty()) {
            return;
        }

        controller.schedule_periodically(initial.initial, initial.period, selectedProducer.get());
    }
};

template<class Duration, class Coordination>
struct defer_everyframe : public defer_observable<
    rxu::all_true<
        std::is_convertible<Duration, rxsc::scheduler::clock_type::duration>::value,
        is_coordination<Coordination>::value>,
    void,
    everyframe, Coordination>
{
};

}

/*! @copydoc rx-everyframe.hpp
 */
template<class Coordination>
auto everyframe(long count, Coordination cn)
    ->  typename std::enable_if<
                    detail::defer_everyframe<rxsc::scheduler::clock_type::duration, Coordination>::value,
        typename    detail::defer_everyframe<rxsc::scheduler::clock_type::duration, Coordination>::observable_type>::type {
    return          detail::defer_everyframe<rxsc::scheduler::clock_type::duration, Coordination>::make(cn.now(), rxsc::scheduler::clock_type::duration(1), count, std::move(cn));
}

/*! @copydoc rx-everyframe.hpp
 */
template<class Coordination>
auto everyframe(rxsc::scheduler::clock_type::time_point when, UObject* owner, long count, Coordination cn)
    ->  typename std::enable_if<
                    detail::defer_everyframe<rxsc::scheduler::clock_type::duration, Coordination>::value,
        typename    detail::defer_everyframe<rxsc::scheduler::clock_type::duration, Coordination>::observable_type>::type {
    return          detail::defer_everyframe<rxsc::scheduler::clock_type::duration, Coordination>::make(when, rxsc::scheduler::clock_type::duration(1), count, std::move(cn));
}

}

}

#endif
