/* Ignores header if not needed after #include */
	#pragma once

	#include <cstdint>			// fixed size integer
	#include <string>			// std::string
	#include <unordered_map>	// hashtable

namespace pje::engine {

	/* LSysGenerator - Generates a word where every character represents a command for the TurtleInterpreter */
	class LSysGenerator {
	public:
		LSysGenerator() = delete;
		/* STANDARD CONSTRUCTOR
		*	L-System := {Alphabet, Axiom, Rules}
		*	>> [INFO]	Only supports 0L and 1L-Systems at the moment
		*	>> [INFO]	Characters without defined rule will persist in new word
		*	>> [INFO]	Generates first m_currentLSysWord automatically
		*/
		LSysGenerator(std::string alphabet, std::string axiom, std::unordered_map<std::string, std::string> rules, uint8_t iterations = 0, std::string envInput = "]");
		~LSysGenerator();
		void generate0LSysWord(std::string axiom, uint8_t iterations);
		void generate1LSysWord(std::string axiom, uint8_t iterations);
		std::string getCurrentLSysWord() const;
		std::string getAlphabet() const;
	private:
		std::string m_currentLSysWord;
		std::string m_alphabet;
		std::unordered_map<std::string, std::string> m_rules;
		std::string m_lEnvInput;
	};
}