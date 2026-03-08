#pragma once // 4J

// gdraw_d3d11.h stub - all functions are inline no-ops
// Original: gdraw_d3d11.h - author: Fabian Giesen - copyright 2011 RAD Game Tools

#define IDOC

typedef enum gdraw_d3d11_resourcetype
{
   GDRAW_D3D11_RESOURCE_rendertarget,
   GDRAW_D3D11_RESOURCE_texture,
   GDRAW_D3D11_RESOURCE_vertexbuffer,
   GDRAW_D3D11_RESOURCE_dynbuffer,

   GDRAW_D3D11_RESOURCE__count,
} gdraw_d3d11_resourcetype;

IDOC inline int gdraw_D3D11_SetResourceLimits(gdraw_d3d11_resourcetype type, S32 num_handles, S32 num_bytes) { (void)type; (void)num_handles; (void)num_bytes; return 1; }

IDOC inline GDrawFunctions * gdraw_D3D11_CreateContext(ID3D11Device *dev, ID3D11DeviceContext *ctx, S32 w, S32 h) { (void)dev; (void)ctx; (void)w; (void)h; return 0; }

IDOC inline void gdraw_D3D11_DestroyContext(void) {}

IDOC inline void gdraw_D3D11_SetErrorHandler(void (__cdecl *error_handler)(HRESULT hr)) { (void)error_handler; }

IDOC inline void gdraw_D3D11_SetRendertargetSize(S32 w, S32 h) { (void)w; (void)h; }

IDOC inline void gdraw_D3D11_SetTileOrigin(ID3D11RenderTargetView *main_rt, ID3D11DepthStencilView *main_ds,
                                      ID3D11ShaderResourceView *non_msaa_rt, S32 x, S32 y) { (void)main_rt; (void)main_ds; (void)non_msaa_rt; (void)x; (void)y; }

IDOC inline void gdraw_D3D11_NoMoreGDrawThisFrame(void) {}

IDOC inline void gdraw_D3D11_PreReset(void) {}

IDOC inline void gdraw_D3D11_PostReset(void) {}

IDOC inline void RADLINK gdraw_D3D11_BeginCustomDraw_4J(IggyCustomDrawCallbackRegion *Region, F32 mat[16]) { (void)Region; (void)mat; }
IDOC inline void RADLINK gdraw_D3D11_CalculateCustomDraw_4J(IggyCustomDrawCallbackRegion *Region, F32 mat[16]) { (void)Region; (void)mat; }
IDOC inline void RADLINK gdraw_D3D11_BeginCustomDraw(IggyCustomDrawCallbackRegion *Region, F32 mat[4][4]) { (void)Region; (void)mat; }

IDOC inline void RADLINK gdraw_D3D11_EndCustomDraw(IggyCustomDrawCallbackRegion *Region) { (void)Region; }

IDOC inline void RADLINK gdraw_D3D11_GetResourceUsageStats(gdraw_d3d11_resourcetype type, S32 *handles_used, S32 *bytes_used) { (void)type; if(handles_used) *handles_used = 0; if(bytes_used) *bytes_used = 0; }

IDOC inline GDrawTexture *gdraw_D3D11_WrappedTextureCreate(ID3D11ShaderResourceView *tex_view) { (void)tex_view; return 0; }

IDOC inline void gdraw_D3D11_WrappedTextureChange(GDrawTexture *tex, ID3D11ShaderResourceView *tex_view) { (void)tex; (void)tex_view; }

IDOC inline void gdraw_D3D11_WrappedTextureDestroy(GDrawTexture *tex) { (void)tex; }

inline GDrawTexture * RADLINK gdraw_D3D11_MakeTextureFromResource(U8 *resource_file, S32 length, IggyFileTextureRaw *texture) { (void)resource_file; (void)length; (void)texture; return 0; }
inline void RADLINK gdraw_D3D11_DestroyTextureFromResource(GDrawTexture *tex) { (void)tex; }

// 4J added
inline void RADLINK gdraw_D3D11_setViewport_4J() {}
