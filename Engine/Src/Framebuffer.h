#pragma once
#include "core.h"
#include "Event.h"
#include "Texture.h"
#include <map>

enum AttachmentType {
	COLOR,
	DEPTH,
	STENCIL
};

#pragma warning(push)
#pragma warning(suppress : 5208)
typedef struct {
	uint16_t Width = 1;
	uint16_t Height = 1;
	GLint InternalFormat = GL_RGBA;
	GLenum PixelFormat = GL_RGBA;
	GLenum ChannelType = GL_FLOAT;
	/*
	* WindowBuffer flag marks whether a Framebuffer size should mirror the size of the Viewport / GameWindow client area
	* When the EVENT_RESIZE signal is broadcast by the Message Pump, the Framebuffer textures are freed and new ones are generated following the ColorBufferCount and DepthAndStencil properties
	* of the FramebufferDesc struct
	*/
	bool WindowBuffer = false;

	//you can manually attach texture to the depth slot but I'm going to auto gen a Depth&Stencil buffer texture
	bool DepthAndStencil = true;

	bool NoColorAttachment = false;
} FramebufferDesc;
#pragma warning(pop)
//stupid error not important, complains abuot valid c++ code

typedef struct {
	glm::ivec2 src0;
	glm::ivec2 src1;
	glm::ivec2 dst0;
	glm::ivec2 dst1;
	GLbitfield mask;
	GLenum filter;
} FramebufferBlitParams;

//no copy or rvalue constructor, copy assignment and rvalue assignment also deleted. Use the ::Create method to get an instance wrapped in a shared_ptr
class Framebuffer
{
private:
	static GLuint s_BoundFramebuffer;

	std::map<GLenum, std::shared_ptr<CTexture> > m_Textures;
	std::shared_ptr<CTexture> m_DepthStencilTexture;
	std::vector<int> m_DrawBuffers;

	int Initialize();
	void Release();
protected:
	FramebufferDesc m_Desc;
	Framebuffer(const FramebufferDesc&);

	GLuint m_id;

	void DebugAttachment(GLenum attachment);
public:
	~Framebuffer();

	Framebuffer(const Framebuffer&) = delete; //no copy constructor
	Framebuffer(Framebuffer&&) = delete; //no construction from rvalue 
	Framebuffer& operator=(const Framebuffer&) = delete; //no copy assignment
	Framebuffer& operator=(Framebuffer&&) = delete; //no rvalue assignment either

	static std::shared_ptr<Framebuffer> Create(const FramebufferDesc&);

	virtual void Bind();
	virtual void Unbind();

	void AttachTexture(GLenum attachment, const std::shared_ptr<CTexture>& texture);
	int CheckStatus();
	void SetDrawBuffers(const std::vector<int>& AttachmentUnits);

	std::shared_ptr<CTexture> GetDepthTexture();

	//void Blit(const FramebufferBlitParams&);
	//void Blit(const FramebufferBlitParams&, const Framebuffer& destination);
};

