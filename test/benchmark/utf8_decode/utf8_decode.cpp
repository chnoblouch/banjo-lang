#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

int main(int /* argc */, const char ** /* argv */) {
    std::ifstream file{"input.txt", std::ios::binary};
    file.seekg(0, std::ios::end);
    std::size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::uint8_t> input;
    input.resize(size);
    file.read(reinterpret_cast<char *>(input.data()), input.size());

    std::vector<std::uint32_t> buffer;

    for (unsigned i = 0; i < input.size(); i++) {
        std::uint8_t c = input.at(i);

        if ((c & 0x80) == 0) {
            buffer.push_back(static_cast<std::uint32_t>(c));
        } else if ((c & 0xE0) == 0xC0) {
            std::uint32_t c1 = static_cast<std::uint32_t>(c);
            std::uint32_t c2 = static_cast<std::uint32_t>(input.at(i + 1));
            std::uint32_t value = ((c1 & 0x1F) << 6) | (c2 & 0x3F);

            buffer.push_back(value);
            i += 1;
        } else if ((c & 0xF0) == 0xE0) {
            std::uint32_t c1 = static_cast<std::uint32_t>(c);
            std::uint32_t c2 = static_cast<std::uint32_t>(input.at(i + 1));
            std::uint32_t c3 = static_cast<std::uint32_t>(input.at(i + 2));
            std::uint32_t value = ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);

            buffer.push_back(value);
            i += 2;
        } else {
            std::uint32_t c1 = static_cast<std::uint32_t>(c);
            std::uint32_t c2 = static_cast<std::uint32_t>(input.at(i + 1));
            std::uint32_t c3 = static_cast<std::uint32_t>(input.at(i + 2));
            std::uint32_t c4 = static_cast<std::uint32_t>(input.at(i + 3));
            std::uint32_t value = ((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);

            buffer.push_back(value);
            i += 3;
        }
    }

    std::cout.write(reinterpret_cast<const char *>(buffer.data()), sizeof(std::uint32_t) * buffer.size());
}
