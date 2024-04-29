#include "argsParser.h"

pje::engine::ArgsParser::ArgsParser(int argc, char** argv, uint8_t valid_argc) {
	if (argc > valid_argc)
		throw std::runtime_error("Too many arguments were given by the user!");

	/* pattern: --<argName >= <argValue> */
	std::regex argsPattern("--(.*)=(.*)");

	/* map for regex evaluation */
	std::unordered_map<std::string, int> validLiterals{
		{"a", 0}, {"c", 1}, {"w", 2}, {"h", 3}, {"vsync", 4}, {"env", 5}
	};

	for (uint8_t i = 1; i < argc; i++) {
		std::string currentArg = argv[i];
		std::smatch smatch;

		if (std::regex_match(currentArg, smatch, argsPattern)) {
			/* smatch holds array with 3 elements : <pattern> <argName> <argValue> */
			switch (mapArgToInt(validLiterals, smatch[1])) {
			/* 0 = > objectAmount */
			case 0:
				this->m_amountOfObjects = std::stoi(smatch[2]);
				break;
			/* 1 = > objectComplexity */
			case 1:
				this->m_complexityOfObjects = std::stoi(smatch[2]);
				break;
			/* 2 => windowWidth */
			case 2:
				this->m_width = std::stoi(smatch[2]);
				break;
			/* 3 => windowHeight */
			case 3:
				this->m_height = std::stoi(smatch[2]);
				break;
			/* 4 => vsync */
			case 4:
				this->m_vsync = std::stoi(smatch[2]);
				break;
			/* 5 => env */
			case 5:
				this->m_graphicsAPI = smatch[2];
				break;
			/* invalid argument */
			default:
				std::cout << "[PJE] \tInvalid argument was found.\n";
			}
		}
	}
	std::cout << "[PJE] \tAll correct args were evaluated." << std::endl;
}

pje::engine::ArgsParser::~ArgsParser() {}

int pje::engine::ArgsParser::mapArgToInt(std::unordered_map<std::string, int> map, std::string arg) {
	auto pair = map.find(arg);
	if (pair == map.end()) {
		/* given argument is unknown to the program */
		return -1;
	}
	else {
		/* returns valid literal for upcoming switch case */
		return pair->second;
	}
}