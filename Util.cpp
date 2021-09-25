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
