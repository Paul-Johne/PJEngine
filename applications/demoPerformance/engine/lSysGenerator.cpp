#include "lSysGenerator.h"

pje::engine::LSysGenerator::LSysGenerator(std::string alphabet, 
										  std::string axiom, 
										  std::unordered_map<std::string, std::string> rules, 
										  uint8_t iterations, 
										  std::string envInput) : m_currentLSysWord(""), m_alphabet(alphabet), m_rules(rules) {
	if (envInput.size() == 0) {
		/* generates this->m_currentLSysWord */
		generate0LSysWord(axiom, iterations);
	}
	else {
		/* sets environmental input for 1L-Systems and 2L-Systems */
		this->m_lEnvInput = envInput.at(0);

		/* generates this->m_currentLSysWord */
		generate1LSysWord(axiom, iterations);
	}
}

pje::engine::LSysGenerator::~LSysGenerator() {}

void pje::engine::LSysGenerator::generate0LSysWord(std::string word, uint8_t iterations) {
	this->m_currentLSysWord.clear();

	if (iterations == 0) {
		this->m_currentLSysWord.append(word);
		return;
	}

	for (std::string::size_type i = 0; i <= word.size(); i++) {
		auto pair = this->m_rules.find(word.substr(i, 1));
		if (pair != this->m_rules.end()) {
			this->m_currentLSysWord.append(pair->second);
		}
		else {
			this->m_currentLSysWord.append(1, word.at(i));
		}
	}
}

void pje::engine::LSysGenerator::generate1LSysWord(std::string word, uint8_t iterations) {
	/* clears member for next current iteration */
	this->m_currentLSysWord.clear();
	
	/* recursion base */
	if (iterations == 0) {
		this->m_currentLSysWord.append(word);
		return;
	}
	
	/* first character + environmental input for rule lookup */
	this->m_currentLSysWord.append(
		this->m_rules.find(this->m_lEnvInput + word.at(0))->second
	);

	/*
	*	- populates string of 'current iteration' with remaining characters of 'word' in pairs of two characters
	*	- appends rule.value || if (no rule was found) => appends current character
	*/
	for (std::string::size_type i = 1; i < word.size(); i++) {
		auto pair = this->m_rules.find(word.substr(i-1, 2));
		if (pair != this->m_rules.end()) {
			this->m_currentLSysWord.append(pair->second);
		}
		else {
			this->m_currentLSysWord.append(1, word.at(i));
		}
	}

	/* recursion step */
	generate1LSysWord(this->m_currentLSysWord, iterations-1);
}

std::string pje::engine::LSysGenerator::getCurrentLSysWord() const { return m_currentLSysWord; }
std::string pje::engine::LSysGenerator::getAlphabet() const { return m_alphabet; }