
#pragma once
#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

/* concept Range
{
    std::begin(range)
    std::end(range)
}
*/

// Traits

template <typename Rng>
using GetIter = decltype(std::begin(std::declval<Rng &>()));

template <typename Rng>
using GetValueType = typename GetIter<Rng>::value_type;

template <typename, typename = void>
struct IsRange : std::false_type {};
template <typename Range>
struct IsRange<Range, std::void_t<GetIter<Range>>> : std::true_type {};

template <typename Range>
struct IsRandomAccess
    : std::is_same<
          typename std::iterator_traits<GetIter<Range>>::iterator_category,
          std::random_access_iterator_tag> {};

template <typename Range, typename E = void>  //
class View;

template <typename>
struct IsView : std::false_type {};
template <typename Range>
struct IsView<View<Range>> : std::true_type {};
template <typename Range>
struct IsView<Range &> : IsView<std::remove_reference_t<Range>> {};

template <typename Range>
struct IsSized : IsRandomAccess<Range> {};
template <typename Range>
struct IsSized<Range &> : IsSized<std::remove_reference_t<Range>> {};

// Functor Wrapper

template <typename F>  //
struct IsFunctionPointer {
  // NOLINTNEXTLINE
  static const bool value =
      std::is_pointer<F>::value
          ? std::is_function<typename std::remove_pointer<F>::type>::value
          : false;
};

template <typename F, typename = void>  //
struct FunctorWrapper : public F {
  explicit FunctorWrapper(F &&f) : F(std::move(f)) {}
  explicit FunctorWrapper(F const &f) : F(f) {}
};

template <typename F>  //
struct FunctorWrapperBase {
  explicit FunctorWrapperBase(F &&f) : _impl(std::move(f)) {}
  explicit FunctorWrapperBase(F const &f) : _impl(f) {}

  template <typename... Args>  //
  decltype(auto) operator()(Args &&...args) noexcept(
      std::is_nothrow_invocable_v<F, Args &&...>) {
    return std::invoke(_impl, std::forward<Args>(args)...);
  }

 private:
  F _impl;
};

template <typename F>  //
struct FunctorWrapper<F, std::enable_if_t<std::is_final_v<F>>>
    : FunctorWrapperBase<F> {
  using FunctorWrapperBase<F>::FunctorWrapperBase;
};

template <typename F>  //
struct FunctorWrapper<F, std::enable_if_t<std::is_function_v<F>>>
    : FunctorWrapperBase<std::decay_t<F>> {
  using FunctorWrapperBase<std::decay_t<F>>::FunctorWrapperBase;
};

template <typename F>  //
struct FunctorWrapper<F, std::enable_if_t<IsFunctionPointer<F>::value>>
    : FunctorWrapperBase<F> {
  using FunctorWrapperBase<F>::FunctorWrapperBase;
};

template <typename... Funcs>  //
class Overloaded : private FunctorWrapper<Funcs>... {
 public:
  using FunctorWrapper<Funcs>::operator()...;

  explicit Overloaded(Funcs &&...fns)
      :  //
        FunctorWrapper<Funcs...>(std::forward<Funcs>(fns)...) {}
};
template <typename... Funcs>
Overloaded(Funcs &&...) -> Overloaded<Funcs...>;

// View

template <typename Range>  //
class View<Range, std::enable_if_t<!IsView<Range>::value>> {
 public:
  using iterator = GetIter<Range>;

  iterator begin()  // NOLINT
  {
    return _begin;
  }

  iterator end()  // NOLINT
  {
    return _end;
  }

  typename iterator::difference_type size() const  // NOLINT
  {
    return std::distance(_begin, _end);
  }

  explicit View(iterator b, iterator e) : _begin(b), _end(e) {}

  explicit View(Range &range) : View(std::begin(range), std::end(range)) {}

 private:
  iterator _begin;
  iterator _end;
};

