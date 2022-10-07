#include "Camera.h"

Camera::Camera() {
	m_Position = glm::vec2(0.f);
	m_Dimensions = glm::vec2(1920.f, 1080.f);
	RebuildProjection();
}

Camera::Camera(const glm::vec2& p, const glm::vec2& d) : m_Position(p), m_Dimensions(d) {
	RebuildProjection();
}

Camera::~Camera() {
	//stuff
}

void Camera::RebuildProjection() {

	m_Proj = glm::ortho(
		m_Position.x - m_Dimensions.x / 2.f,
		m_Position.x + m_Dimensions.x / 2.f,
		m_Position.y - m_Dimensions.y / 2.f,
		m_Position.y + m_Dimensions.y / 2.f
	);
}

std::shared_ptr<Camera> Camera::Create(const glm::vec2& position, const glm::vec2& dimensions) {
	return std::shared_ptr<Camera>(new Camera(position, dimensions));
}

void Camera::SetPosition(const glm::vec2& p) {
	m_Position = p;
	RebuildProjection();
}

void Camera::SetDimensions(const glm::vec2& d) {
	m_Dimensions = d;
	RebuildProjection();
}

glm::mat4& Camera::GetProjection() {
	return m_Proj;
}