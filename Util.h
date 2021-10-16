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
};

