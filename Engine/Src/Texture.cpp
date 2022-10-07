#include "texture.h"

int CTexture::s_BoundTextureHandle = NULL_HANDLE;

CTexture::CTexture() :
	m_textureID(NULL_HANDLE),
	m_path("UNNAMED TEXTURE"),
	m_samplerObjectID(NULL_HANDLE),
	m_InternalFormat(GL_RGB),
	m_Format(GL_RGB),
	m_Type(GL_UNSIGNED_BYTE)
{
	m_mipMapsGenerated = false;
}
CTexture::~CTexture()
{
	Release();
	m_textureID = NULL_HANDLE;
}

void CTexture::CreateBlank(int width, int height, GLenum internalformat, GLenum format, GLenum type, bool generateMipMaps) {
	CreateFromDataExt(0, width, height, internalformat, format, type, generateMipMaps);
}

void CTexture::CreateFromDataExt(void* data, int width, int height, GLenum internalformat, GLenum format, GLenum type, bool generateMipMaps) {
	if (m_textureID != NULL_HANDLE) {
		Release();
	}

	//correct from scuffed FreeImage data format to something that is actually used by mentally sound people
	if (internalformat == GL_BGR) {
		internalformat = GL_RGB;
	}
	else if (internalformat == GL_BGRA) {
		internalformat = GL_RGBA;
	}

	m_InternalFormat = internalformat;
	m_Format = format;
	m_Type = type;
	m_width = width;
	m_height = height;

	glGenTextures(1, &m_textureID);
	glGenSamplers(1, &m_samplerObjectID);
	//LOG("Create Texture " << m_textureID);
	glBindTexture(GL_TEXTURE_2D, m_textureID);

	Engine::CheckGLError("1");

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, data);

	Engine::CheckGLError("2");

	GLenum min_filter = GL_LINEAR;
	if (generateMipMaps) {
		//LOG("GL_LINEAR_MIPMAP_LINEAR");
		min_filter = GL_LINEAR_MIPMAP_LINEAR;
	}

	SetSamplerObjectParameter(GL_TEXTURE_MIN_FILTER, min_filter);
	SetSamplerObjectParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	SetSamplerObjectParameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
	SetSamplerObjectParameter(GL_TEXTURE_WRAP_T, GL_REPEAT);

	Engine::CheckGLError("3");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	Engine::CheckGLError("4");

	if (generateMipMaps) {
		glGenerateTextureMipmap(m_textureID); //bindless mipmap generation
		m_mipMapsGenerated = true;
	}
}

// Create a texture from the data stored in bData.  
void CTexture::CreateFromData(void* data, int width, int height, int bpp, GLenum format, bool generateMipMaps)
{
	CreateFromDataExt(data, width, height, format, format, GL_UNSIGNED_BYTE, generateMipMaps);
}

void CTexture::Rename(const std::string& s) {
	m_path = s; //This is bad code, it isn't really intuitive that I stuffed the name into the path member
}

void CTexture::SetSamplerObjectParameter(GLenum parameter, GLenum value)
{
	glSamplerParameteri(m_samplerObjectID, parameter, value);
}

void CTexture::SetSamplerObjectParameterf(GLenum parameter, float value)
{
	glSamplerParameterf(m_samplerObjectID, parameter, value);
}

// Binds a texture for rendering
void CTexture::Bind(int iTextureUnit)
{
	if (s_BoundTextureHandle == m_textureID) {
		return;
	}
	s_BoundTextureHandle = m_textureID;

	//LOG("Bind Texture " << m_textureID << " (" << m_path << ") to slot " << iTextureUnit << " texture type " << Renderer::GetTextureType(m_textureID));
	glActiveTexture(GL_TEXTURE0 + iTextureUnit);
	
	glBindTexture(GL_TEXTURE_2D, m_textureID);
	
	glBindSampler(iTextureUnit, m_samplerObjectID);
	
}

// Frees memory on the GPU of the texture
void CTexture::Release()
{
	if (m_textureID != NULL_HANDLE && m_textureID < -1) {
		glDeleteSamplers(1, &m_samplerObjectID);
		glDeleteTextures(1, &m_textureID);
	}
}

int CTexture::GetWidth()
{
	return m_width;
}

int CTexture::GetHeight()
{
	return m_height;
}

int CTexture::GetBPP()
{
	return m_bpp;
}

const GLuint CTexture::GetHandle() const {
	return m_textureID;
}

void CTexture::Resize(int width, int height) {
	Release();
	CreateBlank(width, height, m_InternalFormat, m_Format, m_Type, m_mipMapsGenerated);
}

const std::string& CTexture::GetName() const {
	return m_path;
}