template <typename Range>  //
class View<Range, std::enable_if_t<IsView<Range>::value>>
    : public std::remove_reference_t<Range> {
  using Underlying = std::remove_reference_t<Range>;

 public:
  explicit View(Underlying &range) : Underlying(range) {}
};
template <typename Rng>
View(Rng &&) -> View<Rng>;

// Sized

template <typename Range, typename E = void>
class Sized;

template <typename Range>
struct IsSized<Sized<Range>> : std::true_type {};

template <typename Range>  //
class Sized<Range, std::enable_if_t<!IsSized<Range>::value>> {
  using iterator = GetIter<Range>;

 public:
  typename iterator::difference_type size() const  // NOLINT
  {
    return _size;
  }

  explicit Sized(iterator b, iterator e) : _size(std::distance(b, e)) {}

  explicit Sized(Range &range) : _size(std::size(range)) {}

 private:
  typename iterator::difference_type _size;
};

template <typename Range>  //
class Sized<Range, std::enable_if_t<IsSized<Range>::value>> {
  using iterator = GetIter<Range>;

 public:
  explicit Sized(iterator b, iterator e) {}

  explicit Sized(Range &range) {}
};

template <typename Range>  //
class SizedView : private Sized<Range>, private View<Range> {
 public:
  using iterator = GetIter<Range>;

  using View<Range>::begin;
  using View<Range>::end;

  typename iterator::difference_type size() const  // NOLINT
  {
    if constexpr (IsRandomAccess<Range>::value || IsSized<Range>::value) {
      return View<Range>::size();
    } else {
      return Sized<Range>::size();
    }
  }

  explicit SizedView(iterator b, iterator e)
      : Sized<Range>(b, e), View<Range>(b, e) {}

  explicit SizedView(Range &range) : Sized<Range>(range), View<Range>(range) {}
};

template <typename Range>  //
class SizedView<SizedView<Range>> : public SizedView<Range> {};

template <typename Rng>
SizedView(Rng &&) -> SizedView<Rng>;

template <typename Range>
struct IsView<SizedView<Range>> : std::true_type {};
template <typename Range>
struct IsSized<SizedView<Range>> : std::true_type {};

// OwningView

template <typename Range>  //
class OwningView : private std::vector<GetValueType<Range>> {
  using Inner = std::vector<GetValueType<Range>>;

 public:
  using typename Inner::iterator;
  using typename Inner::value_type;

  using Inner::begin;
  using Inner::end;
  using Inner::size;

  explicit OwningView(Range &&range) {
    Inner::reserve(std::size(range));
    for (auto &&element : range) {
      Inner::emplace_back(std::move(element));
    }
  }
};

template <typename Range>  //
class OwningView<OwningView<Range>> : public OwningView<Range> {};

// RefCountView

template <typename Range>  //
struct RefCountView : SizedView<Range> {
 public:
  RefCountView(int &rc, Range &rng)  //
      : SizedView<Range>(rng), ref_count(rc) {
    ref_count++;
  }

  ~RefCountView() { ref_count--; }

 private:
  int &ref_count;
};
template <typename Rng>  //
RefCountView(int &rc, Rng &&) -> RefCountView<Rng>;

template <typename Range>
struct IsView<RefCountView<Range>> : std::true_type {};
template <typename Range>
struct IsSized<RefCountView<Range>> : std::true_type {};

// Non Terminal Combinators

// Map

template <typename Range, typename UnaryOp>  //
class MapView : private SizedView<Range>, private FunctorWrapper<UnaryOp> {
  template <typename It>  //
  struct MapIterator : private It, private FunctorWrapper<UnaryOp> {
    using iterator_category = std::forward_iterator_tag;

    using reference =
        std::invoke_result_t<FunctorWrapper<UnaryOp>, typename It::reference>;

    using value_type = std::decay_t<reference>;
    using pointer = value_type *;
    using difference_type = typename It::difference_type;

