#include "turtleInterpreter.h"

pje::engine::PlantTurtle::PlantTurtle(std::string inputAlphabet) : TurtleInterpreter<std::vector<pje::engine::types::Primitive>>("SLF-+[]"), m_renderable() {
	std::string acceptedAlphabet = { m_alphabet };

	/* sorting both alphabets to check for identicalness */
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

void pje::engine::PlantTurtle::buildLSysObject(std::string lSysWord, const std::vector<pje::engine::types::Primitive>& primitiveSet) {
	std::cout << "[PJE] \tBuilding LSysObject.." << std::endl;
}