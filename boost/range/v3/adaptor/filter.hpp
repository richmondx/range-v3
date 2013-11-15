// Boost.Range library
//
//  Copyright Thorsten Ottosen, Neil Groves 2006 - 2008. 
//  Copyright Eric Niebler 2013.
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_V3_ADAPTOR_FILTER_HPP
#define BOOST_RANGE_V3_ADAPTOR_FILTER_HPP

#include <cassert>
#include <utility>
#include <iterator>
#include <type_traits>
#include <boost/range/v3/detail/iterator_facade.hpp>
#include <boost/range/v3/range_fwd.hpp>
#include <boost/range/v3/range_traits.hpp>
#include <boost/range/v3/begin_end.hpp>
#include <boost/range/v3/detail/function_wrapper.hpp>
#include <boost/range/v3/detail/compressed_pair.hpp>

namespace boost
{
    namespace range
    {
        inline namespace v3
        {
            namespace detail
            {
                auto filter_range_category(std::input_iterator_tag)           -> std::input_iterator_tag;
                auto filter_range_category(std::forward_iterator_tag)         -> std::forward_iterator_tag;
                auto filter_range_category(std::bidirectional_iterator_tag)   -> std::bidirectional_iterator_tag;
            }

            template<typename Rng, typename Pred>
            struct filter_range
            {
            private:
                detail::compressed_pair<Rng, detail::function_wrapper<Pred>> rng_and_pred_;

                // FltRng is either filter_range or filter_range const.
                template<typename FltRng>
                struct basic_iterator
                  : range::iterator_facade<
                        basic_iterator<FltRng>
                      , range_value_t<Rng>
                      , decltype(detail::filter_range_category(range_category_t<Rng>{}))
                      , decltype(*range::begin(std::declval<FltRng &>().rng_and_pred_.first()))
                    >
                {
                private:
                    friend struct filter_range;
                    friend class range::iterator_core_access;
                    using base_range_iterator = decltype(range::begin(std::declval<FltRng &>().rng_and_pred_.first()));

                    FltRng *rng_;
                    base_range_iterator it_;

                    basic_iterator(FltRng &rng, base_range_iterator it)
                      : rng_(&rng), it_(std::move(it))
                    {
                        satisfy();
                    }
                    void increment()
                    {
                        assert(it_ != range::end(rng_->rng_and_pred_.first()));
                        ++it_; satisfy();
                    }
                    void decrement()
                    {
                        while(!rng_->rng_and_pred_.second()(*--it_)) {}
                    }
                    bool equal(basic_iterator const &that) const
                    {
                        assert(rng_ == that.rng_);
                        return it_ == that.it_;
                    }
                    auto dereference() const -> decltype(*it_)
                    {
                        assert(it_ != range::end(rng_->rng_and_pred_.first()));
                        return *it_;
                    }
                    void satisfy()
                    {
                        auto const e = range::end(rng_->rng_and_pred_.first());
                        while(it_ != e && !rng_->rng_and_pred_.second()(*it_))
                            ++it_;
                    }
                public:
                    basic_iterator()
                      : rng_{}, it_{}
                    {}
                    // For iterator -> const_iterator conversion
                    template<typename OtherFltRng,
                             typename = typename std::enable_if<
                                            !std::is_const<OtherFltRng>::value>::type>
                    basic_iterator(basic_iterator<OtherFltRng> that)
                      : rng_(that.rng_), it_(std::move(that).it_)
                    {}
                };
            public:
                using iterator       = basic_iterator<filter_range>;
                using const_iterator = basic_iterator<filter_range const>;

                filter_range(Rng && rng, Pred pred)
                  : rng_and_pred_{std::forward<Rng>(rng), std::move(pred)}
                {}
                iterator begin()
                {
                    return {*this, range::begin(rng_and_pred_.first())};
                }
                iterator end()
                {
                    return {*this, range::end(rng_and_pred_.first())};
                }
                const_iterator begin() const
                {
                    return {*this, range::begin(rng_and_pred_.first())};
                }
                const_iterator end() const
                {
                    return {*this, range::end(rng_and_pred_.first())};
                }
                bool operator!() const
                {
                    return begin() == end();
                }
                explicit operator bool() const
                {
                    return begin() != end();
                }
                Rng & base()
                {
                    return rng_and_pred_.first();
                }
                Rng const & base() const
                {
                    return rng_and_pred_.first();
                }
            };

            constexpr struct filterer
            {
            private:
                template<typename Pred>
                class filterer1
                {
                    Pred pred_;
                public:
                    filterer1(Pred pred)
                      : pred_(std::move(pred))
                    {}
                    template<typename Rng>
                    friend filter_range<Rng, Pred> operator|(Rng && rng, filterer1 && pred)
                    {
                        return {std::forward<Rng>(rng), std::move(pred).pred_};
                    }
                    template<typename Rng>
                    friend filter_range<Rng, Pred> operator|(Rng && rng, filterer1 const & pred)
                    {
                        return {std::forward<Rng>(rng), pred.pred_};
                    }
                };
            public:
                template<typename Rng, typename Pred>
                filter_range<Rng, Pred> operator()(Rng && rng, Pred pred) const
                {
                    return {std::forward<Rng>(rng), std::move(pred)};
                }
                template<typename Pred>
                constexpr filterer1<Pred> operator()(Pred pred) const
                {
                    return pred;
                }
            } filter {};
        }
    }
}

#endif