    MapIterator(FunctorWrapper<UnaryOp> &u, It i)
        : It(i), FunctorWrapper<UnaryOp>(u) {}

    reference operator*() {
      return FunctorWrapper<UnaryOp>::operator()(It::operator*());
    }

    MapIterator &operator++() {
      It::operator++();
      return *this;
    }

    MapIterator operator++(int)  // NOLINT
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    bool operator==(const MapIterator &rhs) const {
      return static_cast<It const &>(*this) == static_cast<It const &>(rhs);
    }
    bool operator!=(const MapIterator &rhs) const { return !(*this == rhs); }
  };

 public:
  using iterator = MapIterator<GetIter<Range>>;

  iterator begin()  // NOLINT
  {
    return {static_cast<FunctorWrapper<UnaryOp> &>(*this),
            SizedView<Range>::begin()};
  }

  iterator end()  // NOLINT
  {
    return {static_cast<FunctorWrapper<UnaryOp> &>(*this),
            SizedView<Range>::end()};
  }

  using SizedView<Range>::size;

  template <typename U>
  explicit MapView(Range &range, U &&op)
      : SizedView<Range>(range),  //
        FunctorWrapper<UnaryOp>(std::forward<U>(op)) {}
};
template <typename Rng, typename U>  //
MapView(Rng &&c, U &&u) -> MapView<Rng, std::remove_reference_t<U>>;

template <typename Range, typename UnaryOp>
struct IsView<MapView<Range, UnaryOp>> : std::true_type {};
template <typename Range, typename UnaryOp>
struct IsSized<MapView<Range, UnaryOp>> : std::true_type {};

namespace ranges {
template <typename UnaryOp>
struct Map : private FunctorWrapper<UnaryOp> {
  template <typename Range>  //
  auto operator()(Range &&input_range) {
    return MapView{input_range, static_cast<FunctorWrapper<UnaryOp> &>(*this)};
  }

  explicit Map(UnaryOp &&o) : FunctorWrapper<UnaryOp>(std::move(o)) {}
  explicit Map(UnaryOp const &o) : FunctorWrapper<UnaryOp>(o) {}
};
}  // namespace ranges

template <typename Range, typename UnaryOp>  //
inline auto Map(Range &&input_range, UnaryOp &&op) {
  return ranges::Map(std::forward<UnaryOp>(op))(
      std::forward<Range>(input_range));
}

// Filter

template <typename Range, typename Pred>  //
class FilterView : private SizedView<Range>, private FunctorWrapper<Pred> {
  template <typename It>  //
  struct FilterIterator : private FunctorWrapper<Pred> {
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename It::value_type;
    using difference_type = typename It::difference_type;
    using reference = typename It::reference;
    using pointer = typename It::pointer;

    FilterIterator(FunctorWrapper<Pred> &p, It i, It e)
        : FunctorWrapper<Pred>(p), it(i), end(e) {
      Advance();
    }

    reference operator*() { return *it; }

    FilterIterator &operator++() {
      ++it;
      Advance();
      return *this;
    }

    FilterIterator operator++(int)  // NOLINT
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    bool operator==(const FilterIterator &rhs) const { return it == rhs.it; }
    bool operator!=(const FilterIterator &rhs) const { return !(*this == rhs); }

    void Advance() {
      while (!(it == end) && !FunctorWrapper<Pred>::operator()(*it)) {
        ++it;
      }
    }

    It it;
    It end;
  };

 public:
  using iterator = FilterIterator<GetIter<Range>>;

  iterator begin()  // NOLINT
  {
    return {static_cast<FunctorWrapper<Pred> &>(*this),
            SizedView<Range>::begin(), SizedView<Range>::end()};
  }

  iterator end()  // NOLINT
  {
    return {static_cast<FunctorWrapper<Pred> &>(*this), SizedView<Range>::end(),
            SizedView<Range>::end()};
  }

  // Conservative
  using SizedView<Range>::size;

