#pragma once

#define NULL_HANDLE 0xFFFFFFFF
#include "core.h"

// Class that provides a texture for texture mapping in OpenGL
class CTexture
{
private:
	static int s_BoundTextureHandle;
public:
	void CreateBlank(int width, int height, GLenum internalformat, GLenum format, GLenum type, bool generateMipMaps);
	void CreateFromDataExt(void* data, int width, int height, GLenum internalformat, GLenum format, GLenum type, bool generateMipMaps);
	void CreateFromData(void* data, int width, int height, int bpp, GLenum format, bool generateMipMaps = false);

	void Bind(int textureUnit = 0);

	void SetSamplerObjectParameter(GLenum parameter, GLenum value);
	void SetSamplerObjectParameterf(GLenum parameter, float value);

	int GetWidth();
	int GetHeight();
	int GetBPP();

	void Release();

	const GLuint GetHandle() const;
	const std::string& GetName() const;

	void Rename(const std::string& s);
	void Resize(int width, int height);

	CTexture();
	~CTexture();
private:
	int m_width, m_height, m_bpp; // Texture width, height, and bytes per pixel
	UINT m_textureID; // Texture id
	UINT m_samplerObjectID; // Sampler id
	bool m_mipMapsGenerated;

	GLenum m_InternalFormat;
	GLenum m_Format;
	GLenum m_Type;

	std::string m_path; //I stuffed the name into this because it's only used for textures loaded from files, which probably don't need to be named at runtime anyway
};

