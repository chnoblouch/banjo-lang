# Benchmark Results

These results compare the execution time of the same algorithm written in Banjo
and C++. The C++ compiler is Clang on all platforms. Results are collected and
averaged over multiple runs using `hyperfine`. The last column shows how much
faster/slower the Banjo code ran relative to C++.

This is not a scientific comparison. Its purpose is to get a feel for how fast
Banjo programs run.

Benchmarks are compiled and run with the following configurations:

- Debug: `banjo --opt-level 0` / `clang++ -O0`
- Optimized: `banjo --opt-level 2` / `clang++ -O3`

In general, Banjo beats C++ in debug builds (probably because its standard
library relies less on inlining). C++ wins almost always in optimized builds,
especially if number crunching is involved, although there are cases where it
gets pretty close.

## Mandelbrot

| Processor              | Architecture | OS    | Configuration | Banjo  | C++    | Comparison     |
|------------------------|--------------|-------|---------------|--------|--------|----------------|
| AMD Ryzen 7 PRO 7840HS | x86-64       | Linux | Debug         | 1.232s | 1.370s | 1.11x as fast  |
| AMD Ryzen 7 PRO 7840HS | x86-64       | Linux | Optimized     | 0.410s | 0.386s | 0.94x as fast  |
| Apple M3               | AArch64      | macOS | Debug         | 1.156s | 1.398s | 1.21x as fast  |
| Apple M3               | AArch64      | macOS | Optimized     | 0.501s | 0.339s | 0.68x as fast  |

## Primes

| Processor              | Architecture | OS    | Configuration | Banjo  | C++    | Comparison     |
|------------------------|--------------|-------|---------------|--------|--------|----------------|
| AMD Ryzen 7 PRO 7840HS | x86-64       | Linux | Debug         | 1.399s | 10.37s | 7.41x as fast  |
| AMD Ryzen 7 PRO 7840HS | x86-64       | Linux | Optimized     | 0.518s | 0.258s | 0.50x as fast  |
| Apple M3               | AArch64      | macOS | Debug         | 1.107s | 2.951s | 2.67x as fast  |
| Apple M3               | AArch64      | macOS | Optimized     | 0.465s | 0.305s | 0.66x as fast  |

## UTF-8 Decode

| Processor              | Architecture | OS    | Configuration | Banjo   | C++     | Comparison     |
|------------------------|--------------|-------|---------------|---------|---------|----------------|
| AMD Ryzen 7 PRO 7840HS | x86-64       | Linux | Debug         |  65.7ms | 170.2ms | 2.59x as fast  |
| AMD Ryzen 7 PRO 7840HS | x86-64       | Linux | Optimized     |  34.1ms |  35.9ms | 1.05x as fast  |
| Apple M3               | AArch64      | macOS | Debug         |  57.5ms | 204.6ms | 3.56x as fast  |
| Apple M3               | AArch64      | macOS | Optimized     |  20.2ms |  12.7ms | 0.63x as fast  |