  template <typename P>
  explicit FilterView(Range &range, P &&pred)
      : SizedView<Range>(range),  //
        FunctorWrapper<Pred>(std::forward<P>(pred)) {}
};
template <typename Rng, typename Pred>  //
FilterView(Rng &&c, Pred &&p) -> FilterView<Rng, std::remove_reference_t<Pred>>;

template <typename Range, typename Pred>
struct IsView<FilterView<Range, Pred>> : std::true_type {};
template <typename Range, typename Pred>
struct IsSized<FilterView<Range, Pred>> : std::true_type {};

namespace ranges {
template <typename Pred>
struct Filter : private FunctorWrapper<Pred> {
  template <typename Range>  //
  auto operator()(Range &&range) {
    return FilterView{range, static_cast<FunctorWrapper<Pred> &>(*this)};
  }

  explicit Filter(Pred &&p) : FunctorWrapper<Pred>(std::move(p)) {}
  explicit Filter(Pred const &p) : FunctorWrapper<Pred>(p) {}
};
}  // namespace ranges

template <typename Range, typename Pred>  //
inline auto Filter(Range &&range, Pred &&op) {
  return ranges::Filter(std::forward<Pred>(op))(std::forward<Range>(range));
}

// FilterMap

template <typename Range, typename Func>  //
class FilterMapView : private SizedView<Range>, private FunctorWrapper<Func> {
  template <typename It>  //
  struct FilterMapIterator : private FunctorWrapper<Func> {
    using iterator_category = std::forward_iterator_tag;

    using reference =
        typename std::invoke_result_t<Func, typename It::reference>::value_type;

    using value_type = std::decay_t<reference>;
    using pointer = value_type *;
    using difference_type = typename It::difference_type;

    FilterMapIterator(FunctorWrapper<Func> &f, It i, It e)
        : FunctorWrapper<Func>(f), it(i), end(e) {
      Advance();
    }

    reference operator*() { return current.value(); }

    FilterMapIterator &operator++() {
      ++it;
      Advance();
      return *this;
    }

    FilterMapIterator operator++(int)  // NOLINT
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    bool operator==(const FilterMapIterator &rhs) const { return it == rhs.it; }
    bool operator!=(const FilterMapIterator &rhs) const {
      return !(*this == rhs);
    }

    void Advance() {
      while (!(it == end)) {
        current = FunctorWrapper<Func>::operator()(*it);
        if (current.has_value()) {
          break;
        }
        ++it;
      }
    }

    It it;
    It end;
    std::optional<value_type> current;
  };

 public:
  using iterator = FilterMapIterator<GetIter<Range>>;

  iterator begin()  // NOLINT
  {
    return {static_cast<FunctorWrapper<Func> &>(*this),
            SizedView<Range>::begin(), SizedView<Range>::end()};
  }

  iterator end()  // NOLINT
  {
    return {static_cast<FunctorWrapper<Func> &>(*this), SizedView<Range>::end(),
            SizedView<Range>::end()};
  }

  // Conservative
  using SizedView<Range>::size;

  template <typename P>
  explicit FilterMapView(Range &range, P &&func)
      : SizedView<Range>(range),  //
        FunctorWrapper<Func>(std::forward<P>(func)) {}
};
template <typename Rng, typename Func>  //
FilterMapView(Rng &&c, Func &&p)
    -> FilterMapView<Rng, std::remove_reference_t<Func>>;

template <typename Range, typename Func>
struct IsView<FilterMapView<Range, Func>> : std::true_type {};
template <typename Range, typename Func>
struct IsSized<FilterMapView<Range, Func>> : std::true_type {};

namespace ranges {
template <typename Func>
struct FilterMap : private FunctorWrapper<Func> {
  template <typename Range>  //
  auto operator()(Range &&range) {
    return FilterMapView{range, static_cast<FunctorWrapper<Func> &>(*this)};
  }

