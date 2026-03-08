#pragma once

// bgfx-based renderer replacing C4JRender (4J_Render.lib)
// safe to include from stdafx.h — no bgfx headers needed
// provides full C4JRender API so existing code can switch via macro

#include <cstdint>
#include <cstdlib>
#include <cstring>

class SDLWindow;

class ImageFileBuffer
{
public:
	enum EImageType { e_typePNG, e_typeJPG };
	EImageType m_type;
	void* m_pBuffer;
	int m_bufferSize;
	int GetType() { return m_type; }
	void* GetBufferPointer() { return m_pBuffer; }
	int GetBufferSize() { return m_bufferSize; }
	void Release() { free(m_pBuffer); m_pBuffer = nullptr; }
	bool Allocated() { return m_pBuffer != nullptr; }
};

typedef struct {
	int Width;
	int Height;
} D3DXIMAGE_INFO;

typedef struct _XSOCIAL_PREVIEWIMAGE {
	unsigned char* pBytes;
	unsigned long Pitch;
	unsigned long Width;
	unsigned long Height;
} XSOCIAL_PREVIEWIMAGE, *PXSOCIAL_PREVIEWIMAGE;

class BgfxRenderer
{
public:
	bool Init(SDLWindow& window, int width, int height);
	void Shutdown();
	void Resize(int width, int height);

	void Tick();
	void UpdateGamma(unsigned short usGamma);
	void StartFrame();
	void DoScreenGrabOnNextPresent();
	void Present();
	void Clear(int flags, void* pRect = nullptr);
	void SetClearColour(const float colourRGBA[4]);
	bool IsWidescreen();
	bool IsHiDef();
	void CaptureThumbnail(ImageFileBuffer* pngOut);
	void CaptureScreen(ImageFileBuffer* jpgOut, XSOCIAL_PREVIEWIMAGE* previewOut);
	void BeginConditionalSurvey(int identifier);
	void EndConditionalSurvey();
	void BeginConditionalRendering(int identifier);
	void EndConditionalRendering();

	// legacy D3D11 init — no-op, we use Init()
	void Initialise(void* pDevice, void* pSwapChain);
	void InitialiseContext();

	void MatrixMode(int type);
	void MatrixSetIdentity();
	void MatrixTranslate(float x, float y, float z);
	void MatrixRotate(float angle, float x, float y, float z);
	void MatrixScale(float x, float y, float z);
	void MatrixPerspective(float fovy, float aspect, float zNear, float zFar);
	void MatrixOrthogonal(float left, float right, float bottom, float top, float zNear, float zFar);
	void MatrixPop();
	void MatrixPush();
	void MatrixMult(float* mat);
	const float* MatrixGet(int type);
	void Set_matrixDirty();

	typedef enum
	{
		VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1,
		VERTEX_TYPE_COMPRESSED,
		VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT,
		VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_TEXGEN,
		VERTEX_TYPE_COUNT
	} eVertexType;

	typedef enum
	{
		PIXEL_SHADER_TYPE_STANDARD,
		PIXEL_SHADER_TYPE_PROJECTION,
		PIXEL_SHADER_TYPE_FORCELOD,
		PIXEL_SHADER_COUNT
	} ePixelShaderType;

	typedef enum
	{
		VIEWPORT_TYPE_FULLSCREEN,
		VIEWPORT_TYPE_SPLIT_TOP,
		VIEWPORT_TYPE_SPLIT_BOTTOM,
		VIEWPORT_TYPE_SPLIT_LEFT,
		VIEWPORT_TYPE_SPLIT_RIGHT,
		VIEWPORT_TYPE_QUADRANT_TOP_LEFT,
		VIEWPORT_TYPE_QUADRANT_TOP_RIGHT,
		VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT,
		VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT,
	} eViewportType;

	typedef enum
	{
		PRIMITIVE_TYPE_TRIANGLE_LIST,
		PRIMITIVE_TYPE_TRIANGLE_STRIP,
		PRIMITIVE_TYPE_TRIANGLE_FAN,
		PRIMITIVE_TYPE_QUAD_LIST,
		PRIMITIVE_TYPE_LINE_LIST,
		PRIMITIVE_TYPE_LINE_STRIP,
		PRIMITIVE_TYPE_COUNT
	} ePrimitiveType;

