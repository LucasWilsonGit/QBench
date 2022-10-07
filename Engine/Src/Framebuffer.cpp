#include "Framebuffer.h"
#include <iostream>

std::string TextureTypeString(GLenum x) {
	std::string type;
	switch (x) {
	case GL_TEXTURE:
		type = "GL_TEXTURE";
		break;
	case GL_RENDERBUFFER:
		type = "GL_RENDERBUFFER";
		break;
	case GL_NONE:
		type = "GL_NONE";
		break;
	case GL_TEXTURE_1D:
		type = "GL_TEXTURE_1D";
		break;
	case GL_TEXTURE_2D:
		type = "GL_TEXTURE_2D";
		break;
	case GL_TEXTURE_3D:
		type = "GL_TEXTURE_3D";
		break;
	case GL_TEXTURE_CUBE_MAP:
		type = "GL_TEXTURE_CUBE_MAP";
		break;
	default:
		type = "DEFAULT";
	}
	return type;
}

std::string AttachmentNameString(GLenum x) {
	std::string type;
	switch (x) {
	case GL_STENCIL_ATTACHMENT:
		type = "STENCIL_ATTACHMENT";
		break;
	case GL_DEPTH_ATTACHMENT:
		type = "DEPTH_ATTACHMENT";
		break;
	case GL_DEPTH_STENCIL_ATTACHMENT:
		type = "DEPTH_STENCIL_ATTACHMENT";
		break;
	default:
		type = "COLOR_ATTACHMENT" + std::to_string(x - GL_COLOR_ATTACHMENT0);
		break;
	}
	return type;
}

GLuint Framebuffer::s_BoundFramebuffer = 0; //the backbuffer

Framebuffer::Framebuffer(const FramebufferDesc& desc) : m_Desc(desc), m_Textures(std::map<GLenum, std::shared_ptr<CTexture>>{}) {
	Initialize();
}

Framebuffer::~Framebuffer() {
	Release();
}

int Framebuffer::Initialize() {
	glCreateFramebuffers(1, &m_id);
	glBindFramebuffer(GL_FRAMEBUFFER, m_id);

	//Generates and binds the Depth Stencil Texture
	if (m_Desc.DepthAndStencil) {
		m_DepthStencilTexture = std::make_shared<CTexture>();
		m_DepthStencilTexture->CreateBlank(m_Desc.Width, m_Desc.Height, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, false);
		//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthStencilTexture->GetHandle(), 0);
		//m_Textures.emplace(pair<GLenum, std::shared_ptr<CTexture>)
		AttachTexture(GL_DEPTH_STENCIL_ATTACHMENT, m_DepthStencilTexture);
	}

	if (m_Desc.NoColorAttachment) {
		glReadBuffer(GL_NONE);
		glDrawBuffer(GL_NONE);
	}

	//Restore the previously bound buffer
	glBindFramebuffer(GL_FRAMEBUFFER, s_BoundFramebuffer);

	if (m_Desc.WindowBuffer) {
		//if the Framebuffer size is flagged to be locked to the viewport size then we should subscribe to EVENT_RESIZE signals and resize our Framebuffer textures
		
	}

	return 0;
}

int Framebuffer::CheckStatus() {
	if (s_BoundFramebuffer != m_id) {
		//WARN("Binding framebuffer " << m_id << " to check status");
		Bind();
	}

	//Check for errors
	int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch (status) {
	case GL_FRAMEBUFFER_COMPLETE:
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		ERROR("Framebuffer incomplete attachment");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		ERROR("Framebuffer missing attachments");
		break;
	/*case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
		ERROR("Framebuffer incomplete dimensions");
		break; 
		
		missing from the opengl wrapper that I've linked
	*/
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		ERROR("Framebuffer incomplete DrawBuffer");
		break;
	default:
		std::cout << "Framebuffer not complete!" << std::endl;
		break;
	}
	return status;
}

void Framebuffer::Release() {
	Unbind();
	glDeleteFramebuffers(1, &m_id);

	m_Textures.clear(); //CTexture class slightly modified to follow RAII, so that we can just let the destructor clean up resources assuming our Framebuffer is the only thing holding a ref
}

std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferDesc& desc) {

	return std::shared_ptr<Framebuffer>(new Framebuffer(desc));
}