  explicit FilterMap(Func &&p) : FunctorWrapper<Func>(std::move(p)) {}
  explicit FilterMap(Func const &p) : FunctorWrapper<Func>(p) {}
};
}  // namespace ranges

template <typename Range, typename Func>  //
inline auto FilterMap(Range &&range, Func &&op) {
  return ranges::FilterMap(std::forward<Func>(op))(std::forward<Range>(range));
}

//  Take

template <typename Range, typename = void>  //
class TakeView : private SizedView<Range> {
  template <typename It>
  struct TakeIterator {
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename It::value_type;
    using difference_type = typename It::difference_type;
    using reference = typename It::reference;
    using pointer = typename It::pointer;

    reference operator*() { return *it; }

    TakeIterator &operator++() {
      --left;
      ++it;
      if (it == end) {
        left = 0;
      }
      return *this;
    }

    TakeIterator operator++(int)  // NOLINT
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    bool operator==(const TakeIterator &rhs) const {
      return left == rhs.left && end == rhs.end;
    }

    bool operator!=(const TakeIterator &rhs) const { return !(*this == rhs); }

    TakeIterator(It i, It e, difference_type l)
        : it(i), end(e), left(it == end ? 0 : l) {}

    It it;
    It end;
    difference_type left;
  };

 public:
  using iterator = TakeIterator<GetIter<Range>>;

  iterator begin()  // NOLINT
  {
    return {SizedView<Range>::begin(), SizedView<Range>::end(), size()};
  }

  iterator end()  // NOLINT
  {
    auto &&end = SizedView<Range>::end();
    return {end, end, 0};
  }

  typename iterator::difference_type size() const  // NOLINT
  {
    return std::min(SizedView<Range>::size(), _count);
  }

  explicit TakeView(Range &range, typename iterator::difference_type count)
      : SizedView<Range>(range),  //
        _count(count) {}

 private:
  typename iterator::difference_type _count;
};

template <typename Range>  //
class TakeView<Range, std::enable_if_t<IsRandomAccess<Range>::value>>
    : private SizedView<Range> {
 public:
  using iterator = GetIter<Range>;

  using SizedView<Range>::begin;

  typename iterator::difference_type size() const  // NOLINT
  {
    return std::min(SizedView<Range>::size(), _count);
  }

  iterator end()  // NOLINT
  {
    return begin() + size();
  }

  explicit TakeView(Range &range, typename iterator::difference_type count)
      : SizedView<Range>(range),  //
        _count(count) {}

 private:
  typename iterator::difference_type _count;
};

template <typename Rng>  //
TakeView(Rng &&, size_t)
    -> TakeView<Rng, std::enable_if_t<IsRandomAccess<Rng>::value>>;

template <typename Range>
struct IsView<TakeView<Range>> : std::true_type {};
template <typename Range>
struct IsSized<TakeView<Range>> : std::true_type {};

namespace ranges {
struct Take {
  template <typename Range>  //
  auto operator()(Range &&range) {
    return TakeView{range, _count};
  }

  explicit Take(std::ptrdiff_t count) : _count(count) {}

  std::ptrdiff_t _count;
};
}  // namespace ranges

template <typename Range>  //
inline auto Take(Range &&range, std::ptrdiff_t count) {
  return ranges::Take{count}(std::forward<Range>(range));
}

// Flatten

template <typename Range>
class FlattenView : private View<Range> {
  template <typename It>  //
  struct FlattenIterator {
    using UnderlyingIt = typename It::value_type::iterator;
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename UnderlyingIt::value_type;
    using difference_type = typename UnderlyingIt::difference_type;
    using reference = typename UnderlyingIt::reference;
    using pointer = typename UnderlyingIt::pointer;

    reference operator*() { return *inner; }

    FlattenIterator &operator++() {
      if (it != end && inner_end == ++inner) {
        ++it;
        Init();
      }
      return *this;
    }

