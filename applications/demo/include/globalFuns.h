/* Ignores header if not needed after #include */
	#pragma once

	#include <iostream>
	#include <fstream>

	#include <string>
	#include <vector>

std::vector<char> readSpirvFile(const std::string& filename);