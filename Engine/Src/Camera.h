#pragma once
#include "core.h"
class Camera
{
private:
	glm::vec2 m_Position;
	glm::vec2 m_Dimensions;
	glm::mat4 m_Proj;

	void RebuildProjection();

	Camera();
	Camera(const glm::vec2& p, const glm::vec2& d);
public:
	~Camera();

	static std::shared_ptr<Camera> Create(const glm::vec2& position, const glm::vec2& dimensions);

	void SetPosition(const glm::vec2& pos);
	void SetDimensions(const glm::vec2& dims);

	glm::mat4& GetProjection();
};