    FlattenIterator operator++(int)  // NOLINT
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    bool operator==(const FlattenIterator &rhs) const {
      return it == rhs.it && (it == end ? true : inner == rhs.inner);
    }
    bool operator!=(const FlattenIterator &rhs) const {
      return !(*this == rhs);
    }
    FlattenIterator(It i, It e) : it(i), end(e) { Init(); }

    void Init() {
      if (it == end) {
        return;
      }
      auto &&temp = *it;
      inner = std::begin(temp);
      inner_end = std::end(temp);
    }

    explicit FlattenIterator(It e) : it(e), end(e) {}

    It it;
    It end;
    UnderlyingIt inner;
    UnderlyingIt inner_end;
  };

 public:
  using iterator = FlattenIterator<GetIter<Range>>;

  iterator begin()  // NOLINT
  {
    return {View<Range>::begin(), View<Range>::end()};
  }

  iterator end()  // NOLINT
  {
    return iterator{View<Range>::end()};
  }

  using View<Range>::size;

  explicit FlattenView(Range &range) : View<Range>(range) {
    static_assert(IsRange<GetValueType<Range>>::value,
                  "Flatten could be used only on nested ranges");
  }
};
template <typename Rng>  //
FlattenView(Rng &&c) -> FlattenView<Rng>;

template <typename Range>
struct IsView<FlattenView<Range>> : std::true_type {};

namespace ranges {
struct Flatten {
  template <typename Range>  //
  auto operator()(Range &&range) {
    return FlattenView{range};
  }
};
}  // namespace ranges

template <typename Range>  //
inline auto Flatten(Range &&range) {
  return ranges::Flatten{}(std::forward<Range>(range));
}

// RepeatView

template <typename Range>
class RepeatView : private SizedView<Range> {
  template <typename It>
  struct RepeatIterator {
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename It::value_type;
    using difference_type = typename It::difference_type;
    using reference = typename It::reference;
    using pointer = typename It::pointer;

    reference operator*() { return *it; }

    RepeatIterator &operator++() {
      ++it;
      if (it == end && --left > 0) {
        it = begin;
      }
      return *this;
    }

    RepeatIterator operator++(int)  // NOLINT
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    bool operator==(const RepeatIterator &rhs) const {
      return left == rhs.left && it == rhs.it;
    }
    bool operator!=(const RepeatIterator &rhs) const { return !(*this == rhs); }

    It it;
    It begin;
    It end;
    int left;
  };

 public:
  using iterator = RepeatIterator<GetIter<Range>>;

  iterator begin()  // NOLINT
  {
    auto &&bgn = SizedView<Range>::begin();
    auto &&end = SizedView<Range>::end();
    int count = bgn == end ? 0 : static_cast<int>(_count);

    return {_cut.value_or(bgn), bgn, end, count};
  }

  iterator end()  // NOLINT
  {
    auto &&bgn = SizedView<Range>::begin();
    auto &&end = SizedView<Range>::end();
    int count = bgn == end ? 0 : (_cut.has_value() ? 1 : 0);

    return {_cut.value_or(end), bgn, end, count};
  }

  typename iterator::difference_type size() const  // NOLINT
  {
    return (_count - (_cut.has_value() ? 1 : 0)) * SizedView<Range>::size();
  }

  explicit RepeatView(Range &range, size_t count,
                      std::optional<GetIter<Range>> cut = std::nullopt)
      : SizedView<Range>(range), _count(count), _cut(cut) {}

 private:
  size_t _count;
  std::optional<GetIter<Range>> _cut;
};

template <typename Rng>  //
RepeatView(Rng &&c) -> RepeatView<Rng>;

template <typename Range>
struct IsView<RepeatView<Range>> : std::true_type {};
template <typename Range>
struct IsSized<RepeatView<Range>> : std::true_type {};

namespace ranges {
struct Repeat {
  template <typename Range>  //
  auto operator()(Range &&range) {
    return RepeatView{range, count};
  }

