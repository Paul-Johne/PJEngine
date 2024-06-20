/* Ignores header if not needed after #include */
	#pragma once

/* Third Party Files */
	#include <cstdint>			// fixed size integer
	#include <string>			// std::string
	#include <unordered_map>	// hashtable

	#include <iostream>			// i/o stream
	#include <regex>			// regex tools

namespace pje::engine {

	/* ArgsParser - Parses user input and populates config data for current process */
	class ArgsParser {
	public:
		/* members with default values */
		uint8_t			m_amountOfObjects		= 1;
		uint8_t			m_complexityOfObjects	= 0;
		uint16_t		m_width					= 100;
		uint16_t		m_height				= 100;
		bool			m_vsync					= 0;
		std::string		m_graphicsAPI			= "";

		ArgsParser() = delete;
		/* STANDARD CONSTRUCTOR 
		*	program expects max. 6 arguments for its members :
		*		*.exe --a=<objectAmount> --c=<objectComplexity> --w=<windowWidth> --h=<windowHeight> --vsync=<0||1> --env=<vulkan/opengl>
		*/
		ArgsParser(int argc, char** arcv, uint8_t valid_argc = 7);
		~ArgsParser();

	private:
		/* return value for switch case evaluation in ArgsParser(int, char**, uint8_t) */
		int mapArgToInt(std::unordered_map<std::string, int> map, std::string arg);
	};
}