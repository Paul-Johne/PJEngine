/* Ignores header if not needed after #include */
	#pragma once

	#include <string>		// std::string
	
	#include <iostream>		// i/o stream
	#include <algorithm>	// std::sort

	#include <glm/glm.hpp>	// glm

namespace pje::engine {

	// abstract function: virtual <return type> <method name> () = 0

	/* [ABSTRACT] TurtleInterpreter - Declares base functionalities (Generating of matrices and PJEModel) */
	class TurtleInterpreter {
	public:
		TurtleInterpreter() = delete;
		TurtleInterpreter(std::string validAlphabet);
		~TurtleInterpreter();
	protected:
		const std::string m_alphabet;
	};

	/* PlantTurtle - Generates animatable 3D object */
	class PlantTurtle final : public TurtleInterpreter {
	public:
		PlantTurtle() = delete;
		/* STANDARD CONSTRUCTOR
		*	> alphabet	 : { S, L, F, -, +, [, ] }
		*	> base class : uses TurtleInterpreter(std::string)
		*/
		PlantTurtle(std::string inputAlphabet, std::string validAlphabet = "SLF-+[]");
		~PlantTurtle();
	};
}