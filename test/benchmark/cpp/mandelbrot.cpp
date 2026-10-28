// Thanks to:
//   - https://en.wikipedia.org/wiki/Mandelbrot_set
//   - https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/mandelbrot-node-1.html

#include <cstdint>
#include <iostream>
#include <vector>

static constexpr unsigned SIZE = 2048;
static constexpr unsigned MAX_ITERATIONS = 100;

int main(int /* argc */, const char ** /* argv */) {
    std::vector<std::uint8_t> buffer;

    for (unsigned py = 0; py < SIZE; py++) {
        for (unsigned px = 0; px < SIZE; px++) {
            float zr = 0.0f;
            float zi = 0.0f;
            float cr = 2.0f * static_cast<float>(px) / static_cast<float>(SIZE) - 1.5f;
            float ci = 2.0f * static_cast<float>(py) / static_cast<float>(SIZE) - 1.0f;

            unsigned num_iterations = 0;

            for (unsigned i = 0; i < MAX_ITERATIONS; i++) {
                float tr = zr * zr - zi * zi + cr;
                float ti = 2.0 * zr * zi + ci;
                zi = ti;
                zr = tr;

                if (zr * zr + zi * zi > 4.0) {
                    break;
                }

                num_iterations += 1;
            }

            float value = static_cast<float>(num_iterations) / static_cast<float>(MAX_ITERATIONS);
            std::uint8_t color = static_cast<std::uint8_t>(255.0 * value);

            buffer.push_back(color);
            buffer.push_back(color);
            buffer.push_back(color);
        }
    }

    std::cout << "P6\n" << SIZE << " " << SIZE << "\n255\n";
    std::cout.write(reinterpret_cast<char *>(buffer.data()), buffer.size());

    return 0;
}