void Framebuffer::DebugAttachment(GLenum attachment) {
	int slot = attachment - GL_COLOR_ATTACHMENT0;
	std::string attachmentstr = AttachmentNameString(attachment);
	std::shared_ptr<CTexture> tex;
	auto res = m_Textures.find(attachment);
	if (res != m_Textures.end()) {
		tex = res->second;
	}
	else
	{
		ERROR("Attempt to debug unbound attachment slot " << attachmentstr);
	}
	//std::shared_ptr<CTexture> tex = m_Desc.DepthAndStencil ? m_Textures[slot + 1] : m_Textures[slot];

	GLint x;
	glGetNamedFramebufferAttachmentParameteriv(m_id, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &x);

	GLuint tex_handle = x;

	glGetNamedFramebufferAttachmentParameteriv(m_id, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &x);
	std::string type = TextureTypeString(x);
	glGetTextureParameteriv(tex_handle, GL_TEXTURE_TARGET, &x);

	m_Textures[slot]->Bind();
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_SAMPLES, &x);
	GLint y;
	glGetIntegerv(GL_MAX_FRAMEBUFFER_SAMPLES, &y);

}

void Framebuffer::Bind() {
	glViewport(0, 0, m_Desc.Width, m_Desc.Height); //update the global state viewport which is some kind of transform from unit space fragments into the framebuffer space, necessary so that the full bound textures are drawn to, not some window inside them or beyond them
	s_BoundFramebuffer = m_id;
	glBindFramebuffer(GL_FRAMEBUFFER, m_id);
	if (m_DrawBuffers.size() > 0) {
		//WARN("Bound " << m_DrawBuffers.size() << " draw buffers while binding FBO " << m_id);
		glDrawBuffers(m_DrawBuffers.size(), (const GLenum*)&m_DrawBuffers[0]);

		for (GLenum a : m_DrawBuffers) {
			auto res = m_Textures.find(a);
			std::string s = "";
			if (res != m_Textures.end()) {
				s = res->second->GetName();
			}
			//WARN(AttachmentNameString(a) << " " << s);
		}
	}
	else
	{
		//WARN("Bound empty framebuffer FBO " << m_id << ", set DrawBuffer to GL_NONE");
		glDrawBuffer(GL_NONE);
	}
}

void Framebuffer::Unbind() {
	if (s_BoundFramebuffer == m_id) {
		//WARN("Unbind Framebuffer " << m_id);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		s_BoundFramebuffer = 0;
		//glDrawBuffer(GL_BACK); draw buffer is part of FBO state, unbinding should return to default fbo which has it's drawbuffer set up already
	}
}

//Does not need to be bound to be called, 
void Framebuffer::AttachTexture(GLenum attachment, const std::shared_ptr<CTexture>& texture) {
	if (attachment > GL_COLOR_ATTACHMENT0 + 15) {
		ERROR(__FUNCSIG__ << ": ERROR: Attachment slot exceeds maximum FBO color attachment slot");
	}

	if (m_Desc.NoColorAttachment) {
		//WARN("Attaching to no color attachment fbo");
	}

	//glNamedFramebufferTexture(m_id, attachment, texture->GetHandle(), 0);
	glNamedFramebufferTexture(m_id, attachment, texture->GetHandle(), 0);
	//LOG("Framebuffer " << m_id << " attach texture " << texture->GetHandle() << " to color attachment " << attachment - GL_COLOR_ATTACHMENT0);

	auto res = m_Textures.find(attachment);
	if (res != m_Textures.end()) {
		res->second = texture;
	}
	else
	{
		m_Textures.emplace(std::pair<GLenum, std::shared_ptr<CTexture>>(attachment, texture));
	}

	//If the attachment is a GL_COLOR_ATTACHMENTx not already in the internal DrawBuffers vector, add it
	//i.e Attaching a texture to an attachment point implies that the attachment point should be able to be drawn to by the shaders
	bool add = true;
	for (auto i : m_DrawBuffers) {
		if (i == attachment) {
			add = false;
		}
	}
	if (add && attachment >= GL_COLOR_ATTACHMENT0 && attachment <= GL_COLOR_ATTACHMENT0 + 15) {
		m_DrawBuffers.push_back(attachment);
	}
}

void Framebuffer::SetDrawBuffers(const std::vector<int>& AttachmentUnits) {

	if (s_BoundFramebuffer == m_id) {
		WARN("Set Draw Buffers");
		glDrawBuffers(AttachmentUnits.size(), (const GLenum*)&AttachmentUnits[0]);
	}
	else
	{
		ERROR(__FUNCSIG__ << ": Setting DrawBuffers for unbound Framebuffer");
	}

	m_DrawBuffers = AttachmentUnits;
}

std::shared_ptr<CTexture> Framebuffer::GetDepthTexture() {
	return m_DepthStencilTexture;
}