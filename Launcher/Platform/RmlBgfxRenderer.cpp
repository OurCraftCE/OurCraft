#include "RmlBgfxRenderer.h"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

#include "shaders/shader_rmlui.h"

// stb_image for LoadTexture (implementation is in BgfxRenderer.cpp)
#include "../../ExternalDependency/bimg/3rdparty/stb/stb_image.h"

#include <cstdio>
#include <cstring>

RmlBgfxRenderer::RmlBgfxRenderer()
{
	memset(m_transform, 0, sizeof(m_transform));
	m_transform[0] = m_transform[5] = m_transform[10] = m_transform[15] = 1.0f;
}

RmlBgfxRenderer::~RmlBgfxRenderer()
{
	Shutdown();
}

bool RmlBgfxRenderer::Init(int viewWidth, int viewHeight)
{
	m_width = viewWidth;
	m_height = viewHeight;

	// Vertex layout matching Rml::Vertex: position(2f), colour(4b), tex_coord(2f)
	m_layout
		.begin()
		.add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		.end();

	// Create 1x1 white texture for untextured geometry
	{
		uint32_t white = 0xFFFFFFFF;
		const bgfx::Memory* mem = bgfx::copy(&white, 4);
		m_whiteTexture = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, 0, mem);
	}

	// Create uniforms
	m_texUniform = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
	m_translateUniform = bgfx::createUniform("u_rmlTranslate", bgfx::UniformType::Vec4);

	if (!LoadShaders())
		return false;

	return true;
}

bool RmlBgfxRenderer::LoadShaders()
{
	const uint8_t* vsData = vs_rmlui_d3d11;
	uint32_t vsSize = sizeof(vs_rmlui_d3d11);
	const uint8_t* fsTexData = fs_rmlui_d3d11;
	uint32_t fsTexSize = sizeof(fs_rmlui_d3d11);
	const uint8_t* fsColData = fs_rmlui_color_d3d11;
	uint32_t fsColSize = sizeof(fs_rmlui_color_d3d11);

	// Textured program
	{
		bgfx::ShaderHandle vsh = bgfx::createShader(bgfx::copy(vsData, vsSize));
		bgfx::ShaderHandle fsh = bgfx::createShader(bgfx::copy(fsTexData, fsTexSize));
		m_texturedProgram = bgfx::createProgram(vsh, fsh, true);
		if (!bgfx::isValid(m_texturedProgram))
			return false;
	}

	// Color-only program
	{
		bgfx::ShaderHandle vsh = bgfx::createShader(bgfx::copy(vsData, vsSize));
		bgfx::ShaderHandle fsh = bgfx::createShader(bgfx::copy(fsColData, fsColSize));
		m_colorProgram = bgfx::createProgram(vsh, fsh, true);
		if (!bgfx::isValid(m_colorProgram))
			return false;
	}

	return true;
}

void RmlBgfxRenderer::Shutdown()
{
	// Release all geometries
	for (auto& pair : m_geometries)
		delete pair.second;
	m_geometries.clear();

	// Release all textures
	for (auto& pair : m_textures)
	{
		if (bgfx::isValid(pair.second))
			bgfx::destroy(pair.second);
	}
	m_textures.clear();

	if (bgfx::isValid(m_whiteTexture)) { bgfx::destroy(m_whiteTexture); m_whiteTexture = BGFX_INVALID_HANDLE; }
	if (bgfx::isValid(m_texturedProgram)) { bgfx::destroy(m_texturedProgram); m_texturedProgram = BGFX_INVALID_HANDLE; }
	if (bgfx::isValid(m_colorProgram)) { bgfx::destroy(m_colorProgram); m_colorProgram = BGFX_INVALID_HANDLE; }
	if (bgfx::isValid(m_texUniform)) { bgfx::destroy(m_texUniform); m_texUniform = BGFX_INVALID_HANDLE; }
	if (bgfx::isValid(m_translateUniform)) { bgfx::destroy(m_translateUniform); m_translateUniform = BGFX_INVALID_HANDLE; }
}

void RmlBgfxRenderer::SetViewportSize(int width, int height)
{
	m_width = width;
	m_height = height;
}

void RmlBgfxRenderer::BeginFrame()
{
	// Set up orthographic projection for this view
	float ortho[16];
	bx::mtxOrtho(ortho, 0.0f, (float)m_width, (float)m_height, 0.0f, -1.0f, 1.0f, 0.0f, bgfx::getCaps()->homogeneousDepth);

	bgfx::setViewRect(m_viewId, 0, 0, (uint16_t)m_width, (uint16_t)m_height);
	bgfx::setViewTransform(m_viewId, nullptr, ortho);
	bgfx::setViewMode(m_viewId, bgfx::ViewMode::Sequential);

	// Clear state
	m_scissorEnabled = false;
}

// ---- Geometry ----

Rml::CompiledGeometryHandle RmlBgfxRenderer::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	CompiledGeom* geom = new CompiledGeom();
	geom->vertices.assign(vertices.begin(), vertices.end());
	geom->indices.assign(indices.begin(), indices.end());

	Rml::CompiledGeometryHandle handle = m_nextGeomId++;
	m_geometries[handle] = geom;
	return handle;
}

