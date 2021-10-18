#pragma once
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <random>

// Helper functions
namespace Util
{
	// Read file into string
	void read_file(const std::string& filepath, std::string& file_content);

	// Generate random key
	void generate_random_key(uint8_t* public_key, size_t key_length);

	// Convert string of hex chars to ASCII bytes
	void convert_hex_str_to_bytes(const std::string& hex_str, std::vector<uint8_t>& bytes);
};

