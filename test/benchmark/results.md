# Benchmark Results

These results compare the execution time of the same algorithm written in Banjo
and C++. The C++ compiler is Clang on all platforms. Results are collected and
averaged over multiple runs using `hyperfine`. The last column shows how much
faster/slower the Banjo code ran compared to C++.

This is not a scientific comparison. Its purpose is to get a feel for how fast
Banjo programs run.

Benchmarks are compiled and run with the following configurations:

- Debug: `banjo --opt-level 2` / `clang++ -O3`
- Optimized: `banjo --opt-level 0` / `clang++ -O0`

## Mandelbrot

| Processor              | Architecture | OS    | Configuration | Banjo  | C++    | Comparison     |
|------------------------|--------------|-------|---------------|--------|--------|----------------|
| AMD Ryzen 7 PRO 7840HS | x86-64       | Linux | Debug         | 1.242s | 1.885s | 1.52x as fast  |
| AMD Ryzen 7 PRO 7840HS | x86-64       | Linux | Optimized     | 0.407s | 0.509s | 1.25x as fast  |
| Apple M3               | AArch64      | macOS | Debug         | 1.267s | 1.667s | 1.32x as fast  |
| Apple M3               | AArch64      | macOS | Optimized     | 0.530s | 0.494s | 0.93x as fast  |