	void DrawVertices(ePrimitiveType PrimitiveType, int count, void* dataIn, eVertexType vType, ePixelShaderType psType);
	void DrawVertexBuffer(ePrimitiveType PrimitiveType, int count, void* buffer, eVertexType vType, ePixelShaderType psType);

	void CBuffLockStaticCreations();
	int  CBuffCreate(int count);
	void CBuffDelete(int first, int count);
	void CBuffStart(int index, bool full = false);
	void CBuffClear(int index);
	int  CBuffSize(int index);
	void CBuffEnd();
	bool CBuffCall(int index, bool full = true);
	void CBuffTick();
	void CBuffDeferredModeStart();
	void CBuffDeferredModeEnd();

	typedef enum
	{
		TEXTURE_FORMAT_RxGyBzAw,
		MAX_TEXTURE_FORMATS
	} eTextureFormat;

	// int-based handles matching C4JRender
	int  TextureCreate();
	void TextureFree(int idx);
	void TextureBind(int idx);
	void TextureBindVertex(int idx);
	void TextureSetTextureLevels(int levels);
	int  TextureGetTextureLevels();
	void TextureData(int width, int height, void* data, int level, eTextureFormat format = TEXTURE_FORMAT_RxGyBzAw);
	void TextureDataUpdate(int xoffset, int yoffset, int width, int height, void* data, int level);
	void TextureSetParam(int param, int value);
	void TextureDynamicUpdateStart();
	void TextureDynamicUpdateEnd();
	long LoadTextureData(const char* szFilename, D3DXIMAGE_INFO* pSrcInfo, int** ppDataOut);
	long LoadTextureData(unsigned char* pbData, unsigned long dwBytes, D3DXIMAGE_INFO* pSrcInfo, int** ppDataOut);
	long SaveTextureData(const char* szFilename, D3DXIMAGE_INFO* pSrcInfo, int* ppDataOut);
	long SaveTextureDataToMemory(void* pOutput, int outputCapacity, int* outputLength, int width, int height, int* ppDataIn);
	void TextureGetStats();
	void* TextureGetTexture(int idx);

	void StateSetColour(float r, float g, float b, float a);
	void StateSetDepthMask(bool enable);
	void StateSetBlendEnable(bool enable);
	void StateSetBlendFunc(int src, int dst);
	void StateSetBlendFactor(unsigned int colour);
	void StateSetAlphaFunc(int func, float param);
	void StateSetDepthFunc(int func);
	void StateSetFaceCull(bool enable);
	void StateSetFaceCullCW(bool enable);
	void StateSetLineWidth(float width);
	void StateSetWriteEnable(bool red, bool green, bool blue, bool alpha);
	void StateSetDepthTestEnable(bool enable);
	void StateSetAlphaTestEnable(bool enable);
	void StateSetDepthSlopeAndBias(float slope, float bias);
	void StateSetFogEnable(bool enable);
	void StateSetFogMode(int mode);
	void StateSetFogNearDistance(float dist);
	void StateSetFogFarDistance(float dist);
	void StateSetFogDensity(float density);
	void StateSetFogColour(float red, float green, float blue);
	void StateSetLightingEnable(bool enable);
	void StateSetVertexTextureUV(float u, float v);
	void StateSetLightColour(int light, float red, float green, float blue);
	void StateSetLightAmbientColour(float red, float green, float blue);
	void StateSetLightDirection(int light, float x, float y, float z);
	void StateSetLightEnable(int light, bool enable);
	void StateSetViewport(eViewportType viewportType);
	void StateSetEnableViewportClipPlanes(bool enable);
	void StateSetTexGenCol(int col, float x, float y, float z, float w, bool eyeSpace);
	void StateSetStencil(int Function, uint8_t stencil_ref, uint8_t stencil_func_mask, uint8_t stencil_write_mask);
	void StateSetForceLOD(int LOD);

