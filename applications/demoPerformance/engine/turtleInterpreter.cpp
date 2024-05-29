#include "turtleInterpreter.h"

/* PlantTurtle only works with an alphabet := "SLF-+[]" */
pje::engine::PlantTurtle::PlantTurtle(std::string inputAlphabet) : TurtleInterpreter<pje::engine::types::Primitive>("SLF-+[]"), m_renderable() {
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

void pje::engine::PlantTurtle::buildLSysObject(std::string lSysWord, const std::vector<pje::engine::types::Primitive>& primitives) {
	/* resetting m_turtlePosMat and restpose-stack */
	m_turtlePosMat = glm::mat4(1.0f);
	while (!m_restposes.empty())
		m_restposes.pop();

	/* resetting offsets */
	m_offsetVCount = 0;
	m_offsetICount = 0;

	std::cout << "[PJE] \tBuilding Renderable (LSysObject).." << std::endl;

	/* lookup table for faster access to each primitive */
	std::unordered_map<std::string, pje::engine::types::Primitive> primitiveSet;
	const static std::array<std::string, 4> primitiveIdentifier = { "ground", "stem", "leaf", "flower" };

	std::for_each(
		std::execution::par_unseq,
		primitives.begin(),
		primitives.end(),
		[&primitiveSet](const pje::engine::types::Primitive& primitive) {
			for (const auto& identifier : primitiveIdentifier) {
				if (primitive.m_identifier.find(identifier) != std::string::npos) {
					primitiveSet[identifier] = primitive;
					break;
				}
			}
		}
	);

	/* cleanup of m_renderable for this build */
	m_renderable.m_objectPrimitives.clear();
	m_renderable.m_bones.clear();
	m_renderable.m_boneRefs.clear();
	m_renderable.m_matrices = {};
	m_renderable.m_choosenTexture = primitives[0].m_texture;	// PROJECT LIMITATION: same texture map for all primitives

	/* first bone of m_renderable */
	m_renderable.m_bones.push_back(createBone());

	/* optional ground primitive */
	deployPrimitive(primitiveSet.at("ground"), glm::vec3(0.0f), false);

	/* loops through all commands to generate m_renderable */
	for (std::string::size_type i = 0; i < lSysWord.size(); i++) {
		evaluateLSysCommand(lSysWord[i], primitiveSet);
	}
	
	std::cout << "[PJE] \tColumns of final m_turtlePosMat:\n\t" << glm::to_string(m_turtlePosMat) << std::endl;
	std::cout << 
		"[PJE] \tBuilding Renderable (LSysObject) --- DONE" << 
		"\n\tPrimitives inside of Renderable : \t" << m_renderable.m_objectPrimitives.size() << 
		"\n\tBones inside of Renderable : \t\t" << m_renderable.m_bones.size() <<
		"\n\tBoneRefs inside of Renderable : \t" << m_renderable.m_boneRefs.size() << 
	std::endl;
}

void pje::engine::PlantTurtle::evaluateLSysCommand(const char& command, const std::unordered_map<std::string, pje::engine::types::Primitive>& primitiveSet) {
	/* actual evaluation of given command */
	switch (command) {
	case 'S':
		deployPrimitive(primitiveSet.at("stem"), glm::vec3(0.0f, 0.2f, 0.0f), true);
		break;
	case 'L':
		deployPrimitive(primitiveSet.at("leaf"), glm::vec3(0.0f, 0.2f, 0.0f), true);
		break;
	case 'F':
		deployPrimitive(primitiveSet.at("flower"), glm::vec3(0.0f, 0.2f, 0.0f), true);
		break;
	case '-':
		/* Assumption: tilting to left */
		tiltTurtle(-45.0f);
		/* new bone after tilting */
		m_renderable.m_bones.push_back(createBone());
		break;
	case '+':
		/* Assumption: tilting to right */
		tiltTurtle(45.0f);
		/* new bone after tilting */
		m_renderable.m_bones.push_back(createBone());
		break;
	case '[':
		/* saving current turtle position for fallback */
		m_restposes.push(m_turtlePosMat);
		break;
	case ']':
		/* fallback to last remembered turtle position */
		m_turtlePosMat = m_restposes.top();
		m_restposes.pop();
		break;
	default:
		std::cout << "[PJE] \tPlantTurtle received invalid command.\n";
	}
}

void pje::engine::PlantTurtle::deployPrimitive(const pje::engine::types::Primitive& primitive, const glm::vec3& postTurtleTranslation, bool needsBoneRef) {
	/* create new LSysPrimitive for m_renderable */
	pje::engine::types::LSysPrimitive currentLSysPrimitive;
	currentLSysPrimitive.m_identifier						= primitive.m_identifier;
	currentLSysPrimitive.m_texture							= primitive.m_texture;
	currentLSysPrimitive.m_offsetPriorPrimitivesVertices	= m_offsetVCount;
	currentLSysPrimitive.m_offsetPriorPrimitivesIndices		= m_offsetICount;
	currentLSysPrimitive.m_meshes							= primitive.m_meshes;

	/* update PlantTurtle's offsets for next call of deployPrimitive() */
	if (primitive.m_meshes.size() > 1) {
		m_offsetVCount += (
			primitive.m_meshes[primitive.m_meshes.size() - 2].m_offsetPriorMeshesVertices +		// vertex count of all prior meshes to last mesh +
			primitive.m_meshes[primitive.m_meshes.size() - 1].m_vertices.size()					// vertex count of last mesh
			);
		m_offsetICount += (
			primitive.m_meshes[primitive.m_meshes.size() - 2].m_offsetPriorMeshesIndices +		// index count of all prior meshes to last mesh +
			primitive.m_meshes[primitive.m_meshes.size() - 1].m_indices.size()					// index count of last mesh
			);
	}
	else {
		m_offsetVCount += primitive.m_meshes[0].m_vertices.size();
		m_offsetICount += primitive.m_meshes[0].m_indices.size();
	}

	/* creates BoneRef and an offset for the primitive's vertices to access the right BoneRef in shader */
	glm::uint offset;
	if (needsBoneRef) {
		/* create new BoneRef for all vertices of this primitive */
		m_renderable.m_boneRefs.push_back(createRef());
		/* offset for m_boneAttrib of each vertex of this primitive */
		offset = m_renderable.m_boneRefs.size() - 1;
	}
	else {
		offset = 0;
	}

	/* vertex: primitive space => model space */
	for (auto& mesh : currentLSysPrimitive.m_meshes) {
		std::for_each(
			std::execution::par_unseq,
			mesh.m_vertices.begin(),
			mesh.m_vertices.end(),
			[&](pje::engine::types::Vertex& v) {
				// vertex to model space: O_i * v
				v.m_pos			= glm::vec3(m_turtlePosMat * glm::vec4(v.m_pos, 1.0f));
				// adjusting normal after transforming normal
				v.m_normal		= glm::normalize(glm::transpose(glm::inverse(m_turtlePosMat)) * glm::vec4(v.m_normal, 0.0f));
				// uvec2(<first relevant boneRef>, <boneRefsCount for this vertex>) | PROJECT LIMITATION: count = 1 (1 BoneRef per Vertex)
				v.m_boneAttrib	= glm::uvec2(offset, 1);
			}
		);
	}

	m_renderable.m_objectPrimitives.push_back(currentLSysPrimitive);

	/* local translation: m_turtlePosMat * postTurtleTranslation */
	m_turtlePosMat = glm::translate(m_turtlePosMat, postTurtleTranslation);
	std::cout << "[GLM] \tColumns of m_turtlePosMat after deployPrimitive:\n\t" << glm::to_string(m_turtlePosMat) << std::endl;
}

void pje::engine::PlantTurtle::tiltTurtle(float degrees) {
	glm::mat4 rotation = glm::mat4(1.0f);

	/* ### R_z(degrees) := I * R_y(-90.0f) * R_x(degrees) * R_y(90.0f) ### */
	rotation = glm::rotate(rotation, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	rotation = glm::rotate(rotation, glm::radians(degrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotation = glm::rotate(rotation, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	//rotation = glm::rotate(rotation, glm::radians(degrees), glm::vec3(0.0f, 0.0f, 1.0f));

	/* local rotation: m_turtlePosMat * rotation */
	m_turtlePosMat *= rotation;
	std::cout << "[GLM] \tColumns of tilted m_turtlePosMat:\n\t" << glm::to_string(m_turtlePosMat) << std::endl;
}

pje::engine::types::Bone pje::engine::PlantTurtle::createBone() {
	/* Bone(restpose, restposeInv, animationpose) */
	return pje::engine::types::Bone{
		m_turtlePosMat, glm::inverse(m_turtlePosMat), glm::mat4(1.0f)
	};
}

pje::engine::types::BoneRef pje::engine::PlantTurtle::createRef() {
	/* BoneRef(boneId, weight) */
	return pje::engine::types::BoneRef{
		m_renderable.m_bones.size() - 1,
		1.0f								// PROJECT LIMITATION: 1 vertex <-> 1 bone
	};
}