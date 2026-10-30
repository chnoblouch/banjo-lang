# Benchmark Results

These results compare the execution time of the same algorithm written in Banjo
and C++. The C++ compiler is Clang on all platforms. Results are collected and
averaged over multiple runs using `hyperfine`. The last column shows how much
faster/slower the Banjo code ran compared to C++.

This is not a scientific comparison. Its purpose is to get a feel for how fast
Banjo programs run.

Benchmarks are compiled and run with the following configurations:

- Debug: `banjo --opt-level 0` / `clang++ -O0`
- Optimized: `banjo --opt-level 2` / `clang++ -O3`

## Mandelbrot

| Processor              | Architecture | OS    | Configuration | Banjo  | C++    | Comparison     |
|------------------------|--------------|-------|---------------|--------|--------|----------------|
| AMD Ryzen 7 PRO 7840HS | x86-64       | Linux | Debug         | 1.232s | 1.370s | 1.11x as fast  |
| AMD Ryzen 7 PRO 7840HS | x86-64       | Linux | Optimized     | 0.410s | 0.386s | 0.94x as fast  |
| Apple M3               | AArch64      | macOS | Debug         | 1.156s | 1.398s | 1.21x as fast  |
| Apple M3               | AArch64      | macOS | Optimized     | 0.501s | 0.339s | 0.68x as fast  |
