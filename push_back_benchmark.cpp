#include <type_traits>
#include <vector>
#include <chrono>
#include <iostream>
#include <cstdlib>

typedef std::conditional<std::chrono::high_resolution_clock::is_steady,
	                 std::chrono::high_resolution_clock,
			 std::chrono::steady_clock>::type benchmark_clock;

void push_back_benchmark(unsigned L) {
    auto start = benchmark_clock::now();
    std::vector<unsigned> v;
    for (unsigned i = 0; i < L; i++) {
         v.push_back(i);
    }
    auto end = benchmark_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << L << " .push_back() in " << elapsed << " usec" << std::endl;
}

int main(int argc, char** argv) {
    unsigned L = 0;
    if (argc >= 2) {
        L = std::atoi(argv[1]);
    }
    if (0 == L) {
        L = 1U << 20;
    }
    std::cout << "Using " <<
        (std::chrono::high_resolution_clock::is_steady ? "high_resolution_clock" : "steady_clock")
	<< ", resolution: "
        << benchmark_clock::period::num << '/' << benchmark_clock::period::den
	<< " sec" << std::endl;
    push_back_benchmark(L);
    return 0;
}
