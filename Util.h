#pragma once
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

// Helper functions
namespace Util
{
	// Read file into string
	bool read_file(const std::string& filepath, std::string& file_content);

	// Convert string of hex chars to ASCII bytes
	void convert_hex_str_to_bytes(const std::string& hex_str, std::vector<uint8_t>& bytes);
};