  explicit Repeat(size_t c) : count(c) {}

  size_t count;
};
}  // namespace ranges

template <typename Range>  //
inline auto Repeat(Range &&range, size_t count,
                   std::optional<GetIter<Range>> cut = std::nullopt) {
  return RepeatView{std::forward<Range>(range), count, cut};
}

// Terminal Combinators

namespace ranges {

namespace detail {
struct CollectGuard {};
}  // namespace detail

template <typename Rng = detail::CollectGuard>
struct Collect {
  template <typename Range>  //
  auto operator()(Range &&input_range) {
    using Container =
        std::conditional_t<std::is_same_v<Rng, detail::CollectGuard>,
                           std::vector<GetValueType<Range>>, std::decay_t<Rng>>;
    Container output_range{};
    if constexpr (std::is_same_v<std::vector<GetValueType<Container>>,
                                 Container>) {
      auto &&size = std::size(input_range);
      assert(size >= 0);
      output_range.reserve(static_cast<size_t>(size));
      this->template Transfer<Container>(
          input_range, std::back_insert_iterator{output_range});
    } else if constexpr (std::is_same_v<std::deque<GetValueType<Container>>,
                                        Container> or
                         std::is_same_v<std::list<GetValueType<Container>>,
                                        Container>) {
      this->template Transfer<Container>(
          input_range, std::back_insert_iterator{output_range});
    } else {
      this->template Transfer<Container>(
          input_range, std::inserter(output_range, std::end(output_range)));
    }
    return output_range;
  }

 private:
  template <typename Dest, typename Src, typename It>  //
  void Transfer(Src &src, It dest) {
    if constexpr (std::is_copy_constructible_v<GetValueType<Dest>>) {
      std::copy(std::begin(src), std::end(src), dest);
    } else {
      std::move(std::begin(src), std::end(src), dest);
    }
  }

  template <typename Dest, typename Src, typename It>  //
  void Transfer(
      Src &src, It dest,
      std::enable_if_t<!std::is_same_v<GetValueType<Src>, GetValueType<Dest>>> =
          {}) {
    if constexpr (std::is_copy_constructible_v<GetValueType<Dest>>) {
      std::transform(std::begin(src), std::end(src), dest, [](auto const &e) {
        return static_cast<GetValueType<Dest>>(e);  //
      });
    } else {
      std::transform(std::begin(src), std::end(src), dest, [](auto &&e) {
        return static_cast<GetValueType<Dest> &&>(
            std::forward<decltype(e)>(e));  //
      });
    }
  }
};

}  // namespace ranges

template <typename OutputRange, typename InputRange>  //
inline OutputRange Collect(InputRange &&input_range) {
  return ranges::Collect<OutputRange>()(std::forward<InputRange>(input_range));
}

namespace ranges {
template <typename Func>
struct ForEach : private FunctorWrapper<Func> {
  template <typename Range>  //
  auto operator()(Range &&input_range) {
    return std::for_each(input_range.begin(), input_range.end(),
                         static_cast<FunctorWrapper<Func> &>(*this));
  }

  explicit ForEach(Func &&c) : FunctorWrapper<Func>(std::move(c)) {}
  explicit ForEach(Func const &c) : FunctorWrapper<Func>(c) {}
};
}  // namespace ranges

template <typename Range, typename Function>  //
inline auto ForEach(Range &&input_range, Function &&cb) {
  return ranges::ForEach(std::forward<Function>(cb))(
      std::forward<Range>(input_range));
}

template <typename Range>  //
inline typename GetIter<Range>::difference_type Len(Range &&range) {
  return std::distance(range.begin(), range.end());
}

template <typename Range>  //
inline typename GetIter<Range>::difference_type LazyLen(Range &&range) {
  return std::size(std::forward<Range>(range));
}

template <typename Range, typename Acc, typename BinOp>  //
inline Acc Fold(Range &&range, Acc acc, BinOp &&op) {
  return std::accumulate(range.begin(), range.end(), acc,
                         std::forward<BinOp>(op));
}

