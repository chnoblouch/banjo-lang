# Benchmark Results

These results compare the execution time of the same algorithm written in Banjo
and C++. Results are collected averaged over multiple runs using `hyperfine`.
The last row compares Banjo execution time to C++.

This is not a scientific comparison. Its purpose is to get a feel for how fast
Banjo programs run.

# Mandelbrot

| Processor  | Build Mode | Banjo  | C++    | Comparison   |
|------------|------------|--------|--------|--------------|
| Apple M3   | Debug      | 1.267s | 1.667s | 1.32x faster |
| Apple M3   | Optimized  | 0.530s | 0.494s | 0.93x slower |