void RmlBgfxRenderer::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture)
{
	auto it = m_geometries.find(geometry);
	if (it == m_geometries.end())
		return;

	CompiledGeom* geom = it->second;
	int numVertices = (int)geom->vertices.size();
	int numIndices = (int)geom->indices.size();

	if (numVertices == 0 || numIndices == 0)
		return;

	// Check transient buffer availability
	if (!bgfx::getAvailTransientVertexBuffer(numVertices, m_layout))
		return;
	if (!bgfx::getAvailTransientIndexBuffer(numIndices, true))
		return;

	bgfx::TransientVertexBuffer tvb;
	bgfx::TransientIndexBuffer tib;
	bgfx::allocTransientVertexBuffer(&tvb, numVertices, m_layout);
	bgfx::allocTransientIndexBuffer(&tib, numIndices, true); // 32-bit indices

	// Copy vertex data
	memcpy(tvb.data, geom->vertices.data(), numVertices * sizeof(Rml::Vertex));

	// Copy index data (RmlUi uses int, bgfx 32-bit index = uint32_t)
	memcpy(tib.data, geom->indices.data(), numIndices * sizeof(int));

	bgfx::setVertexBuffer(0, &tvb, 0, numVertices);
	bgfx::setIndexBuffer(&tib, 0, numIndices);

	// Set translation uniform
	float translate[4] = { translation.x, translation.y, 0.0f, 0.0f };
	bgfx::setUniform(m_translateUniform, translate);

	// Set transform if active
	if (m_hasTransform)
	{
		bgfx::setTransform(m_transform);
	}

	// Scissor
	if (m_scissorEnabled)
	{
		bgfx::setScissor(
			(uint16_t)m_scissorRegion.Left(),
			(uint16_t)m_scissorRegion.Top(),
			(uint16_t)m_scissorRegion.Width(),
			(uint16_t)m_scissorRegion.Height()
		);
	}

	// State: alpha blending, write RGB+A, no depth test
	uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A
		| BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA);

	bgfx::setState(state);

	// Texture
	if (texture != 0)
	{
		auto texIt = m_textures.find(texture);
		if (texIt != m_textures.end())
			bgfx::setTexture(0, m_texUniform, texIt->second);
		else
			bgfx::setTexture(0, m_texUniform, m_whiteTexture);

		bgfx::submit(m_viewId, m_texturedProgram);
	}
	else
	{
		bgfx::setTexture(0, m_texUniform, m_whiteTexture);
		bgfx::submit(m_viewId, m_colorProgram);
	}
}

void RmlBgfxRenderer::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
	auto it = m_geometries.find(geometry);
	if (it != m_geometries.end())
	{
		delete it->second;
		m_geometries.erase(it);
	}
}

// ---- Textures ----

Rml::TextureHandle RmlBgfxRenderer::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source)
{
	FILE* f = fopen(source.c_str(), "rb");
	if (!f)
		return 0;

	fseek(f, 0, SEEK_END);
	long fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);

	std::vector<unsigned char> fileData((size_t)fileSize);
	fread(fileData.data(), 1, (size_t)fileSize, f);
	fclose(f);

	int w = 0, h = 0, channels = 0;
	unsigned char* pixels = stbi_load_from_memory(fileData.data(), (int)fileSize, &w, &h, &channels, 4);
	if (!pixels)
		return 0;

	texture_dimensions.x = w;
	texture_dimensions.y = h;

	// Premultiply alpha to match RmlUi convention
	for (int i = 0; i < w * h; ++i)
	{
		unsigned char* p = pixels + i * 4;
		unsigned char a = p[3];
		p[0] = (unsigned char)((p[0] * a) / 255);
		p[1] = (unsigned char)((p[1] * a) / 255);
		p[2] = (unsigned char)((p[2] * a) / 255);
	}

	const bgfx::Memory* mem = bgfx::copy(pixels, w * h * 4);
	stbi_image_free(pixels);

	bgfx::TextureHandle th = bgfx::createTexture2D(
		(uint16_t)w, (uint16_t)h, false, 1,
		bgfx::TextureFormat::RGBA8,
		BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
		mem
	);

	if (!bgfx::isValid(th))
		return 0;

	Rml::TextureHandle handle = m_nextTexId++;
	m_textures[handle] = th;
	return handle;
}

Rml::TextureHandle RmlBgfxRenderer::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions)
{
	int w = source_dimensions.x;
	int h = source_dimensions.y;

	const bgfx::Memory* mem = bgfx::copy(source.data(), w * h * 4);

	bgfx::TextureHandle th = bgfx::createTexture2D(
		(uint16_t)w, (uint16_t)h, false, 1,
		bgfx::TextureFormat::RGBA8,
		BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT,
		mem
	);

	if (!bgfx::isValid(th))
		return 0;

	Rml::TextureHandle handle = m_nextTexId++;
	m_textures[handle] = th;
	return handle;
}

void RmlBgfxRenderer::ReleaseTexture(Rml::TextureHandle texture)
{
	auto it = m_textures.find(texture);
	if (it != m_textures.end())
	{
		if (bgfx::isValid(it->second))
			bgfx::destroy(it->second);
		m_textures.erase(it);
	}
}

// ---- Scissor ----

void RmlBgfxRenderer::EnableScissorRegion(bool enable)
{
	m_scissorEnabled = enable;
}

void RmlBgfxRenderer::SetScissorRegion(Rml::Rectanglei region)
{
	m_scissorRegion = region;
}

// ---- Transform ----

void RmlBgfxRenderer::SetTransform(const Rml::Matrix4f* transform)
{
	if (transform)
	{
		m_hasTransform = true;
		// RmlUi Matrix4f is column-major by default; bgfx setTransform expects column-major
		memcpy(m_transform, transform->data(), sizeof(float) * 16);
	}
	else
	{
		m_hasTransform = false;
	}
}
