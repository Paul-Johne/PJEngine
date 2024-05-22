/* Ignores header if not needed after #include */
	#pragma once

	#include <string>		// std::string
	#include <vector>		// std::vector
	
	#include <iostream>		// i/o stream
	#include <algorithm>	// std::sort

	#include <glm/glm.hpp>	// glm

	#include "pjeBuffers.h"

namespace pje::engine {

	/* TurtleInterpreter - Declares base functionalities (Generating of matrices and PJEModel) */
	template <typename Source>
	class TurtleInterpreter {
	public:
		TurtleInterpreter() = delete;
		virtual ~TurtleInterpreter() {};
		virtual void buildLSysObject(std::string lSysWord, const Source& source) = 0;
	protected:
		const std::string m_alphabet;
		TurtleInterpreter(std::string validAlphabet) : m_alphabet(validAlphabet) {};
	};

	/* PlantTurtle - Generates animatable 3D object */
	class PlantTurtle final : public TurtleInterpreter<std::vector<pje::engine::types::Primitive>> {
	public:
		pje::engine::types::LSysObject m_renderable;

		PlantTurtle() = delete;
		/* STANDARD CONSTRUCTOR
		*	> inputAlphabet	 : { S, L, F, -, +, [, ] }
		*/
		PlantTurtle(std::string inputAlphabet);
		~PlantTurtle();

		/* builds LSysObject by evaluating a given lSysWord and using a given set of Primitives */
		void buildLSysObject(std::string lSysWord, const std::vector<pje::engine::types::Primitive>& primitiveSet) override;
	};
}