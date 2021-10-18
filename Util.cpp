#include "Util.h"

void Util::read_file(const std::string& filepath, std::string& file_content)
{
    std::ifstream file_stream(filepath);
    std::stringstream read_buffer;
    read_buffer << file_stream.rdbuf();

    // copy to returned string
    file_content = read_buffer.str();

    // close stream
    file_stream.close();
}

void Util::generate_random_key(uint8_t* public_key, size_t key_length)
{
    // Use strong random number generator
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<> dist(0, 0xff);

    // Generate random bytes
    for (uint32_t i = 0; i < key_length; i++)
    {
        public_key[i] = static_cast<uint8_t>(dist(gen));
    }
}

void Util::convert_hex_str_to_bytes(const std::string& hex_str, std::vector<uint8_t>& bytes)
{
    for (uint32_t i = 0; i < hex_str.length(); i += 2) {
        std::string byte = hex_str.substr(i, 2);
        uint8_t byte_as_ascii = static_cast<uint8_t>(std::strtoul(byte.c_str(), NULL, 16));
        bytes.push_back(byte_as_ascii);
    }
}