template <typename K, typename V, typename Ktr>  //
auto Find(std::unordered_map<K, V> const &map, Ktr &&key)
    -> GetIter<decltype(map)> {
  return map.find(std::forward<Ktr>(key));
}

template <typename K, typename V, typename Ktr>  //
auto Find(std::map<K, V> const &map, Ktr &&key) -> GetIter<decltype(map)> {
  return map.find(std::forward<Ktr>(key));
}

template <typename K, typename V, typename Ktr>  //
auto Find(std::multimap<K, V> const &map, Ktr &&key) -> GetIter<decltype(map)> {
  return map.find(std::forward<Ktr>(key));
}

template <
    typename Range, typename T,
    typename = std::enable_if_t<std::is_convertible_v<
        std::remove_cv_t<std::remove_reference_t<T>>, GetValueType<Range>>>>
GetIter<Range> Find(Range &&range, T &&val) {
  return std::find(range.begin(), range.end(), val);
}

template <typename Range, typename Pred>  //
GetIter<Range> FindIf(Range &&range, Pred &&pred) {
  return std::find_if(range.begin(), range.end(), std::forward<Pred>(pred));
}

template <typename Range, typename Pred>  //
GetIter<Range> FindFirst(Range &&range, Pred &&pred) {
  auto &&end = std::end(range);
  auto &&it = std::begin(range);
  for (; it != end; ++it) {
    if (std::invoke(std::forward<Pred>(pred), *it)) {
      return it;
    }
  }
  return end;
}

struct First {
  template <typename A, typename B>  //
  A const &operator()(std::pair<A, B> const &val) {
    return val.first;
  }
};

struct Second {
  template <typename A, typename B>  //
  B const &operator()(std::pair<A, B> const &val) {
    return val.second;
  }
};

template <typename Range, typename T, typename Proj>  //
GetIter<Range> Find(Range &&range, T &&val, Proj proj) {
  return FindIf(range, [&](auto &&elem) {  //
    return std::invoke(proj, std::forward<decltype(elem)>(elem)) == val;
  });
}

template <typename Range, typename T>
bool Contains(Range &&range, T &&val) {
  return Find(range, val) != std::end(range);
}

template <typename Range, typename T, typename Proj>
bool Contains(Range &&range, T &&val, Proj proj) {
  return Find(range, std::forward<T>(val), proj) != std::end(range);
}

template <typename Range, typename Cmp = std::less<>>  //
inline GetValueType<Range> Max(Range &&range, Cmp cmp = {}) {
  auto &&bgn = std::begin(range);
  auto &&end = std::end(range);
  for (auto it = bgn; it != end; ++it) {
    if (cmp(*bgn, *it)) {
      bgn = it;
    }
  }
  return *bgn;
}

template <typename Range, typename Cmp = std::less<>>  //
inline GetValueType<Range> Min(Range &&range, Cmp cmp = {}) {
  auto &&bgn = std::begin(range);
  auto &&end = std::end(range);
  for (auto it = bgn; it != end; ++it) {
    if (cmp(*it, *bgn)) {
      bgn = it;
    }
  }
  return *bgn;
}

template <typename RngL, typename RngR>  //
inline bool Equal(RngL &&lhs, RngR &&rhs) {
  return Len(lhs) == Len(rhs) &&  //
         std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename RngL, typename RngR>  //
inline auto Difference(RngL &&lhs, RngR &&rhs) {
  return std::forward<RngL>(lhs)  //
         | ranges::Filter([&](auto &&elem) { return !Contains(rhs, elem); });
}

template <typename Range, typename Comb>  //
inline auto operator|(Range &&rng, Comb comb)
    -> std::enable_if_t<IsView<Range>::value,
                        decltype(comb(std::forward<Range>(rng)))> {
  return comb(std::forward<Range>(rng));
}
