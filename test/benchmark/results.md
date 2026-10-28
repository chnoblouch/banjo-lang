# Benchmark Results

These results compare the execution time of the same algorithm written in Banjo
and C++. Results are collected and averaged over multiple runs using
`hyperfine`. The last column compares Banjo execution time to C++.

This is not a scientific comparison. Its purpose is to get a feel for how fast
Banjo programs run.

## Mandelbrot

| Architecture | Processor              | Build Mode | Banjo  | C++    | Comparison     |
|--------------|------------------------|------------|--------|--------|----------------|
| x86-64       | AMD Ryzen 7 PRO 7840HS | Debug      | 1.242s | 1.885s | 1.52x as fast  |
| x86-64       | AMD Ryzen 7 PRO 7840HS | Optimized  | 0.407s | 0.509s | 1.25x as fast  |
| AArch64      | Apple M3               | Debug      | 1.267s | 1.667s | 1.32x as fast  |
| AArch64      | Apple M3               | Optimized  | 0.530s | 0.494s | 0.93x as fast  |
