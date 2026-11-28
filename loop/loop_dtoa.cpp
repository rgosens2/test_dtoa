#include <bit>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "../zmij/zmij.h"

int main() {
    char buf1[zmij::double_buffer_size];

    // Prints the bit pattern in hex and the double
    // NOTE: the highest 3 nibbles contain the sign and exponent (1+11=12 bits)
    // The last 13 nibbles are the fraction (13x4=52 bits).
    // NOTE: print niet meer dan 1000 anders hangt de integrated terminal.

    // Print +0 and then from the smallest positive subnormal upwards.
    //
    // The first positive double you print after +0.0 (bits = 0x0000000000000001ULL,
    // value $2^{-1074} \approx 4.9406564584124654 \times 10^{-324}$) is the smallest
    // positive double (std::numeric_limits<double>::denorm_min())
    //
    // All of them are subnormals: In IEEE 754 binary64, a double is subnormal
    // whenever its 11-bit biased exponent field is 0 (bits < (1ULL << 52), i.e.,
    // bits < 4,503,599,627,370,496). The first 1,000,000 bit patterns (0 through
    // 999,999) all have exponent 0 and significands 0 through 999,999.
    for (uint64_t bits = 0; bits < 1000; ++bits) {
        //double v = std::bit_cast<double>(bits); // no bit_cast in clang14
        double v;
        std::memcpy(&v, &bits, sizeof v);
        char* end1 = zmij::write(buf1, sizeof buf1, v);
        
        printf("0x%016" PRIX64 "\t", bits);
        printf("%.*s\n", static_cast<int>(end1 - buf1), buf1);
    }

    printf("...\n");

    // Print highest normals
    //
    // Biased exponent (11 bits): 0x7FE (2046)
    // Is the largest exponent for finite positive binary64 numbers (0x7FF is reserved
    // for $\pm\infty$ and NaN).
    //
    // The highest normal positive double (DBL_MAX, std::numeric_limits<double>::max())
    // is:
    // Value: $(2 - 2^{-52}) \times 2^{1023} \approx 1.7976931348623157 \times 10^{308}$
    // Hex bit pattern: 0x7FEFFFFFFFFFFFFF
    constexpr uint64_t end_bits = 0x7FEFFFFFFFFFFFFF;
    constexpr uint64_t start_bits = end_bits - 1000;

    for (uint64_t bits = start_bits; bits <= end_bits; ++bits) {
        //double v = std::bit_cast<double>(bits); // no bit_cast in clang14
        double v;
        std::memcpy(&v, &bits, sizeof v);
        char* end1 = zmij::write(buf1, sizeof buf1, v);
        
        printf("0x%016" PRIX64 "\t", bits);
        printf("%.*s\n", static_cast<int>(end1 - buf1), buf1);
    }

    // TEST:
    // print infinity
    double v = std::numeric_limits<double>::infinity();
    uint64_t bits;
    std::memcpy(&bits, &v, sizeof v);
    char* end1 = zmij::write(buf1, sizeof buf1, v);

    printf("0x%016" PRIX64 "\t", bits);
    printf("%.*s\n", static_cast<int>(end1 - buf1), buf1);

    // print nan
    v = std::numeric_limits<double>::quiet_NaN();
    std::memcpy(&bits, &v, sizeof v);
    end1 = zmij::write(buf1, sizeof buf1, v);

    printf("0x%016" PRIX64 "\t", bits);
    printf("%.*s\n", static_cast<int>(end1 - buf1), buf1);
}