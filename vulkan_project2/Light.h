#pragma once
#include <glm/glm.hpp>
struct DynamicLight { //동적 라이팅(쉬움 but 성능 low)
	glm::vec3 lightDir;
	
};
struct StaticLight { //정적 라이팅
	glm::vec3 lightDir;

};