	void BeginEvent(const wchar_t* eventName);
	void EndEvent();

	void Suspend();
	bool Suspended();
	void Resume();

	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }

private:
	int m_width = 1920;
	int m_height = 1080;

	float m_clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	static const int STACK_DEPTH = 32;
	float m_modelViewStack[STACK_DEPTH][16];
	float m_projectionStack[STACK_DEPTH][16];
	float m_textureMatrix[16];
	int m_mvTop = 0;
	int m_projTop = 0;
	int m_currentMode = 0; // 0=mv, 1=proj, 2=tex

	float* GetCurrentMatrix();

	static const int MAX_TEXTURES = 4096;
	struct TextureSlot
	{
		bool used = false;
		int width = 0;
		int height = 0;
		int levels = 1;
		uint16_t bgfxHandle = 0xFFFF; // bgfx::TextureHandle is 2 bytes
	};
	TextureSlot m_texSlots[MAX_TEXTURES];
	int m_boundTexture = -1;
	int m_textureLevels = 1;

	int AllocTextureSlot();
	void FreeTextureSlot(int idx);

	bool m_blendEnabled = false;
	bool m_depthTestEnabled = true;
	bool m_depthWriteEnabled = true;
	bool m_cullEnabled = false;
	bool m_cullCW = true;
	bool m_fogEnabled = false;
	bool m_lightingEnabled = false;
	bool m_suspended = false;
	float m_colour[4] = { 1, 1, 1, 1 };
	float m_fogColor[3] = { 0, 0, 0 };
	float m_fogNear = 0, m_fogFar = 100;
	int m_blendSrc = 1; // GL_ONE
	int m_blendDst = 0; // GL_ZERO
	int m_depthFunc = 4; // GL_LEQUAL
	float m_alphaRef = 0.0f;
	bool m_alphaTestEnabled = false;

	// uint16_t to avoid pulling in bgfx headers
	uint16_t m_texColorUniform = 0xFFFF;
	uint16_t m_whiteTexture = 0xFFFF;
	uint16_t m_program = 0xFFFF;
	uint16_t m_u_colorUniform = 0xFFFF;
	uint16_t m_u_fogParamsUniform = 0xFFFF;
	uint16_t m_u_fogColorUniform = 0xFFFF;
	bool m_shadersLoaded = false;

	void LoadShaders();
	uint64_t BuildBgfxState() const;
	void ConvertQuadsToTriangles(const void* quadData, int quadVertCount, int vertexStride, void* triOut, int& triVertCount) const;

	void SubmitDraw(ePrimitiveType primType, int count, void* dataIn, eVertexType vType,
					int texIdx, float mvMatrix[16], float projMatrix[16]);

	struct RecordedDraw
	{
		ePrimitiveType primType;
		eVertexType vType;
		int vertCount;
		int texIdx;
		float mvMatrix[16];
		float projMatrix[16];
		bool blendEnabled;
		int blendSrc, blendDst;
		bool depthTestEnabled;
		bool depthWriteEnabled;
		int depthFunc;
		bool cullEnabled;
		bool cullCW;
		bool alphaTestEnabled;
		float alphaRef;
		uint8_t* vertexData; // heap-allocated copy
	};

	struct CmdBuffer
	{
		bool allocated = false;
		RecordedDraw* draws = nullptr;
		int drawCount = 0;
		int drawCapacity = 0;
		int totalBytes = 0;

		void Clear();
		void AddDraw(const RecordedDraw& draw);
		~CmdBuffer();
	};

	static const int MAX_CMD_BUFFERS = 8192;
	CmdBuffer* m_cmdBuffers = nullptr; // lazy-allocated array
	int m_nextCmdBufferId = 1;
	int m_recordingBuffer = -1; // -1 = not recording
	bool m_deferredMode = false;

	void EnsureCmdBuffers();
};

// global instance
extern BgfxRenderer g_BgfxRenderer;

// redirect RenderManager to g_BgfxRenderer
#define RenderManager g_BgfxRenderer

