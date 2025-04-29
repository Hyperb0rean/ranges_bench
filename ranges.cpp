#include <algorithm>
#include <cstdlib>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <ranges>
#include "ranges.h"

// #pragma GCC target("avx512f")
// #include <x86intrin.h>

static std::mt19937 gen{42};

std::vector<int64_t> generate(size_t size) {
  std::vector<int64_t> vec(size);
  std::uniform_int_distribution<int64_t> dist(
      0, std::numeric_limits<int64_t>::max());
  std::generate(vec.begin(), vec.end(), [&]() { return dist(gen); });
  return vec;
}

enum FilterCollect { FCStl, FCStorm };

static void BM_FilterCollect(benchmark::State& state) {
  size_t size = state.range(0);
  auto data = generate(size);

  for (auto _ : state) {
    switch (state.range(1)) {
      case FCStorm:
        benchmark::DoNotOptimize(
            View{data}                                                 //
            | ranges::Filter([](auto&& num) { return num % 2 == 0; })  //
            | ranges::Collect<>{});
        break;

      case FCStl:
        benchmark::DoNotOptimize(
            data                                                           //
            | std::views::filter([](auto&& num) { return num % 2 == 0; })  //
            | std::ranges::to<std::vector<int64_t>>());

        break;
    }
  }
}

enum FilterMapCollect { FMCStl, FMCStormOptimized, FMCStorm };

static void BM_FilterMapCollect(benchmark::State& state) {
  size_t size = state.range(0);
  auto data = generate(size);

  for (auto _ : state) {
    switch (state.range(1)) {
      case FMCStorm:
        benchmark::DoNotOptimize(
            View{data}                                                     //
            | ranges::Filter([](auto&& num) { return num % 2 == 0; })      //
            | ranges::Map([](auto&& num) { return std::to_string(num); })  //
            | ranges::Collect<>{});
        break;
      case FMCStormOptimized:
        benchmark::DoNotOptimize(
            View{data}  //
            | ranges::FilterMap([](auto&& num) -> std::optional<std::string> {
                if (num % 2 == 0) {
                  return std::to_string(num);
                }
                return std::nullopt;
              })  //
            | ranges::Collect<>{});
        break;
      case FMCStl:
        benchmark::DoNotOptimize(
            data                                                            //
            | std::views::filter([](int64_t num) { return num % 2 == 0; })  //
            | std::views::transform(
                  [](auto&& num) { return std::to_string(num); })  //
            | std::ranges::to<std::vector<std::string>>());

        break;
    }
  }
}

static void BM_MapFilterCollect(benchmark::State& state) {
  size_t size = state.range(0);
  auto data = generate(size);

  for (auto _ : state) {
    switch (state.range(1)) {
      case FMCStorm:
        benchmark::DoNotOptimize(
            View{data}                                                        //
            | ranges::Map([](auto&& num) { return std::to_string(num); })     //
            | ranges::Filter([](auto&& num) { return num.size() % 2 == 0; })  //
            | ranges::Collect<>{});
        break;
      case FMCStormOptimized:
        benchmark::DoNotOptimize(
            View{data}  //
            | ranges::FilterMap([](auto&& num) -> std::optional<std::string> {
                auto str = std::to_string(num);
                if (str.size() % 2 == 0) {
                  return std::move(str);
                }
                return std::nullopt;
              })  //
            | ranges::Collect<>{});
        break;
      case FMCStl:
        benchmark::DoNotOptimize(data  //
                                 | std::views::transform([](auto&& num) {
                                     return std::to_string(num);
                                   })  //
                                 | std::views::filter([](auto&& num) {
                                     return num.size() % 2 == 0;
                                   })  //
                                 | std::ranges::to<std::vector<std::string>>());

        break;
    }
  }
}

BENCHMARK(BM_FilterCollect)
    ->Args({100, FCStl})
    ->Args({100, FCStorm})  //
    ->Args({1000, FCStl})
    ->Args({1000, FCStorm})  //
    ->Args({10000, FCStl})
    ->Args({10000, FCStorm})
    ->Args({100000, FCStl})
    ->Args({100000, FCStorm})  //
    ->Args({1000000, FCStl})
    ->Args({1000000, FCStorm})  //
    ->Args({100000000, FCStl})
    ->Args({100000000, FCStorm})  //
    ;

BENCHMARK(BM_FilterMapCollect)
    ->Args({100, FMCStl})
    ->Args({100, FMCStormOptimized})
    ->Args({100, FMCStorm})  //
    ->Args({1000, FMCStl})
    ->Args({1000, FMCStormOptimized})
    ->Args({1000, FMCStorm})  //
    ->Args({10000, FMCStl})
    ->Args({10000, FMCStormOptimized})
    ->Args({10000, FMCStorm})
    ->Args({100000, FMCStl})
    ->Args({100000, FMCStormOptimized})
    ->Args({100000, FMCStorm})  //
    ->Args({1000000, FMCStl})
    ->Args({1000000, FMCStormOptimized})
    ->Args({1000000, FMCStorm})  //
    ->Args({100000000, FMCStl})
    ->Args({100000000, FMCStormOptimized})
    ->Args({100000000, FMCStorm})  //
    ;

BENCHMARK(BM_MapFilterCollect)
    ->Args({100, FMCStl})
    ->Args({100, FMCStormOptimized})
    ->Args({100, FMCStorm})  //
    ->Args({1000, FMCStl})
    ->Args({1000, FMCStormOptimized})
    ->Args({1000, FMCStorm})  //
    ->Args({10000, FMCStl})
    ->Args({10000, FMCStormOptimized})
    ->Args({10000, FMCStorm})
    ->Args({100000, FMCStl})
    ->Args({100000, FMCStormOptimized})
    ->Args({100000, FMCStorm})  //
    ->Args({1000000, FMCStl})
    ->Args({1000000, FMCStormOptimized})
    ->Args({1000000, FMCStorm})  //
    ->Args({100000000, FMCStl})
    ->Args({100000000, FMCStormOptimized})
    ->Args({100000000, FMCStorm})  //
    ;
BENCHMARK_MAIN();
