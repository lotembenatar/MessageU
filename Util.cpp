#include "Util.h"

bool Util::read_file(const std::string& filepath, std::string& file_content)
{
    std::ifstream file_stream(filepath);
    if (file_stream.is_open())
    {
        std::stringstream read_buffer;
        read_buffer << file_stream.rdbuf();

        // copy to returned string
        file_content = read_buffer.str();

        // close stream
        file_stream.close();
        return true;
    }
    return false;
}

void Util::convert_hex_str_to_bytes(const std::string& hex_str, std::vector<uint8_t>& bytes)
{
    for (uint32_t i = 0; i < hex_str.length(); i += 2) {
        std::string byte = hex_str.substr(i, 2);
        uint8_t byte_as_ascii = static_cast<uint8_t>(std::strtoul(byte.c_str(), NULL, 16));
        bytes.push_back(byte_as_ascii);
    }
}