// redirect C4JRender:: enum references
typedef BgfxRenderer C4JRender;

// GL constants (from original 4J_Render.h, used throughout game code)

const int GL_MODELVIEW_MATRIX = 0;
const int GL_PROJECTION_MATRIX = 1;
const int GL_MODELVIEW = 0;
const int GL_PROJECTION = 1;
const int GL_TEXTURE = 2;

const int GL_S = 0;
const int GL_T = 1;
const int GL_R = 2;
const int GL_Q = 3;

const int GL_TEXTURE_GEN_S = 0;
const int GL_TEXTURE_GEN_T = 1;
const int GL_TEXTURE_GEN_Q = 2;
const int GL_TEXTURE_GEN_R = 3;

const int GL_TEXTURE_GEN_MODE = 0;
const int GL_OBJECT_LINEAR = 0;
const int GL_EYE_LINEAR = 1;
const int GL_OBJECT_PLANE = 0;
const int GL_EYE_PLANE = 1;

const int GL_TEXTURE_2D = 1;
const int GL_BLEND = 2;
const int GL_CULL_FACE = 3;
const int GL_ALPHA_TEST = 4;
const int GL_DEPTH_TEST = 5;
const int GL_FOG = 6;
const int GL_LIGHTING = 7;
const int GL_LIGHT0 = 8;
const int GL_LIGHT1 = 9;

const int CLEAR_DEPTH_FLAG = 1;
const int CLEAR_COLOUR_FLAG = 2;
const int GL_DEPTH_BUFFER_BIT = CLEAR_DEPTH_FLAG;
const int GL_COLOR_BUFFER_BIT = CLEAR_COLOUR_FLAG;

// blend factors (hardcoded D3D11 enum values to avoid dependency)
const int GL_SRC_ALPHA = 5;
const int GL_ONE_MINUS_SRC_ALPHA = 6;
const int GL_ONE = 2;
const int GL_ZERO = 1;
const int GL_DST_ALPHA = 7;
const int GL_SRC_COLOR = 3;
const int GL_DST_COLOR = 9;
const int GL_ONE_MINUS_DST_COLOR = 10;
const int GL_ONE_MINUS_SRC_COLOR = 4;
const int GL_CONSTANT_ALPHA = 14;
const int GL_ONE_MINUS_CONSTANT_ALPHA = 15;

// comparison funcs
const int GL_GREATER = 5;
const int GL_EQUAL = 3;
const int GL_LEQUAL = 4;
const int GL_GEQUAL = 7;
const int GL_ALWAYS = 8;

const int GL_TEXTURE_MIN_FILTER = 1;
const int GL_TEXTURE_MAG_FILTER = 2;
const int GL_TEXTURE_WRAP_S = 3;
const int GL_TEXTURE_WRAP_T = 4;

const int GL_NEAREST = 0;
const int GL_LINEAR = 1;
const int GL_EXP = 2;
const int GL_NEAREST_MIPMAP_LINEAR = 0;

const int GL_CLAMP = 0;
const int GL_REPEAT = 1;

const int GL_FOG_START = 1;
const int GL_FOG_END = 2;
const int GL_FOG_MODE = 3;
const int GL_FOG_DENSITY = 4;
const int GL_FOG_COLOR = 5;

const int GL_POSITION = 1;
const int GL_AMBIENT = 2;
const int GL_DIFFUSE = 3;
const int GL_SPECULAR = 4;

const int GL_LIGHT_MODEL_AMBIENT = 1;

const int GL_LINES = C4JRender::PRIMITIVE_TYPE_LINE_LIST;
const int GL_LINE_STRIP = C4JRender::PRIMITIVE_TYPE_LINE_STRIP;
const int GL_QUADS = C4JRender::PRIMITIVE_TYPE_QUAD_LIST;
const int GL_TRIANGLE_FAN = C4JRender::PRIMITIVE_TYPE_TRIANGLE_FAN;
const int GL_TRIANGLE_STRIP = C4JRender::PRIMITIVE_TYPE_TRIANGLE_STRIP;
