#include "turtleInterpreter.h"

pje::engine::TurtleInterpreter::TurtleInterpreter(std::string validAlphabet) : m_alphabet(validAlphabet) {}

pje::engine::TurtleInterpreter::~TurtleInterpreter() {}

/* ################################################################################### */

pje::engine::PlantTurtle::PlantTurtle(std::string inputAlphabet, std::string validAlphabet) : TurtleInterpreter(validAlphabet) {
	std::string acceptedAlphabet = { this->m_alphabet };

	std::sort(acceptedAlphabet.begin(), acceptedAlphabet.end());
	std::sort(inputAlphabet.begin(), inputAlphabet.end());

	if (inputAlphabet == acceptedAlphabet) {
		std::cout << "[PJE] \tPlantTurtle received valid input alphabet for object generation." << std::endl;
	}
	else {
		throw std::runtime_error("PlantTurtle doesn't understand given input alphabet.");
	}
}

pje::engine::PlantTurtle::~PlantTurtle() {}