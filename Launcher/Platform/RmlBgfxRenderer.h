#pragma once

#include <RmlUi/Core/RenderInterface.h>
#include <bgfx/bgfx.h>
#include <vector>
#include <unordered_map>

class RmlBgfxRenderer : public Rml::RenderInterface {
public:
	RmlBgfxRenderer();
	~RmlBgfxRenderer();

	bool Init(int viewWidth, int viewHeight);
	void Shutdown();
	void SetViewId(bgfx::ViewId viewId) { m_viewId = viewId; }
	void SetViewportSize(int width, int height);
	void BeginFrame();

	// RmlUi RenderInterface (required)
	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
	void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;

	void SetTransform(const Rml::Matrix4f* transform) override;

private:
	struct CompiledGeom {
		std::vector<Rml::Vertex> vertices;
		std::vector<int> indices;
	};

	bgfx::ViewId m_viewId = 200;
	bgfx::ProgramHandle m_texturedProgram = BGFX_INVALID_HANDLE;
	bgfx::ProgramHandle m_colorProgram = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle m_texUniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle m_translateUniform = BGFX_INVALID_HANDLE;
	bgfx::TextureHandle m_whiteTexture = BGFX_INVALID_HANDLE;
	bgfx::VertexLayout m_layout;

	int m_width = 0, m_height = 0;
	bool m_scissorEnabled = false;
	Rml::Rectanglei m_scissorRegion;
	bool m_hasTransform = false;
	float m_transform[16];

	Rml::CompiledGeometryHandle m_nextGeomId = 1;
	std::unordered_map<Rml::CompiledGeometryHandle, CompiledGeom*> m_geometries;

	Rml::TextureHandle m_nextTexId = 1;
	std::unordered_map<Rml::TextureHandle, bgfx::TextureHandle> m_textures;

	bool LoadShaders();
};
