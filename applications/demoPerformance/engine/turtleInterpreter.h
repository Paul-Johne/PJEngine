/* Ignores header if not needed after #include */
	#pragma once

/* Third Party Files */
	#include <string>							// std::string
	#include <array>							// std::array
	#include <vector>							// std::vector
	#include <unordered_map>					// hashtable
	#include <stack>							// std::stack
	
	#include <iostream>							// i/o stream
	#include <algorithm>						// classic functions for ranges
	#include <execution>						// parallel algorithms

	#include <glm/glm.hpp>						// glm types
	#include <glm/gtx/string_cast.hpp>			// glm to string debugging
	#include <glm/gtc/matrix_transform.hpp>		// glm matrix operations

/* Project Files */
	#include "pjeBuffers.h"

namespace pje::engine {

	/* TurtleInterpreter - Declares base functionalities (Generating of matrices and PJEModel) */
	template <typename Source>
	class TurtleInterpreter {
	public:
		TurtleInterpreter() = delete;
		virtual ~TurtleInterpreter() {};
		virtual void buildLSysObject(std::string lSysWord, const std::vector<Source>& source) = 0;

	protected:
		const std::string		m_alphabet;
		glm::mat4				m_turtlePosMat;		// build helper: current position of turtle
		std::stack<glm::mat4>	m_restposes;		// build helper: snapshots of m_turtlePosMat

		TurtleInterpreter(std::string validAlphabet) : m_alphabet(validAlphabet), m_turtlePosMat(glm::mat4(1.0f)), m_restposes() {};
	};

	/* PlantTurtle - Generates animatable 3D object */
	class PlantTurtle final : public TurtleInterpreter<pje::engine::types::Primitive> {
	public:
		pje::engine::types::LSysObject m_renderable;	// latest renderable generated by buildLSysObject()

		PlantTurtle() = delete;
		/* STANDARD CONSTRUCTOR
		*	> inputAlphabet	 : { S, L, F, -, +, [, ] }
		*		> S => stem | L => leaf | F => flower | - => left tilt | + => right tilt
		*		> [ => push m_turtlePosMat on stack | ] => pop m_turtlePosMat from stack
		*/
		PlantTurtle(std::string inputAlphabet);
		~PlantTurtle();

		/* builds LSysObject by evaluating a given lSysWord and using a given set of Primitives */
		void buildLSysObject(std::string lSysWord, const std::vector<pje::engine::types::Primitive>& primitives) override;

	private:
		size_t m_offsetVCount = 0;		// build helper: set via LSysPrimitive::m_offsetPriorPrimitivesVertices
		size_t m_offsetICount = 0;		// build helper: set via LSysPrimitive::m_offsetPriorPrimitivesIndices

		/* procedural generation of m_renderable */
		void evaluateLSysCommand(const char& command, const std::unordered_map<std::string, pje::engine::types::Primitive>& primitiveSet);
		/* deploys primitive by solving multiple tasks:
		*	1) deploys primitive via m_turtlePosMat			=> m_renderable
		*	2) assigns (old/new) boneMatrix && new boneRef	=> m_renderable
		*	3) moves m_turtlePosMat by postTurtleTranslation
		*/
		void deployPrimitive(const pje::engine::types::Primitive& primitive, const glm::vec3& postTurtleTranslation, bool needsBoneRef);
		/* rotates m_turtlePosMat by given rotationMat */
		void tiltTurtle(float degrees);
		/* creates a new Bone object for m_renderable.m_boneMatrices */
		pje::engine::types::Bone createBone();
		/* creates a new BoneRef object for m_renderable.m_boneRefs */
		pje::engine::types::BoneRef createRef();
	};
}