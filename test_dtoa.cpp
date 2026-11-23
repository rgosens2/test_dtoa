#include <charconv>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <system_error>
#include <chrono>
#include <cstdio>

#include "zmij/zmij.h"
#include "xjb/xjb64.h"


#include <cmath>
#include <limits>

double values[] = {
    0.0, -0.0,
    0.1, 1.0 / 3.0,
    1e-6, 1e-7, 1e20, 1e21, // notation boundaries
    std::nextafter(1.0, 0.0),
    std::nextafter(1.0, 2.0), // adjacent doubles around 1
    std::numeric_limits<double>::denorm_min(), // smallest subnormal
    std::numeric_limits<double>::min(),        // smallest normal
    std::numeric_limits<double>::max(),
    -std::numeric_limits<double>::max(),
    M_PI,
    42.0, 1000.0,
    std::numeric_limits<double>::infinity(),
    -std::numeric_limits<double>::infinity(),
    std::numeric_limits<double>::quiet_NaN(),
    -std::numeric_limits<double>::quiet_NaN(),
};


struct perf {
    std::chrono::steady_clock::time_point start_;
    perf() : start_(std::chrono::steady_clock::now()) {}
    double elapsed() const {
        auto stop = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = stop - start_;
        return elapsed_seconds.count();
    }
};


void show_to_chars(auto... format_args)
{
    const size_t buf_size = 10;
    char buf[buf_size]{};
    std::to_chars_result result = std::to_chars(buf, buf + buf_size, format_args...);

    if (result.ec != std::errc())
        std::cout << std::make_error_code(result.ec).message() << '\n';
    else
    {
        std::string_view str(buf, result.ptr - buf);
        std::cout << std::quoted(str) << '\n';
    }
}

int main()
{
    //printf("to_chars:\t");
    //show_to_chars(42);
    // Not in Clang 14:
    //show_to_chars(+3.14159F);
    //show_to_chars(-3.14159, std::chars_format::fixed);
    //show_to_chars(-3.14159, std::chars_format::scientific, 3);
    //show_to_chars(3.1415926535, std::chars_format::fixed, 10);

    // Output:
    // rg@MacBook-Air-M2 test_dtoa % ./test_dtoa
    // Starting performance test with 1000000000 runs...
    // zmij:	"3.14159"	6.20 ns
    // xjb:	    "3.14159"	6.54 ns

    // NOTE: we need high num_runs otherwise timings will not be correct and
    // we might get different timings in Debugger and Terminal.
    // Also need to compile with -O3 to get the  ~6 ns as reported in:
    // https://github.com/fmtlib/dtoa-benchmark
    // Here on M2, zmij is consistently faster. On M1 with dtoa-benchmark we 
    // found that xjb was slightly faster.

    // Check
    printf("Checking correctness for %zu values...\n", std::size(values));
    for (double v : values) {
        char buf1[zmij::double_buffer_size]{};
        char* end1 = zmij::write(buf1, sizeof buf1, v);
        std::string_view str1(buf1, end1 - buf1);

        char buf2[zmij::double_buffer_size]{};
        char* end2 = xjb64(v, buf2);
        std::string_view str2(buf2, end2 - buf2);

        // if (str1 != str2) {
        //     std::cout << "Mismatch for value: " << v << '\n';
        //     std::cout << "zmij: " << std::quoted(str1) << '\n';
        //     std::cout << "xjb:  " << std::quoted(str2) << '\n';
        // }

        // NOTE: A double has about 15–17 decimal digits of precision.
        // The C standard’s DBL_DECIMAL_DIG gives the number of significant
        // decimal digits needed to print a double so that reading it back
        // recovers the exact original value. For typical IEEE-754 doubles,
        // that value is 17.
        // NOTE: Why 0 vs 0.0 Differs
        // zmij (0 and -0): Follows the C++17 std::to_chars shortest-representation
        // convention (as well as ECMAScript/JSON JSON.stringify rules), which
        // omits a trailing .0 when there are no fractional digits needed to
        // distinguish the value. (printf %.17g does the same).
        // xjb (0.0 and -0.0): Follows the Python / Rust / yyjson convention, which
        // explicitly appends .0 to integral floating-point values so a reader or
        // downstream parser immediately knows the token represents a floating-point
        // number rather than an integer.
        static int i = 1;
        printf("v%d:\t%.17g\nzmij:\t%.*s\nxjb:\t%.*s\n\n", i++, v, static_cast<int>(end1 - buf1), buf1, static_cast<int>(end2 - buf2), buf2);
    }
    ///////////////

    uint num_runs = 1'000'000'000;

    printf("Starting performance test with %d runs...\n", num_runs);

    // zmij
    printf("zmij:\t");
    char buf[zmij::double_buffer_size]{};

    perf p1;
    char* end = nullptr;
    for (uint i = 0; i < num_runs; ++i) {
        //end = zmij::write(buf, sizeof buf, 3.14159e00);
        end = zmij::write(buf, sizeof buf, values[i%std::size(values)]);
    }
    //char* end = zmij::write(buf, sizeof buf, 3.14159e00);
    double elapsed1 = p1.elapsed()/num_runs;

    //std::string_view str(buf, end - buf);
    //std::cout << std::quoted(str) << '\t';
    //printf("%.*s\n", static_cast<int>(end - buf), buf);
    printf("%.2f ns\n", elapsed1*1e9);


    // xjb
    printf("xjb:\t");
    char buf2[zmij::double_buffer_size]{};

    perf p2;
    char* end2 = nullptr;
    for (uint i = 0; i < num_runs; ++i) {
        //end2 = xjb64(3.14159e00, buf2);
        end2 = xjb64(values[i%std::size(values)], buf2);
    }
    double elapsed2 = p2.elapsed()/num_runs;

    //std::string_view str2(buf2, end2 - buf2);
    //std::cout << std::quoted(str2) << '\t';
    //printf("%.*s\n", static_cast<int>(end2 - buf2), buf2);
    printf("%.2f ns\n", elapsed2*1e9);


    // null
    printf("null:\t");

    perf p0;
    for (uint i = 0; i < num_runs; ++i) {
        // NOP
        __asm__ volatile("nop");
    }
    double elapsed0 = p0.elapsed()/num_runs;

    printf("%.2f ns\n", elapsed0*1e9);
}