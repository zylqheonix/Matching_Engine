#include "order_book.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;

Order make_limit_order(uint64_t id, Side side, uint64_t price, uint64_t quantity) {
  Order o{};
  o.id = id;
  o.type = OrderType::LIMIT;
  o.side = side;
  o.price = price;
  o.quantity = quantity;
  o.timestamp = std::chrono::system_clock::now();
  return o;
}

struct LatencyStats {
  uint64_t p50_ns;
  uint64_t p90_ns;
  uint64_t p99_ns;
  uint64_t p999_ns;
  uint64_t max_ns;
};

uint64_t percentile_value(std::vector<uint64_t> samples, double percentile) {
  if (samples.empty()) {
    return 0;
  }
  const double rank = (percentile / 100.0) * static_cast<double>(samples.size() - 1);
  const size_t idx = static_cast<size_t>(rank);
  std::nth_element(samples.begin(), samples.begin() + static_cast<std::ptrdiff_t>(idx),
                   samples.end());
  return samples[idx];
}

LatencyStats compute_stats(const std::vector<uint64_t> &samples) {
  std::vector<uint64_t> working = samples;
  const uint64_t p50 = percentile_value(working, 50.0);
  const uint64_t p90 = percentile_value(working, 90.0);
  const uint64_t p99 = percentile_value(working, 99.0);
  const uint64_t p999 = percentile_value(working, 99.9);
  const uint64_t max = samples.empty()
                           ? 0
                           : *std::max_element(samples.begin(), samples.end());
  return LatencyStats{p50, p90, p99, p999, max};
}

LatencyStats measure_add_no_match(size_t ops) {
  OrderBook book;
  uint64_t next_id = 1;

  // Seed opposite side so incoming buys remain non-crossing.
  for (size_t i = 0; i < 1024; ++i) {
    book.add_limit_order(make_limit_order(next_id++, Side::SELL, 200, 1));
  }

  std::vector<uint64_t> samples;
  samples.reserve(ops);
  for (size_t i = 0; i < ops; ++i) {
    const uint64_t order_id = next_id++;
    const auto start = Clock::now();
    book.add_limit_order(make_limit_order(order_id, Side::BUY, 100, 1));
    const auto end = Clock::now();
    samples.push_back(
        static_cast<uint64_t>(std::chrono::duration_cast<Nanoseconds>(end - start).count()));

    // Keep state bounded: remove the just-added non-crossing order.
    (void)book.cancel_order(order_id);
  }
  return compute_stats(samples);
}

LatencyStats measure_add_match(size_t ops) {
  OrderBook book;
  uint64_t next_id = 1;

  // Keep a stable resting ask queue so each buy matches immediately.
  for (size_t i = 0; i < ops + 2048; ++i) {
    book.add_limit_order(make_limit_order(next_id++, Side::SELL, 100, 1));
  }

  std::vector<uint64_t> samples;
  samples.reserve(ops);
  for (size_t i = 0; i < ops; ++i) {
    const auto start = Clock::now();
    book.add_limit_order(make_limit_order(next_id++, Side::BUY, 100, 1));
    const auto end = Clock::now();
    samples.push_back(
        static_cast<uint64_t>(std::chrono::duration_cast<Nanoseconds>(end - start).count()));
  }
  return compute_stats(samples);
}

LatencyStats measure_cancel(size_t ops) {
  OrderBook book;
  uint64_t next_id = 1;
  std::vector<uint64_t> ring_ids;
  ring_ids.reserve(4096);

  for (size_t i = 0; i < 4096; ++i) {
    const uint64_t id = next_id++;
    book.add_limit_order(make_limit_order(id, Side::BUY, 100, 1));
    ring_ids.push_back(id);
  }

  std::vector<uint64_t> samples;
  samples.reserve(ops);

  size_t idx = 0;
  for (size_t i = 0; i < ops; ++i) {
    const uint64_t to_cancel = ring_ids[idx];
    const auto start = Clock::now();
    (void)book.cancel_order(to_cancel);
    const auto end = Clock::now();
    samples.push_back(
        static_cast<uint64_t>(std::chrono::duration_cast<Nanoseconds>(end - start).count()));

    const uint64_t replacement = next_id++;
    book.add_limit_order(make_limit_order(replacement, Side::BUY, 100, 1));
    ring_ids[idx] = replacement;
    idx = (idx + 1) % ring_ids.size();
  }
  return compute_stats(samples);
}

void write_markdown(const std::string &path, size_t ops, const LatencyStats &add_no_match,
                    const LatencyStats &add_match, const LatencyStats &cancel) {
  std::ofstream out(path, std::ios::trunc);
  if (!out.is_open()) {
    throw std::runtime_error("failed to open output file: " + path);
  }

  out << "# Order Book Latency Percentiles\n\n";
  out << "- Samples per path: " << ops << "\n";
  out << "- Unit: nanoseconds\n";
  out << "- Build expectation: Release (`-O3 -DNDEBUG`)\n\n";
  out << "| Path | p50 | p90 | p99 | p99.9 | max |\n";
  out << "| --- | ---: | ---: | ---: | ---: | ---: |\n";
  out << "| add (no match) | " << add_no_match.p50_ns << " | " << add_no_match.p90_ns
      << " | " << add_no_match.p99_ns << " | " << add_no_match.p999_ns << " | "
      << add_no_match.max_ns << " |\n";
  out << "| add (matches) | " << add_match.p50_ns << " | " << add_match.p90_ns << " | "
      << add_match.p99_ns << " | " << add_match.p999_ns << " | " << add_match.max_ns
      << " |\n";
  out << "| cancel | " << cancel.p50_ns << " | " << cancel.p90_ns << " | "
      << cancel.p99_ns << " | " << cancel.p999_ns << " | " << cancel.max_ns << " |\n";
}

} // namespace

int main(int argc, char **argv) {
  size_t ops = 1'000'000;
  std::string out_path = "benchmarks/latency_percentiles.md";

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--ops=", 0) == 0) {
      ops = static_cast<size_t>(std::stoull(arg.substr(6)));
    } else if (arg.rfind("--out=", 0) == 0) {
      out_path = arg.substr(6);
    }
  }

  const LatencyStats add_no_match = measure_add_no_match(ops);
  const LatencyStats add_match = measure_add_match(ops);
  const LatencyStats cancel = measure_cancel(ops);

  write_markdown(out_path, ops, add_no_match, add_match, cancel);

  std::cout << "Wrote latency percentiles to " << out_path << '\n';
  return 0;
}
