// Thanks to:
//   - https://en.wikipedia.org/wiki/Sieve_of_Eratosthenes
//   - https://github.com/smarr/are-we-fast-yet/blob/master/benchmarks/JavaScript/sieve.js

#include <iostream>
#include <vector>

static constexpr unsigned LIMIT = 50000000;

int main(int /* argc */, const char ** /* argv */) {
    std::vector<bool> flags;
    flags.assign(LIMIT + 1, true);

    unsigned num_primes = 0;

    for (unsigned i = 2; i <= LIMIT; i++) {
        if (!flags.at(i)) {
            continue;
        }

        for (unsigned p = 2 * i; p <= LIMIT; p++) {
            flags.at(p) = false;
            p += i;
        }

        num_primes += 1;
    }

    std::cout << num_primes << '\n';
}
