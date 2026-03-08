// Iggy stub header - all functions are inline no-ops
// Original: Iggy -- Copyright 2008-2013 RAD Game Tools

#ifndef __RAD_INCLUDE_IGGY_H__
#define __RAD_INCLUDE_IGGY_H__

#include <stdlib.h> // size_t
#include <string.h> // memset

#define IggyVersion "1.2.30"
#define IggyFlashVersion "9,1,2,30"

#include "rrcore.h"   // base data types, macros

RADDEFSTART

#ifndef IGGY_GDRAW_SHARED_TYPEDEF
   #define IGGY_GDRAW_SHARED_TYPEDEF
   typedef struct GDrawFunctions GDrawFunctions;
   typedef struct GDrawTexture   GDrawTexture;
#endif

#define IDOCN

typedef enum IggyResult
{
   IGGY_RESULT_SUCCESS = 0,

   IGGY_RESULT_Warning_None                            =   0,

   IGGY_RESULT_Warning_Misc                            = 100,
   IGGY_RESULT_Warning_GDraw                           = 101,
   IGGY_RESULT_Warning_ProgramFlow                     = 102,
   IGGY_RESULT_Warning_Actionscript                    = 103,
   IGGY_RESULT_Warning_Graphics                        = 104,
   IGGY_RESULT_Warning_Font                            = 105,
   IGGY_RESULT_Warning_Timeline                        = 106,
   IGGY_RESULT_Warning_Library                         = 107,
   IGGY_RESULT_Warning_ValuePath                       = 108,
   IGGY_RESULT_Warning_Audio                           = 109,

   IGGY_RESULT_Warning_CannotSustainFrameRate          = 201,
   IGGY_RESULT_Warning_ThrewException                  = 202,

   IGGY_RESULT_Error_Threshhold                        = 400,

   IGGY_RESULT_Error_Misc                              = 400,
   IGGY_RESULT_Error_GDraw                             = 401,
   IGGY_RESULT_Error_ProgramFlow                       = 402,
   IGGY_RESULT_Error_Actionscript                      = 403,
   IGGY_RESULT_Error_Graphics                          = 404,
   IGGY_RESULT_Error_Font                              = 405,
   IGGY_RESULT_Error_Create                            = 406,
   IGGY_RESULT_Error_Library                           = 407,
   IGGY_RESULT_Error_ValuePath                         = 408,
   IGGY_RESULT_Error_Audio                             = 409,

   IGGY_RESULT_Error_Internal                          = 499,

   IGGY_RESULT_Error_InvalidIggy                       = 501,
   IGGY_RESULT_Error_InvalidArgument                   = 502,
   IGGY_RESULT_Error_InvalidEntity                     = 503,
   IGGY_RESULT_Error_UndefinedEntity                   = 504,

   IGGY_RESULT_Error_OutOfMemory                       = 1001,
} IggyResult;

typedef enum IggyDatatype
{
   IGGY_DATATYPE__invalid_request,
   IGGY_DATATYPE_undefined,
   IGGY_DATATYPE_null,
   IGGY_DATATYPE_boolean,
   IGGY_DATATYPE_number,
   IGGY_DATATYPE_string_UTF8,
   IGGY_DATATYPE_string_UTF16,
   IGGY_DATATYPE_fastname,
   IGGY_DATATYPE_valuepath,
   IGGY_DATATYPE_valueref,
   IGGY_DATATYPE_array,
   IGGY_DATATYPE_object,
   IGGY_DATATYPE_displayobj,
   IGGY_DATATYPE_xml,
   IGGY_DATATYPE_namespace,
   IGGY_DATATYPE_qname,
   IGGY_DATATYPE_function,
   IGGY_DATATYPE_class,
} IggyDatatype;

#ifdef __RADWIN__
#include <stddef.h>
IDOCN typedef wchar_t IggyUTF16;
#else
typedef unsigned short IggyUTF16;
#endif

typedef struct IggyStringUTF16
{
   IggyUTF16 *string;
   S32  length;
} IggyStringUTF16;

typedef struct IggyStringUTF8
{
   char *string;
   S32  length;
} IggyStringUTF8;

typedef UINTa IggyName;
typedef struct IggyValuePath IggyValuePath;
typedef void *IggyValueRef;
typedef UINTa IggyTempRef;

typedef struct IggyDataValue
{
   S32 type;
   #ifdef __RAD64__
   S32 padding;
   #endif
   IggyTempRef temp_ref;
   union {
      IggyStringUTF16 string16;
      IggyStringUTF8  string8;
      F64             number;
      rrbool          boolval;
      IggyName        fastname;
      void          * userdata;
      IggyValuePath * valuepath;
      IggyValueRef    valueref;
   };
} IggyDataValue;

typedef struct IggyExternalFunctionCallUTF16
{
   IggyStringUTF16 function_name;
   S32 num_arguments;
   S32 padding;
   IggyDataValue arguments[1];
} IggyExternalFunctionCallUTF16;

typedef struct IggyExternalFunctionCallUTF8
{
   IggyStringUTF8 function_name;
   S32 num_arguments;
   S32 padding;
   IggyDataValue arguments[1];
} IggyExternalFunctionCallUTF8;

typedef void * RADLINK Iggy_AllocateFunction(void *alloc_callback_user_data, size_t size_requested, size_t *size_returned);
typedef void   RADLINK Iggy_DeallocateFunction(void *alloc_callback_user_data, void *ptr);

typedef struct IggyAllocator
{
   void                    *user_callback_data;
   Iggy_AllocateFunction   *mem_alloc;
   Iggy_DeallocateFunction *mem_free;
   #ifndef __RAD64__
   void                    *struct_padding;
   #endif
} IggyAllocator;

// Stub: IggyInit / IggyShutdown
inline void RADLINK IggyInit(IggyAllocator *allocator) { (void)allocator; }
inline void RADLINK IggyShutdown(void) {}

typedef enum IggyConfigureBoolName
{
   IGGY_CONFIGURE_BOOL_StartupExceptionsAreWarnings,
   IGGY_CONFIGURE_BOOL_IgnoreFlashVersion,
   IGGY_CONFIGURE_BOOL_NeverDelayGotoProcessing,
   IGGY_CONFIGURE_BOOL_SuppressAntialiasingOnAllBitmaps,
   IGGY_CONFIGURE_BOOL_SuppressAntialiasingOn9SliceBitmaps,
} IggyConfigureBoolName;

inline void RADLINK IggyConfigureBool(IggyConfigureBoolName prop, rrbool value) { (void)prop; (void)value; }

typedef enum
{
   IGGY_VERSION_1_0_21 = 1,
   IGGY_VERSION_1_0_24 = 3,
   IGGY_VERSION_1_1_1  = 5,
   IGGY_VERSION_1_1_8  = 7,
   IGGY_VERSION_1_2_28 = 9,
   IGGY_VERSION_default=0x7fffffff,
} IggyVersionNumber;

typedef enum
{
   IGGY_VERSIONED_BEHAVIOR_movieclip_gotoand=128,
   IGGY_VERSIONED_BEHAVIOR_textfield_position=129,
   IGGY_VERSIONED_BEHAVIOR_bitmap_smoothing=130,
   IGGY_VERSIONED_BEHAVIOR_textfield_autoscroll=131,
   IGGY_VERSIONED_BEHAVIOR_fast_text_effects=132,
} IggyVersionedBehaviorName;

inline void RADLINK IggyConfigureVersionedBehavior(IggyVersionedBehaviorName prop, IggyVersionNumber value) { (void)prop; (void)value; }

typedef enum IggyTelemetryAmount
{
   IGGY_TELEMETRY_normal,
   IGGY_TELEMETRY_internal,
} IggyTelemetryAmount;

inline void RADLINK IggyUseTmLite(void * context, IggyTelemetryAmount amount) { (void)context; (void)amount; }
inline void RADLINK IggyUseTelemetry(void * context, IggyTelemetryAmount amount) { (void)context; (void)amount; }

// Translation types
typedef struct
{
   IggyUTF16 *object_name;
   rrbool     autosize;
   F32        width;
   F32        height;
   rrbool     is_html_text;
} IggyTextfieldInfo;

typedef void   RADLINK Iggy_TranslationFreeFunction(void *callback_data, void *data, S32 length);
typedef rrbool RADLINK Iggy_TranslateFunctionUTF16(void *callback_data, IggyStringUTF16 *src, IggyStringUTF16 *dest);
typedef rrbool RADLINK Iggy_TranslateFunctionUTF8(void *callback_data, IggyStringUTF8 *src, IggyStringUTF8 *dest);
typedef rrbool RADLINK Iggy_TextfieldTranslateFunctionUTF16(void *callback_data, IggyStringUTF16 *src, IggyStringUTF16 *dest, IggyTextfieldInfo *textfield);
typedef rrbool RADLINK Iggy_TextfieldTranslateFunctionUTF8(void *callback_data, IggyStringUTF8 *src, IggyStringUTF8 *dest, IggyTextfieldInfo *textfield);

inline void RADLINK IggySetLoadtimeTranslationFunction(Iggy_TranslateFunctionUTF16 *func, void *callback_data, Iggy_TranslationFreeFunction *freefunc, void *free_callback_data) { (void)func; (void)callback_data; (void)freefunc; (void)free_callback_data; }
inline void RADLINK IggySetLoadtimeTranslationFunctionUTF16(Iggy_TranslateFunctionUTF16 *func, void *callback_data, Iggy_TranslationFreeFunction *freefunc, void *free_callback_data) { (void)func; (void)callback_data; (void)freefunc; (void)free_callback_data; }
inline void RADLINK IggySetLoadtimeTranslationFunctionUTF8(Iggy_TranslateFunctionUTF8 *func, void *callback_data, Iggy_TranslationFreeFunction *freefunc, void *free_callback_data) { (void)func; (void)callback_data; (void)freefunc; (void)free_callback_data; }
inline void RADLINK IggySetRuntimeTranslationFunction(Iggy_TranslateFunctionUTF16 *func, void *callback_data, Iggy_TranslationFreeFunction *freefunc, void *free_callback_data) { (void)func; (void)callback_data; (void)freefunc; (void)free_callback_data; }
inline void RADLINK IggySetRuntimeTranslationFunctionUTF16(Iggy_TranslateFunctionUTF16 *func, void *callback_data, Iggy_TranslationFreeFunction *freefunc, void *free_callback_data) { (void)func; (void)callback_data; (void)freefunc; (void)free_callback_data; }
inline void RADLINK IggySetRuntimeTranslationFunctionUTF8(Iggy_TranslateFunctionUTF8 *func, void *callback_data, Iggy_TranslationFreeFunction *freefunc, void *free_callback_data) { (void)func; (void)callback_data; (void)freefunc; (void)free_callback_data; }
inline void RADLINK IggySetTextfieldTranslationFunctionUTF16(Iggy_TextfieldTranslateFunctionUTF16 *func, void *callback_data, Iggy_TranslationFreeFunction *freefunc, void *free_callback_data) { (void)func; (void)callback_data; (void)freefunc; (void)free_callback_data; }
inline void RADLINK IggySetTextfieldTranslationFunctionUTF8(Iggy_TextfieldTranslateFunctionUTF8 *func, void *callback_data, Iggy_TranslationFreeFunction *freefunc, void *free_callback_data) { (void)func; (void)callback_data; (void)freefunc; (void)free_callback_data; }

typedef enum
{
   IGGY_LANG_default,
   IGGY_LANG_ja,
   IGGY_LANG_ja_flash,
} IggyLanguageCode;

inline void RADLINK IggySetLanguage(IggyLanguageCode lang) { (void)lang; }

// Playback types
typedef struct Iggy Iggy;
typedef S32 IggyLibrary;

typedef void   RADLINK Iggy_TraceFunctionUTF16(void *user_callback_data, Iggy *player, IggyUTF16 const *utf16_string, S32 length_in_16bit_chars);
typedef void   RADLINK Iggy_TraceFunctionUTF8(void *user_callback_data, Iggy *player, char const *utf8_string, S32 length_in_bytes);
typedef void   RADLINK Iggy_WarningFunction(void *user_callback_data, Iggy *player, IggyResult error_code, char const *error_message);

typedef struct
{
   S32  total_storage_in_bytes;
   S32  stack_size_in_bytes;
   S32  young_heap_size_in_bytes;
   S32  old_heap_size_in_bytes;
   S32  remembered_set_size_in_bytes;
   S32  greylist_size_in_bytes;
   S32  rootstack_size_in_bytes;
   S32  padding;
} IggyPlayerGCSizes;

typedef struct
{
   IggyAllocator allocator;
   IggyPlayerGCSizes gc;
   char *filename;
   char *user_name;
   rrbool load_in_place;
   rrbool did_load_in_place;
} IggyPlayerConfig;

inline Iggy * RADLINK IggyPlayerCreateFromFileAndPlay(char const *filename, IggyPlayerConfig const*config) { (void)filename; (void)config; return 0; }
inline Iggy * RADLINK IggyPlayerCreateFromMemory(void const *data, U32 data_size_in_bytes, IggyPlayerConfig *config) { (void)data; (void)data_size_in_bytes; (void)config; return 0; }

#define IGGY_INVALID_LIBRARY -1

inline IggyLibrary RADLINK IggyLibraryCreateFromMemory(char const *url_utf8_null_terminated, void const *data, U32 data_size_in_bytes, IggyPlayerConfig *config) { (void)url_utf8_null_terminated; (void)data; (void)data_size_in_bytes; (void)config; return IGGY_INVALID_LIBRARY; }
inline IggyLibrary RADLINK IggyLibraryCreateFromMemoryUTF16(IggyUTF16 const *url_utf16_null_terminated, void const *data, U32 data_size_in_bytes, IggyPlayerConfig *config) { (void)url_utf16_null_terminated; (void)data; (void)data_size_in_bytes; (void)config; return IGGY_INVALID_LIBRARY; }

inline void RADLINK IggyPlayerDestroy(Iggy *player) { (void)player; }
inline void RADLINK IggyLibraryDestroy(IggyLibrary lib) { (void)lib; }
inline void RADLINK IggySetWarningCallback(Iggy_WarningFunction *error, void *user_callback_data) { (void)error; (void)user_callback_data; }
inline void RADLINK IggySetTraceCallbackUTF8(Iggy_TraceFunctionUTF8 *trace_utf8, void *user_callback_data) { (void)trace_utf8; (void)user_callback_data; }
inline void RADLINK IggySetTraceCallbackUTF16(Iggy_TraceFunctionUTF16 *trace_utf16, void *user_callback_data) { (void)trace_utf16; (void)user_callback_data; }

typedef struct IggyProperties
{
   S32  movie_width_in_pixels;
   S32  movie_height_in_pixels;
   F32  movie_frame_rate_current_in_fps;
   F32  movie_frame_rate_from_file_in_fps;
   S32  frames_passed;
   S32  swf_major_version_number;
   F64  time_passed_in_seconds;
   F64  seconds_since_last_tick;
   F64  seconds_per_drawn_frame;
} IggyProperties;

static IggyProperties _iggy_stub_properties;
inline IggyProperties * RADLINK IggyPlayerProperties(Iggy *player) { (void)player; memset(&_iggy_stub_properties, 0, sizeof(_iggy_stub_properties)); return &_iggy_stub_properties; }

typedef enum
{
   IGGY_PAUSE_continue_audio,
   IGGY_PAUSE_pause_audio,
   IGGY_PAUSE_stop_audio
} IggyAudioPauseMode;

inline void * RADLINK IggyPlayerGetUserdata(Iggy *player) { (void)player; return 0; }
inline void RADLINK IggyPlayerSetUserdata(Iggy *player, void *userdata) { (void)player; (void)userdata; }

inline void RADLINK IggyPlayerInitializeAndTickRS(Iggy *player) { (void)player; }
inline rrbool RADLINK IggyPlayerReadyToTick(Iggy *player) { (void)player; return 0; }
inline void RADLINK IggyPlayerTickRS(Iggy *player) { (void)player; }
inline void RADLINK IggyPlayerPause(Iggy *player, IggyAudioPauseMode pause_audio) { (void)player; (void)pause_audio; }
inline void RADLINK IggyPlayerPlay(Iggy *player) { (void)player; }
inline void RADLINK IggyPlayerSetFrameRate(Iggy *player, F32 frame_rate_in_fps) { (void)player; (void)frame_rate_in_fps; }
inline void RADLINK IggyPlayerGotoFrameRS(Iggy *f, S32 frame, rrbool stop) { (void)f; (void)frame; (void)stop; }

#ifndef __RAD_HIGGYEXP_
#define __RAD_HIGGYEXP_
typedef void * HIGGYEXP;
#endif

#ifndef __RAD_HIGGYPERFMON_
#define __RAD_HIGGYPERFMON_
typedef void * HIGGYPERFMON;
#endif

IDOCN typedef void RADLINK iggyexp_detach_callback(void *ptr);

IDOCN typedef struct
{
   U64 tick_ticks;
   U64 draw_ticks;
} IggyPerfmonStats;

IDOCN typedef struct
{
   void (RADLINK *get_stats)(Iggy* swf, IggyPerfmonStats* pdest);
   const char* (RADLINK *get_display_name)(Iggy* swf);
} IggyForPerfmonFunctions;

IDOCN typedef struct
{
   rrbool (RADLINK *connection_valid)(Iggy* swf, HIGGYEXP iggyexp);
   S32    (RADLINK *poll_command)(Iggy* swf, HIGGYEXP iggyexp, U8 **buffer);
   void   (RADLINK *send_command)(Iggy* swf, HIGGYEXP iggyexp, U8 command, void *buffer, S32 len);
   S32    (RADLINK *get_storage)(Iggy* swf, HIGGYEXP iggyexp, U8 **buffer);
   rrbool (RADLINK *attach)(Iggy* swf, HIGGYEXP iggyexp, iggyexp_detach_callback *cb, void *cbdata, IggyForPerfmonFunctions* pmf);
   rrbool (RADLINK *detach)(Iggy* swf, HIGGYEXP iggyexp);
   void   (RADLINK *draw_tile_hook)(Iggy* swf, HIGGYEXP iggyexp, GDrawFunctions* iggy_gdraw);
} IggyExpFunctions;

inline void RADLINK IggyInstallPerfmon(void *perfmon_context) { (void)perfmon_context; }
inline void RADLINK IggyUseExplorer(Iggy *swf, void *context) { (void)swf; (void)context; }
IDOCN inline void RADLINK IggyPlayerSendFrameToExplorer(Iggy *f) { (void)f; }

// Font types
typedef struct
{
   F32 ascent;
   F32 descent;
   F32 line_gap;
   F32 average_glyph_width_for_tab_stops;
   F32 largest_glyph_bbox_y1;
} IggyFontMetrics;

typedef struct
{
   F32 x0,y0, x1,y1;
   F32 advance;
} IggyGlyphMetrics;

typedef enum {
   IGGY_VERTEX_move  = 1,
   IGGY_VERTEX_line  = 2,
   IGGY_VERTEX_curve = 3,
} IggyShapeVertexType;

typedef struct
{
   F32 x,y;
   F32 cx,cy;
   U8 type;
   S8 padding;
   U16 f0;
   U16 f1;
   U16 line;
} IggyShapeVertex;

typedef struct
{
   IggyShapeVertex * vertices;
   S32               num_vertices;
   void            * user_context_for_free;
} IggyVectorShape;

typedef struct
{
   U8    *pixels_one_per_byte;
   S32    width_in_pixels;
   S32    height_in_pixels;
   S32    stride_in_bytes;
   S32    oversample;
   rrbool point_sample;
   S32    top_left_x;
   S32    top_left_y;
   F32    pixel_scale_correct;
   F32    pixel_scale_min;
   F32    pixel_scale_max;
   void * user_context_for_free;
} IggyBitmapCharacter;

typedef IggyFontMetrics * RADLINK IggyFontGetFontMetrics(void *user_context, IggyFontMetrics *metrics);

#define IGGY_GLYPH_INVALID              -1
typedef S32                RADLINK IggyFontGetCodepointGlyph(void *user_context, U32 codepoint);
typedef IggyGlyphMetrics * RADLINK IggyFontGetGlyphMetrics(void *user_context, S32 glyph, IggyGlyphMetrics *metrics);
typedef rrbool             RADLINK IggyFontIsGlyphEmpty(void *user_context, S32 glyph);
typedef F32                RADLINK IggyFontGetKerningForGlyphPair(void *user_context, S32 first_glyph, S32 second_glyph);

typedef void RADLINK IggyVectorFontGetGlyphShape(void *user_context, S32 glyph, IggyVectorShape *shape);
typedef void RADLINK IggyVectorFontFreeGlyphShape(void *user_context, S32 glyph, IggyVectorShape *shape);

typedef rrbool RADLINK IggyBitmapFontCanProvideBitmap(void *user_context, S32 glyph, F32 pixel_scale);
typedef rrbool RADLINK IggyBitmapFontGetGlyphBitmap(void *user_context, S32 glyph, F32 pixel_scale, IggyBitmapCharacter *bitmap);
typedef void RADLINK IggyBitmapFontFreeGlyphBitmap(void *user_context, S32 glyph, F32 pixel_scale, IggyBitmapCharacter *bitmap);

typedef struct
{
   IggyFontGetFontMetrics          *get_font_metrics;
   IggyFontGetCodepointGlyph       *get_glyph_for_codepoint;
   IggyFontGetGlyphMetrics         *get_glyph_metrics;
   IggyFontIsGlyphEmpty            *is_empty;
   IggyFontGetKerningForGlyphPair  *get_kerning;
   IggyVectorFontGetGlyphShape     *get_shape;
   IggyVectorFontFreeGlyphShape    *free_shape;
   S32                              num_glyphs;
   void *userdata;
} IggyVectorFontProvider;

typedef struct
{
   IggyFontGetFontMetrics          *get_font_metrics;
   IggyFontGetCodepointGlyph       *get_glyph_for_codepoint;
   IggyFontGetGlyphMetrics         *get_glyph_metrics;
   IggyFontIsGlyphEmpty            *is_empty;
   IggyFontGetKerningForGlyphPair  *get_kerning;
   IggyBitmapFontCanProvideBitmap  *can_bitmap;
   IggyBitmapFontGetGlyphBitmap    *get_bitmap;
   IggyBitmapFontFreeGlyphBitmap   *free_bitmap;
   S32                              num_glyphs;
   void *userdata;
} IggyBitmapFontProvider;

typedef struct
{
   IggyBitmapFontCanProvideBitmap  *can_bitmap;
   IggyBitmapFontGetGlyphBitmap    *get_bitmap;
   IggyBitmapFontFreeGlyphBitmap   *free_bitmap;
   void *userdata;
} IggyBitmapFontOverride;

inline void RADLINK IggySetInstalledFontMaxCount(S32 num) { (void)num; }
inline void RADLINK IggySetIndirectFontMaxCount(S32 num) { (void)num; }

#define IGGY_FONTFLAG_none    0
#define IGGY_FONTFLAG_bold    1
#define IGGY_FONTFLAG_italic  2
#define IGGY_FONTFLAG_all    (~0U)

#define IGGY_TTC_INDEX_none   0

inline void RADLINK IggyFontInstallTruetypeUTF8(const void *truetype_storage, S32 ttc_index, const char *fontname, S32 namelen_in_bytes, U32 fontflags) { (void)truetype_storage; (void)ttc_index; (void)fontname; (void)namelen_in_bytes; (void)fontflags; }
inline void RADLINK IggyFontInstallTruetypeUTF16(const void *truetype_storage, S32 ttc_index, const U16 *fontname, S32 namelen_in_16bit_quantities, U32 fontflags) { (void)truetype_storage; (void)ttc_index; (void)fontname; (void)namelen_in_16bit_quantities; (void)fontflags; }
inline void RADLINK IggyFontInstallTruetypeFallbackCodepointUTF8(const char *fontname, S32 len, U32 fontflags, S32 fallback_codepoint) { (void)fontname; (void)len; (void)fontflags; (void)fallback_codepoint; }
inline void RADLINK IggyFontInstallTruetypeFallbackCodepointUTF16(const U16 *fontname, S32 len, U32 fontflags, S32 fallback_codepoint) { (void)fontname; (void)len; (void)fontflags; (void)fallback_codepoint; }
inline void RADLINK IggyFontInstallVectorUTF8(const IggyVectorFontProvider *vfp, const char *fontname, S32 namelen_in_bytes, U32 fontflags) { (void)vfp; (void)fontname; (void)namelen_in_bytes; (void)fontflags; }
inline void RADLINK IggyFontInstallVectorUTF16(const IggyVectorFontProvider *vfp, const U16 *fontname, S32 namelen_in_16bit_quantities, U32 fontflags) { (void)vfp; (void)fontname; (void)namelen_in_16bit_quantities; (void)fontflags; }
inline void RADLINK IggyFontInstallBitmapUTF8(const IggyBitmapFontProvider *bmf, const char *fontname, S32 namelen_in_bytes, U32 fontflags) { (void)bmf; (void)fontname; (void)namelen_in_bytes; (void)fontflags; }
inline void RADLINK IggyFontInstallBitmapUTF16(const IggyBitmapFontProvider *bmf, const U16 *fontname, S32 namelen_in_16bit_quantities, U32 fontflags) { (void)bmf; (void)fontname; (void)namelen_in_16bit_quantities; (void)fontflags; }
inline void RADLINK IggyFontInstallBitmapOverrideUTF8(const IggyBitmapFontOverride *bmf, const char *fontname, S32 namelen_in_bytes, U32 fontflags) { (void)bmf; (void)fontname; (void)namelen_in_bytes; (void)fontflags; }
inline void RADLINK IggyFontInstallBitmapOverrideUTF16(const IggyBitmapFontOverride *bmf, const U16 *fontname, S32 namelen_in_16bit_quantities, U32 fontflags) { (void)bmf; (void)fontname; (void)namelen_in_16bit_quantities; (void)fontflags; }

inline void RADLINK IggyFontRemoveUTF8(const char *fontname, S32 namelen_in_bytes, U32 fontflags) { (void)fontname; (void)namelen_in_bytes; (void)fontflags; }
inline void RADLINK IggyFontRemoveUTF16(const U16 *fontname, S32 namelen_in_16bit_quantities, U32 fontflags) { (void)fontname; (void)namelen_in_16bit_quantities; (void)fontflags; }

inline void RADLINK IggyFontSetIndirectUTF8(const char *request_name, S32 request_namelen, U32 request_flags, const char *result_name, S32 result_namelen, U32 result_flags) { (void)request_name; (void)request_namelen; (void)request_flags; (void)result_name; (void)result_namelen; (void)result_flags; }
inline void RADLINK IggyFontSetIndirectUTF16(const U16 *request_name, S32 request_namelen, U32 request_flags, const U16 *result_name, S32 result_namelen, U32 result_flags) { (void)request_name; (void)request_namelen; (void)request_flags; (void)result_name; (void)result_namelen; (void)result_flags; }

inline void RADLINK IggyFontSetFallbackFontUTF8(const char *fontname, S32 fontname_len, U32 fontflags) { (void)fontname; (void)fontname_len; (void)fontflags; }
inline void RADLINK IggyFontSetFallbackFontUTF16(const U16 *fontname, S32 fontname_len, U32 fontflags) { (void)fontname; (void)fontname_len; (void)fontflags; }

// Audio
struct _RadSoundSystem;
IDOCN typedef S32 (*IGGYSND_OPEN_FUNC)(struct _RadSoundSystem* i_SoundSystem, U32 i_MinBufferSizeInMs, U32 i_Frequency, U32 i_ChannelCount, U32 i_MaxLockSize, U32 i_Flags);

IDOCN inline void RADLINK IggyAudioSetDriver(IGGYSND_OPEN_FUNC driver_open, U32 flags) { (void)driver_open; (void)flags; }

IDOCN inline void RADLINK IggyAudioUseDirectSound(void) {}
IDOCN inline void RADLINK IggyAudioUseWaveOut(void) {}
IDOCN inline void RADLINK IggyAudioUseXAudio2(void) {}
IDOCN inline void RADLINK IggyAudioUseLibAudio(void) {}
IDOCN inline void RADLINK IggyAudioUseAX(void) {}
IDOCN inline void RADLINK IggyAudioUseCoreAudio(void) {}

inline void RADLINK IggyAudioUseDefault(void) {}

#ifndef __RAD_DEFINE_IGGYMP3__
#define __RAD_DEFINE_IGGYMP3__
IDOCN typedef struct IggyMP3Interface IggyMP3Interface;
IDOCN typedef rrbool IggyGetMP3Decoder(IggyMP3Interface *decoder);
#endif

#ifdef __RADNT__
   inline void RADLINK IggyAudioInstallMP3Decoder(void) {}
   inline void RADLINK IggySetDLLDirectory(char *path) { (void)path; }
   inline void RADLINK IggySetDLLDirectoryW(wchar_t *path) { (void)path; }
#else
   IDOCN inline IggyGetMP3Decoder* RADLINK IggyAudioGetMP3Decoder(void) { return 0; }
   IDOCN inline void RADLINK IggyAudioInstallMP3DecoderExplicit(IggyGetMP3Decoder *init) { (void)init; }

   #define IggyAudioInstallMP3Decoder() \
      IggyAudioInstallMP3DecoderExplicit(IggyAudioGetMP3Decoder()) IDOCN
#endif

inline rrbool RADLINK IggyAudioSetMaxBufferTime(S32 ms) { (void)ms; return 0; }
inline void   RADLINK IggyAudioSetLatency(S32 ms) { (void)ms; }
inline void   RADLINK IggyPlayerSetAudioVolume(Iggy *iggy, F32 attenuation) { (void)iggy; (void)attenuation; }

#define   IGGY_AUDIODEVICE_default    0
#define   IGGY_AUDIODEVICE_primary    1
#define   IGGY_AUDIODEVICE_secondary  2

IDOCN inline void RADLINK IggyPlayerSetAudioDevice(Iggy *iggy, S32 device) { (void)iggy; (void)device; }

// Rendering types
typedef struct IggyCustomDrawCallbackRegion
{
    IggyUTF16 *name;
    F32 x0, y0, x1, y1;
    F32 rgba_mul[4];
    F32 rgba_add[4];
    S32 scissor_x0, scissor_y0, scissor_x1, scissor_y1;
    U8 scissor_enable;
    U8 stencil_func_mask;
    U8 stencil_func_ref;
    U8 stencil_write_mask;
    struct gswf_matrix *o2w;
} IggyCustomDrawCallbackRegion;

typedef void RADLINK Iggy_CustomDrawCallback(void *user_callback_data, Iggy *player, IggyCustomDrawCallbackRegion *Region);
typedef GDrawTexture* RADLINK Iggy_TextureSubstitutionCreateCallback(void *user_callback_data, IggyUTF16 *texture_name, S32 *width, S32 *height, void **destroy_callback_data);
typedef void RADLINK Iggy_TextureSubstitutionDestroyCallback(void *user_callback_data, void *destroy_callback_data, GDrawTexture *handle);
typedef GDrawTexture* RADLINK Iggy_TextureSubstitutionCreateCallbackUTF8(void *user_callback_data, char *texture_name, S32 *width, S32 *height, void **destroy_callback_data);

inline void RADLINK IggySetCustomDrawCallback(Iggy_CustomDrawCallback *custom_draw, void *user_callback_data) { (void)custom_draw; (void)user_callback_data; }
inline void RADLINK IggySetTextureSubstitutionCallbacks(Iggy_TextureSubstitutionCreateCallback *texture_create, Iggy_TextureSubstitutionDestroyCallback *texture_destroy, void *user_callback_data) { (void)texture_create; (void)texture_destroy; (void)user_callback_data; }
inline void RADLINK IggySetTextureSubstitutionCallbacksUTF8(Iggy_TextureSubstitutionCreateCallbackUTF8 *texture_create, Iggy_TextureSubstitutionDestroyCallback *texture_destroy, void *user_callback_data) { (void)texture_create; (void)texture_destroy; (void)user_callback_data; }

typedef enum {
   IGGY_FLUSH_no_callback,
   IGGY_FLUSH_destroy_callback,
} IggyTextureSubstitutionFlushMode;

inline void RADLINK IggyTextureSubstitutionFlush(GDrawTexture *handle, IggyTextureSubstitutionFlushMode do_destroy_callback) { (void)handle; (void)do_destroy_callback; }
inline void RADLINK IggyTextureSubstitutionFlushAll(IggyTextureSubstitutionFlushMode do_destroy_callback) { (void)do_destroy_callback; }

inline void RADLINK IggySetGDraw(GDrawFunctions *gdraw) { (void)gdraw; }
inline void RADLINK IggyPlayerGetBackgroundColor(Iggy *player, F32 output_color[3]) { (void)player; if(output_color) { output_color[0]=0; output_color[1]=0; output_color[2]=0; } }

typedef enum
{
   IGGY_ROTATION_0_degrees = 0,
   IGGY_ROTATION_90_degrees_counterclockwise = 1,
   IGGY_ROTATION_180_degrees = 2,
   IGGY_ROTATION_90_degrees_clockwise = 3,
} Iggy90DegreeRotation;

inline void RADLINK IggyPlayerSetDisplaySize(Iggy *f, S32 w, S32 h) { (void)f; (void)w; (void)h; }
inline void RADLINK IggyPlayerSetPixelShape(Iggy *swf, F32 pixel_x, F32 pixel_y) { (void)swf; (void)pixel_x; (void)pixel_y; }
inline void RADLINK IggyPlayerSetStageRotation(Iggy *f, Iggy90DegreeRotation rot) { (void)f; (void)rot; }
inline void RADLINK IggyPlayerDraw(Iggy *f) { (void)f; }
inline void RADLINK IggyPlayerSetStageSize(Iggy *f, S32 w, S32 h) { (void)f; (void)w; (void)h; }
inline void RADLINK IggyPlayerSetFaux3DStage(Iggy *f, F32 *top_left, F32 *top_right, F32 *bottom_left, F32 *bottom_right, F32 depth_scale) { (void)f; (void)top_left; (void)top_right; (void)bottom_left; (void)bottom_right; (void)depth_scale; }
inline void RADLINK IggyPlayerForceMipmaps(Iggy *f, rrbool force_mipmaps) { (void)f; (void)force_mipmaps; }

inline void RADLINK IggyPlayerDrawTile(Iggy *f, S32 x0, S32 y0, S32 x1, S32 y1, S32 padding) { (void)f; (void)x0; (void)y0; (void)x1; (void)y1; (void)padding; }
inline void RADLINK IggyPlayerDrawTilesStart(Iggy *f) { (void)f; }
inline void RADLINK IggyPlayerDrawTilesEnd(Iggy *f) { (void)f; }
inline void RADLINK IggyPlayerSetRootTransform(Iggy *f, F32 mat[4], F32 tx, F32 ty) { (void)f; (void)mat; (void)tx; (void)ty; }
inline void RADLINK IggyPlayerFlushAll(Iggy *player) { (void)player; }
inline void RADLINK IggyLibraryFlushAll(IggyLibrary h) { (void)h; }
inline void RADLINK IggySetTextCursorPixelWidth(S32 width) { (void)width; }
inline void RADLINK IggyForceBitmapSmoothing(rrbool force_on) { (void)force_on; }
inline void RADLINK IggyFlushInstalledFonts(void) {}
inline void RADLINK IggyFastTextFilterEffects(rrbool enable) { (void)enable; }

typedef enum IggyAntialiasing
{
   IGGY_ANTIALIASING_FontsOnly = 2,
   IGGY_ANTIALIASING_FontsAndLinesOnly = 4,
   IGGY_ANTIALIASING_PrettyGood = 8,
   IGGY_ANTIALIASING_Good = 10,
} IggyAntialiasing;

inline void RADLINK IggyPlayerSetAntialiasing(Iggy *f, IggyAntialiasing antialias_mode) { (void)f; (void)antialias_mode; }

inline void RADLINK IggyPlayerSetBitmapFontCaching(Iggy *f, S32 tex_w, S32 tex_h, S32 max_char_pix_width, S32 max_char_pix_height) { (void)f; (void)tex_w; (void)tex_h; (void)max_char_pix_width; (void)max_char_pix_height; }

inline void RADLINK IggySetFontCachingCalculationBuffer(S32 max_chars, void *optional_temp_buffer, S32 optional_temp_buffer_size_in_bytes) { (void)max_chars; (void)optional_temp_buffer; (void)optional_temp_buffer_size_in_bytes; }

typedef struct IggyGeneric IggyGeneric;

inline IggyGeneric * RADLINK IggyPlayerGetGeneric(Iggy *player) { (void)player; return 0; }
inline IggyGeneric * RADLINK IggyLibraryGetGeneric(IggyLibrary lib) { (void)lib; return 0; }

IDOCN typedef struct
{
   U16 num_textures;
   U16 load_alignment_log2;
   U32 texture_file_size;
   void *texture_info;
} IggyTextureResourceMetadata;

inline void RADLINK IggyGenericInstallResourceFile(IggyGeneric *g, void *data, S32 data_length, rrbool *can_free_now) { (void)g; (void)data; (void)data_length; if(can_free_now) *can_free_now = 1; }
inline IggyTextureResourceMetadata *RADLINK IggyGenericGetTextureResourceMetadata(IggyGeneric *f) { (void)f; return 0; }
inline void RADLINK IggyGenericSetTextureFromResource(IggyGeneric *f, U16 id, GDrawTexture *handle) { (void)f; (void)id; (void)handle; }

typedef enum
{
   IFT_FORMAT_rgba_8888,
   IFT_FORMAT_rgba_4444_LE,
   IFT_FORMAT_rgba_5551_LE,
   IFT_FORMAT_la_88,
   IFT_FORMAT_la_44,
   IFT_FORMAT_i_8,
   IFT_FORMAT_i_4,
   IFT_FORMAT_l_8,
   IFT_FORMAT_l_4,
   IFT_FORMAT_DXT1,
   IFT_FORMAT_DXT3,
   IFT_FORMAT_DXT5,
} IggyFileTexture_Format;

typedef struct
{
   U32 file_offset;
   U8  format;
   U8  mipmaps;
   U16 w,h;
   U16 swf_id;
} IggyFileTextureRaw;

IDOCN typedef struct
{
   U32 file_offset;
   U16 swf_id;
   U16 padding;
   struct {
      U32 data[13];
   } texture;
} IggyFileTexture360;

IDOCN typedef struct
{
   U32 file_offset;
   U16 swf_id;
   U8  format;
   U8  padding;
   struct {
      U32 data[6];
   } texture;
} IggyFileTexturePS3;

IDOCN typedef struct
{
   U32 file_offset1;
   U32 file_offset2;
   U16 swf_id;
   U8  format;
   U8  padding;
   struct {
      U32 data1[39];
   } texture;
} IggyFileTextureWiiu;

IDOCN typedef struct
{
   U32 file_offset;
   U16 swf_id;
   U8  format;
   U8  padding;
   struct {
      U32 data[8];
   } texture;
} IggyFileTexturePS4;

IDOCN typedef struct
{
   U32 file_offset;
   U16 swf_id;
   U8  format;
   U8  padding;
   struct {
      U32 format;
      U32 type;
      U16 width;
      U16 height;
      U8 mip_count;
      U8 pad[3];
   } texture;
} IggyFileTexturePSP2;

// AS3
typedef rrbool RADLINK Iggy_AS3ExternalFunctionUTF8(void *user_callback_data, Iggy *player, IggyExternalFunctionCallUTF8 *call);
typedef rrbool RADLINK Iggy_AS3ExternalFunctionUTF16(void *user_callback_data, Iggy *player, IggyExternalFunctionCallUTF16 *call);

inline void RADLINK IggySetAS3ExternalFunctionCallbackUTF8(Iggy_AS3ExternalFunctionUTF8 *as3_external_function_utf8, void *user_callback_data) { (void)as3_external_function_utf8; (void)user_callback_data; }
inline void RADLINK IggySetAS3ExternalFunctionCallbackUTF16(Iggy_AS3ExternalFunctionUTF16 *as3_external_function_utf16, void *user_callback_data) { (void)as3_external_function_utf16; (void)user_callback_data; }
inline IggyName RADLINK IggyPlayerCreateFastName(Iggy *f, IggyUTF16 const *name, S32 len) { (void)f; (void)name; (void)len; return 0; }
inline IggyName RADLINK IggyPlayerCreateFastNameUTF8(Iggy *f, char const *name, S32 len) { (void)f; (void)name; (void)len; return 0; }
inline IggyResult RADLINK IggyPlayerCallFunctionRS(Iggy *player, IggyDataValue *result, IggyName function, S32 numargs, IggyDataValue *args) { (void)player; (void)result; (void)function; (void)numargs; (void)args; return IGGY_RESULT_SUCCESS; }
inline IggyResult RADLINK IggyPlayerCallMethodRS(Iggy *f, IggyDataValue *result, IggyValuePath *target, IggyName methodname, S32 numargs, IggyDataValue *args) { (void)f; (void)result; (void)target; (void)methodname; (void)numargs; (void)args; return IGGY_RESULT_SUCCESS; }
inline void RADLINK IggyPlayerGarbageCollect(Iggy *player, S32 strength) { (void)player; (void)strength; }

#define IGGY_GC_MINIMAL  0
#define IGGY_GC_NORMAL   30
#define IGGY_GC_MAXIMAL  100

typedef struct
{
   U32 young_heap_size;
   U32 base_old_amount;
   F32 old_heap_fraction;
   F32 new_allocation_multiplier;
   F32 sweep_multiplier;
} IggyGarbageCollectorControl;

typedef enum
{
   IGGY_GC_EVENT_tenure,
   IGGY_GC_EVENT_mark_increment,
   IGGY_GC_EVENT_mark_roots,
   IGGY_GC_EVENT_sweep_finalize,
   IGGY_GC_EVENT_sweep_increment,
   IGGY_GC_WARNING_greylist_overflow,
   IGGY_GC_WARNING_remembered_overflow,
} IggyGarbageCollectionEvent;

typedef struct
{
   U64 event_time_in_microseconds;
   U64 total_marked_bytes;
   U64 total_swept_bytes;
   U64 total_allocated_bytes;
   U64 total_gc_time_in_microseconds;
   char *name;
   IggyGarbageCollectionEvent event;
   U32 increment_processing_bytes;
   U32 last_slice_tenured_bytes;
   U32 last_slice_old_allocation_bytes;
   U32 heap_used_bytes;
   U32 heap_size_bytes;
   U32 onstage_display_objects;
   U32 offstage_display_objects;
} IggyGarbageCollectionInfo;

typedef void RADLINK Iggy_GarbageCollectionCallback(Iggy *player, IggyGarbageCollectionInfo *info);
inline void RADLINK IggyPlayerConfigureGCBehavior(Iggy *player, Iggy_GarbageCollectionCallback *notify_callack, IggyGarbageCollectorControl *control) { (void)player; (void)notify_callack; (void)control; }
inline void RADLINK IggyPlayerQueryGCSizes(Iggy *player, IggyPlayerGCSizes *sizes) { (void)player; (void)sizes; }

inline rrbool RADLINK IggyPlayerGetValid(Iggy *f) { (void)f; return 0; }

IDOCN struct IggyValuePath
{
    Iggy *f;
    IggyValuePath *parent;
    IggyName name;
    IggyValueRef ref;
    S32 index;
    S32 type;
};

typedef enum
{
   IGGY_ValueRef,
   IGGY_ValueRef_Weak,
} IggyValueRefType;

inline rrbool       RADLINK IggyValueRefCheck(IggyValueRef ref) { (void)ref; return 0; }
inline void         RADLINK IggyValueRefFree(Iggy *p, IggyValueRef ref) { (void)p; (void)ref; }
inline IggyValueRef RADLINK IggyValueRefFromPath(IggyValuePath *var, IggyValueRefType reftype) { (void)var; (void)reftype; return 0; }
inline rrbool       RADLINK IggyIsValueRefSameObjectAsTempRef(IggyValueRef value_ref, IggyTempRef temp_ref) { (void)value_ref; (void)temp_ref; return 0; }
inline rrbool       RADLINK IggyIsValueRefSameObjectAsValuePath(IggyValueRef value_ref, IggyValuePath *path, IggyName sub_name, char const *sub_name_utf8) { (void)value_ref; (void)path; (void)sub_name; (void)sub_name_utf8; return 0; }
inline void         RADLINK IggySetValueRefLimit(Iggy *f, S32 max_value_refs) { (void)f; (void)max_value_refs; }
inline S32          RADLINK IggyDebugGetNumValueRef(Iggy *f) { (void)f; return 0; }
inline IggyValueRef RADLINK IggyValueRefCreateArray(Iggy *f, S32 num_slots) { (void)f; (void)num_slots; return 0; }
inline IggyValueRef RADLINK IggyValueRefCreateEmptyObject(Iggy *f) { (void)f; return 0; }
inline IggyValueRef RADLINK IggyValueRefFromTempRef(Iggy *f, IggyTempRef temp_ref, IggyValueRefType reftype) { (void)f; (void)temp_ref; (void)reftype; return 0; }

static IggyValuePath _iggy_stub_root_path;
static IggyValuePath _iggy_stub_callback_result_path;
inline IggyValuePath * RADLINK IggyPlayerRootPath(Iggy *f) { (void)f; memset(&_iggy_stub_root_path, 0, sizeof(_iggy_stub_root_path)); return &_iggy_stub_root_path; }
inline IggyValuePath * RADLINK IggyPlayerCallbackResultPath(Iggy *f) { (void)f; memset(&_iggy_stub_callback_result_path, 0, sizeof(_iggy_stub_callback_result_path)); return &_iggy_stub_callback_result_path; }
inline rrbool RADLINK IggyValuePathMakeNameRef(IggyValuePath *result, IggyValuePath *parent, char const *text_utf8) { (void)result; (void)parent; (void)text_utf8; if(result) memset(result, 0, sizeof(*result)); return 0; }
inline void RADLINK IggyValuePathFromRef(IggyValuePath *result, Iggy *iggy, IggyValueRef ref) { (void)result; (void)iggy; (void)ref; if(result) memset(result, 0, sizeof(*result)); }

inline void RADLINK IggyValuePathMakeNameRefFast(IggyValuePath *result, IggyValuePath *parent, IggyName name) { (void)result; (void)parent; (void)name; if(result) memset(result, 0, sizeof(*result)); }
inline void RADLINK IggyValuePathMakeArrayRef(IggyValuePath *result, IggyValuePath *array_path, int array_index) { (void)result; (void)array_path; (void)array_index; if(result) memset(result, 0, sizeof(*result)); }

inline void RADLINK IggyValuePathSetParent(IggyValuePath *result, IggyValuePath *new_parent) { (void)result; (void)new_parent; }
inline void RADLINK IggyValuePathSetArrayIndex(IggyValuePath *result, int new_index) { (void)result; (void)new_index; }

inline void RADLINK IggyValuePathSetName(IggyValuePath *result, IggyName name) { (void)result; (void)name; }
inline IggyResult RADLINK IggyValueGetTypeRS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, IggyDatatype *result) { (void)var; (void)sub_name; (void)sub_name_utf8; if(result) *result = IGGY_DATATYPE_undefined; return IGGY_RESULT_SUCCESS; }

inline IggyResult RADLINK IggyValueGetF64RS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, F64 *result) { (void)var; (void)sub_name; (void)sub_name_utf8; if(result) *result = 0; return IGGY_RESULT_SUCCESS; }
inline IggyResult RADLINK IggyValueGetF32RS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, F32 *result) { (void)var; (void)sub_name; (void)sub_name_utf8; if(result) *result = 0; return IGGY_RESULT_SUCCESS; }
inline IggyResult RADLINK IggyValueGetS32RS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, S32 *result) { (void)var; (void)sub_name; (void)sub_name_utf8; if(result) *result = 0; return IGGY_RESULT_SUCCESS; }
inline IggyResult RADLINK IggyValueGetU32RS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, U32 *result) { (void)var; (void)sub_name; (void)sub_name_utf8; if(result) *result = 0; return IGGY_RESULT_SUCCESS; }
inline IggyResult RADLINK IggyValueGetStringUTF8RS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, S32 max_result_len, char *utf8_result, S32 *result_len) { (void)var; (void)sub_name; (void)sub_name_utf8; if(utf8_result && max_result_len > 0) utf8_result[0] = 0; if(result_len) *result_len = 0; return IGGY_RESULT_SUCCESS; }
inline IggyResult RADLINK IggyValueGetStringUTF16RS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, S32 max_result_len, IggyUTF16 *utf16_result, S32 *result_len) { (void)var; (void)sub_name; (void)sub_name_utf8; if(utf16_result && max_result_len > 0) utf16_result[0] = 0; if(result_len) *result_len = 0; return IGGY_RESULT_SUCCESS; }
inline IggyResult RADLINK IggyValueGetBooleanRS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, rrbool *result) { (void)var; (void)sub_name; (void)sub_name_utf8; if(result) *result = 0; return IGGY_RESULT_SUCCESS; }
inline IggyResult RADLINK IggyValueGetArrayLengthRS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, S32 *result) { (void)var; (void)sub_name; (void)sub_name_utf8; if(result) *result = 0; return IGGY_RESULT_SUCCESS; }

inline rrbool RADLINK IggyValueSetF64RS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, F64 value) { (void)var; (void)sub_name; (void)sub_name_utf8; (void)value; return 0; }
inline rrbool RADLINK IggyValueSetF32RS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, F32 value) { (void)var; (void)sub_name; (void)sub_name_utf8; (void)value; return 0; }
inline rrbool RADLINK IggyValueSetS32RS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, S32 value) { (void)var; (void)sub_name; (void)sub_name_utf8; (void)value; return 0; }
inline rrbool RADLINK IggyValueSetU32RS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, U32 value) { (void)var; (void)sub_name; (void)sub_name_utf8; (void)value; return 0; }
inline rrbool RADLINK IggyValueSetStringUTF8RS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, char const *utf8_string, S32 stringlen) { (void)var; (void)sub_name; (void)sub_name_utf8; (void)utf8_string; (void)stringlen; return 0; }
inline rrbool RADLINK IggyValueSetStringUTF16RS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, IggyUTF16 const *utf16_string, S32 stringlen) { (void)var; (void)sub_name; (void)sub_name_utf8; (void)utf16_string; (void)stringlen; return 0; }
inline rrbool RADLINK IggyValueSetBooleanRS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, rrbool value) { (void)var; (void)sub_name; (void)sub_name_utf8; (void)value; return 0; }
inline rrbool RADLINK IggyValueSetValueRefRS(IggyValuePath *var, IggyName sub_name, char const *sub_name_utf8, IggyValueRef value_ref) { (void)var; (void)sub_name; (void)sub_name_utf8; (void)value_ref; return 0; }

inline rrbool RADLINK IggyValueSetUserDataRS(IggyValuePath *result, void const *userdata) { (void)result; (void)userdata; return 0; }
inline IggyResult RADLINK IggyValueGetUserDataRS(IggyValuePath *result, void **userdata) { (void)result; if(userdata) *userdata = 0; return IGGY_RESULT_SUCCESS; }

// Input Events
typedef enum IggyEventType
{
   IGGY_EVENTTYPE_None,
   IGGY_EVENTTYPE_MouseLeftDown,
   IGGY_EVENTTYPE_MouseLeftUp,
   IGGY_EVENTTYPE_MouseRightDown,
   IGGY_EVENTTYPE_MouseRightUp,
   IGGY_EVENTTYPE_MouseMiddleDown,
   IGGY_EVENTTYPE_MouseMiddleUp,
   IGGY_EVENTTYPE_MouseMove,
   IGGY_EVENTTYPE_MouseWheel,
   IGGY_EVENTTYPE_KeyUp,
   IGGY_EVENTTYPE_KeyDown,
   IGGY_EVENTTYPE_Char,
   IGGY_EVENTTYPE_Activate,
   IGGY_EVENTTYPE_Deactivate,
   IGGY_EVENTTYPE_Resize,
   IGGY_EVENTTYPE_MouseLeave,
   IGGY_EVENTTYPE_FocusLost,
} IggyEventType;

typedef enum IggyKeyloc
{
   IGGY_KEYLOC_Standard = 0,
   IGGY_KEYLOC_Left = 1,
   IGGY_KEYLOC_Right = 2,
   IGGY_KEYLOC_Numpad = 3,
} IggyKeyloc;

typedef enum IggyKeyevent
{
   IGGY_KEYEVENT_Up = IGGY_EVENTTYPE_KeyUp,
   IGGY_KEYEVENT_Down = IGGY_EVENTTYPE_KeyDown,
} IggyKeyevent;

typedef enum IggyMousebutton
{
   IGGY_MOUSEBUTTON_LeftDown = IGGY_EVENTTYPE_MouseLeftDown,
   IGGY_MOUSEBUTTON_LeftUp = IGGY_EVENTTYPE_MouseLeftUp,
   IGGY_MOUSEBUTTON_RightDown = IGGY_EVENTTYPE_MouseRightDown,
   IGGY_MOUSEBUTTON_RightUp = IGGY_EVENTTYPE_MouseRightUp,
   IGGY_MOUSEBUTTON_MiddleDown = IGGY_EVENTTYPE_MouseMiddleDown,
   IGGY_MOUSEBUTTON_MiddleUp = IGGY_EVENTTYPE_MouseMiddleUp,
} IggyMousebutton;

typedef enum IggyActivestate
{
   IGGY_ACTIVESTATE_Activated = IGGY_EVENTTYPE_Activate,
   IGGY_ACTIVESTATE_Deactivated = IGGY_EVENTTYPE_Deactivate,
} IggyActivestate;

typedef enum IggyKeycode
{
   IGGY_KEYCODE_A = 65, IGGY_KEYCODE_B = 66, IGGY_KEYCODE_C = 67, IGGY_KEYCODE_D = 68,
   IGGY_KEYCODE_E = 69, IGGY_KEYCODE_F = 70, IGGY_KEYCODE_G = 71, IGGY_KEYCODE_H = 72,
   IGGY_KEYCODE_I = 73, IGGY_KEYCODE_J = 74, IGGY_KEYCODE_K = 75, IGGY_KEYCODE_L = 76,
   IGGY_KEYCODE_M = 77, IGGY_KEYCODE_N = 78, IGGY_KEYCODE_O = 79, IGGY_KEYCODE_P = 80,
   IGGY_KEYCODE_Q = 81, IGGY_KEYCODE_R = 82, IGGY_KEYCODE_S = 83, IGGY_KEYCODE_T = 84,
   IGGY_KEYCODE_U = 85, IGGY_KEYCODE_V = 86, IGGY_KEYCODE_W = 87, IGGY_KEYCODE_X = 88,
   IGGY_KEYCODE_Y = 89, IGGY_KEYCODE_Z = 90,

   IGGY_KEYCODE_0 = 48, IGGY_KEYCODE_1 = 49, IGGY_KEYCODE_2 = 50, IGGY_KEYCODE_3 = 51,
   IGGY_KEYCODE_4 = 52, IGGY_KEYCODE_5 = 53, IGGY_KEYCODE_6 = 54, IGGY_KEYCODE_7 = 55,
   IGGY_KEYCODE_8 = 56, IGGY_KEYCODE_9 = 57,

   IGGY_KEYCODE_F1 = 112, IGGY_KEYCODE_F2 = 113, IGGY_KEYCODE_F3 = 114, IGGY_KEYCODE_F4 = 115,
   IGGY_KEYCODE_F5 = 116, IGGY_KEYCODE_F6 = 117, IGGY_KEYCODE_F7 = 118, IGGY_KEYCODE_F8 = 119,
   IGGY_KEYCODE_F9 = 120, IGGY_KEYCODE_F10 = 121, IGGY_KEYCODE_F11 = 122, IGGY_KEYCODE_F12 = 123,
   IGGY_KEYCODE_F13 = 124, IGGY_KEYCODE_F14 = 125, IGGY_KEYCODE_F15 = 126,

   IGGY_KEYCODE_COMMAND = 15, IGGY_KEYCODE_SHIFT = 16, IGGY_KEYCODE_CONTROL = 17, IGGY_KEYCODE_ALTERNATE = 18,

   IGGY_KEYCODE_BACKQUOTE = 192, IGGY_KEYCODE_BACKSLASH = 220, IGGY_KEYCODE_BACKSPACE = 8,
   IGGY_KEYCODE_CAPS_LOCK = 20, IGGY_KEYCODE_COMMA = 188, IGGY_KEYCODE_DELETE = 46,
   IGGY_KEYCODE_DOWN = 40, IGGY_KEYCODE_END = 35, IGGY_KEYCODE_ENTER = 13,
   IGGY_KEYCODE_EQUAL = 187, IGGY_KEYCODE_ESCAPE = 27, IGGY_KEYCODE_HOME = 36,
   IGGY_KEYCODE_INSERT = 45, IGGY_KEYCODE_LEFT = 37, IGGY_KEYCODE_LEFTBRACKET = 219,
   IGGY_KEYCODE_MINUS = 189, IGGY_KEYCODE_NUMPAD = 21,
   IGGY_KEYCODE_NUMPAD_0 = 96, IGGY_KEYCODE_NUMPAD_1 = 97, IGGY_KEYCODE_NUMPAD_2 = 98,
   IGGY_KEYCODE_NUMPAD_3 = 99, IGGY_KEYCODE_NUMPAD_4 = 100, IGGY_KEYCODE_NUMPAD_5 = 101,
   IGGY_KEYCODE_NUMPAD_6 = 102, IGGY_KEYCODE_NUMPAD_7 = 103, IGGY_KEYCODE_NUMPAD_8 = 104,
   IGGY_KEYCODE_NUMPAD_9 = 105, IGGY_KEYCODE_NUMPAD_ADD = 107, IGGY_KEYCODE_NUMPAD_DECIMAL = 110,
   IGGY_KEYCODE_NUMPAD_DIVIDE = 111, IGGY_KEYCODE_NUMPAD_ENTER = 108,
   IGGY_KEYCODE_NUMPAD_MULTIPLY = 106, IGGY_KEYCODE_NUMPAD_SUBTRACT = 109,
   IGGY_KEYCODE_PAGE_DOWN = 34, IGGY_KEYCODE_PAGE_UP = 33, IGGY_KEYCODE_PERIOD = 190,
   IGGY_KEYCODE_QUOTE = 222, IGGY_KEYCODE_RIGHT = 39, IGGY_KEYCODE_RIGHTBRACKET = 221,
   IGGY_KEYCODE_SEMICOLON = 186, IGGY_KEYCODE_SLASH = 191, IGGY_KEYCODE_SPACE = 32,
   IGGY_KEYCODE_TAB = 9, IGGY_KEYCODE_UP = 38,
} IggyKeycode;

typedef enum IggyEventFlag
{
    IGGY_EVENTFLAG_PreventDispatchToObject = 0x1,
    IGGY_EVENTFLAG_PreventFocusTabbing = 0x2,
    IGGY_EVENTFLAG_PreventDefault = 0x4,
    IGGY_EVENTFLAG_RanAtLeastOneHandler = 0x8,
} IggyEventFlag;

typedef struct IggyEvent
{
   S32 type;
   U32 flags;
   S32 x,y;
   S32 keycode,keyloc;
} IggyEvent;

typedef enum IggyFocusChange
{
    IGGY_FOCUS_CHANGE_None,
    IGGY_FOCUS_CHANGE_TookFocus,
    IGGY_FOCUS_CHANGE_LostFocus,
} IggyFocusChange;

typedef struct IggyEventResult
{
    U32 new_flags;
    S32 focus_change;
    S32 focus_direction;
} IggyEventResult;

inline void RADLINK IggyMakeEventNone(IggyEvent *event) { if(event) memset(event, 0, sizeof(*event)); }
inline void RADLINK IggyMakeEventResize(IggyEvent *event) { if(event) { memset(event, 0, sizeof(*event)); event->type = IGGY_EVENTTYPE_Resize; } }
inline void RADLINK IggyMakeEventActivate(IggyEvent *event, IggyActivestate event_type) { if(event) { memset(event, 0, sizeof(*event)); event->type = event_type; } }
inline void RADLINK IggyMakeEventMouseLeave(IggyEvent *event) { if(event) { memset(event, 0, sizeof(*event)); event->type = IGGY_EVENTTYPE_MouseLeave; } }
inline void RADLINK IggyMakeEventMouseMove(IggyEvent *event, S32 x, S32 y) { if(event) { memset(event, 0, sizeof(*event)); event->type = IGGY_EVENTTYPE_MouseMove; event->x = x; event->y = y; } }
inline void RADLINK IggyMakeEventMouseButton(IggyEvent *event, IggyMousebutton event_type) { if(event) { memset(event, 0, sizeof(*event)); event->type = event_type; } }
inline void RADLINK IggyMakeEventMouseWheel(IggyEvent *event, S16 mousewheel_delta) { if(event) { memset(event, 0, sizeof(*event)); event->type = IGGY_EVENTTYPE_MouseWheel; (void)mousewheel_delta; } }
inline void RADLINK IggyMakeEventKey(IggyEvent *event, IggyKeyevent event_type, IggyKeycode keycode, IggyKeyloc keyloc) { if(event) { memset(event, 0, sizeof(*event)); event->type = event_type; event->keycode = keycode; event->keyloc = keyloc; } }
inline void RADLINK IggyMakeEventChar(IggyEvent *event, S32 charcode) { if(event) { memset(event, 0, sizeof(*event)); event->type = IGGY_EVENTTYPE_Char; event->keycode = charcode; } }
inline void RADLINK IggyMakeEventFocusLost(IggyEvent *event) { if(event) { memset(event, 0, sizeof(*event)); event->type = IGGY_EVENTTYPE_FocusLost; } }
inline void RADLINK IggyMakeEventFocusGained(IggyEvent *event, S32 focus_direction) { (void)event; (void)focus_direction; }
inline rrbool RADLINK IggyPlayerDispatchEventRS(Iggy *player, IggyEvent *event, IggyEventResult *result) { (void)player; (void)event; if(result) memset(result, 0, sizeof(*result)); return 0; }
inline void RADLINK IggyPlayerSetShiftState(Iggy *f, rrbool shift, rrbool control, rrbool alt, rrbool command) { (void)f; (void)shift; (void)control; (void)alt; (void)command; }
inline void RADLINK IggySetDoubleClickTime(S32 time_in_ms_from_first_down_to_second_up) { (void)time_in_ms_from_first_down_to_second_up; }
inline void RADLINK IggySetTextCursorFlash(U32 cycle_time_in_ms, U32 visible_time_in_ms) { (void)cycle_time_in_ms; (void)visible_time_in_ms; }

inline rrbool RADLINK IggyPlayerHasFocusedEditableTextfield(Iggy *f) { (void)f; return 0; }
inline rrbool RADLINK IggyPlayerPasteUTF16(Iggy *f, U16 *string, S32 stringlen) { (void)f; (void)string; (void)stringlen; return 0; }
inline rrbool RADLINK IggyPlayerPasteUTF8(Iggy *f, char *string, S32 stringlen) { (void)f; (void)string; (void)stringlen; return 0; }
inline rrbool RADLINK IggyPlayerCut(Iggy *f) { (void)f; return 0; }

#define IGGY_PLAYER_COPY_no_focused_textfield       -1
#define IGGY_PLAYER_COPY_textfield_has_no_selection  0
inline S32 RADLINK IggyPlayerCopyUTF16(Iggy *f, U16 *buffer, S32 bufferlen) { (void)f; (void)buffer; (void)bufferlen; return IGGY_PLAYER_COPY_no_focused_textfield; }
inline S32 RADLINK IggyPlayerCopyUTF8(Iggy *f, char *buffer, S32 bufferlen) { (void)f; (void)buffer; (void)bufferlen; return IGGY_PLAYER_COPY_no_focused_textfield; }

// IME
#ifdef __RADNT__
#define IGGY_IME_SUPPORT
#endif

inline void RADLINK IggyPlayerSetIMEFontUTF8(Iggy *f, const char *font_name_utf8, S32 namelen_in_bytes) { (void)f; (void)font_name_utf8; (void)namelen_in_bytes; }
inline void RADLINK IggyPlayerSetIMEFontUTF16(Iggy *f, const IggyUTF16 *font_name_utf16, S32 namelen_in_2byte_words) { (void)f; (void)font_name_utf16; (void)namelen_in_2byte_words; }

#ifdef IGGY_IME_SUPPORT

#define IGGY_IME_MAX_CANDIDATE_LENGTH 256

IDOCN typedef enum {
    IGGY_IME_COMPOSITION_STYLE_NONE,
    IGGY_IME_COMPOSITION_STYLE_UNDERLINE_DOTTED,
    IGGY_IME_COMPOSITION_STYLE_UNDERLINE_DOTTED_THICK,
    IGGY_IME_COMPOSITION_STYLE_UNDERLINE_SOLID,
    IGGY_IME_COMPOSITION_STYLE_UNDERLINE_SOLID_THICK,
} IggyIMECompositionDrawStyle;

IDOCN typedef enum {
    IGGY_IME_COMPOSITION_CLAUSE_NORMAL,
    IGGY_IME_COMPOSITION_CLAUSE_START,
} IggyIMECompositionClauseState;

IDOCN typedef struct
{
    IggyUTF16 str[IGGY_IME_MAX_CANDIDATE_LENGTH];
    IggyIMECompositionDrawStyle   char_style[IGGY_IME_MAX_CANDIDATE_LENGTH];
    IggyIMECompositionClauseState clause_state[IGGY_IME_MAX_CANDIDATE_LENGTH];
    S32 cursor_pos;
    rrbool display_block_cursor;
    int candicate_clause_start_pos;
    int candicate_clause_end_pos;
} IggyIMECompostitionStringState;

IDOCN inline void RADLINK IggyIMEWin32SetCompositionState(Iggy* f, IggyIMECompostitionStringState* s) { (void)f; (void)s; }
IDOCN inline void RADLINK IggyIMEGetTextExtents(Iggy* f, U32* pdw, U32* pdh, const IggyUTF16* str, U32 text_height) { (void)f; if(pdw) *pdw = 0; if(pdh) *pdh = 0; (void)str; (void)text_height; }
IDOCN inline void RADLINK IggyIMEDrawString(Iggy* f, S32 px, S32 py, const IggyUTF16* str, U32 text_height, const U8 rgba[4]) { (void)f; (void)px; (void)py; (void)str; (void)text_height; (void)rgba; }
IDOCN inline void RADLINK IggyIMEWin32GetCandidatePosition(Iggy* f, F32* pdx, F32* pdy, F32* pdcomp_str_height) { (void)f; if(pdx) *pdx = 0; if(pdy) *pdy = 0; if(pdcomp_str_height) *pdcomp_str_height = 0; }
IDOCN inline void* RADLINK IggyIMEGetFocusedTextfield(Iggy* f) { (void)f; return 0; }
IDOCN inline void RADLINK IggyIMEDrawRect(S32 x0, S32 y0, S32 x1, S32 y1, const U8 rgb[3]) { (void)x0; (void)y0; (void)x1; (void)y1; (void)rgb; }

#endif

// Focus handling
typedef void *IggyFocusHandle;

#define IGGY_FOCUS_NULL    0

typedef struct
{
   IggyFocusHandle object;
   F32 x0, y0, x1, y1;
} IggyFocusableObject;

inline rrbool RADLINK IggyPlayerGetFocusableObjects(Iggy *f, IggyFocusHandle *current_focus, IggyFocusableObject *objs, S32 max_obj, S32 *num_obj) { (void)f; (void)current_focus; (void)objs; (void)max_obj; if(num_obj) *num_obj = 0; return 0; }
inline void RADLINK IggyPlayerSetFocusRS(Iggy *f, IggyFocusHandle object, int focus_key_char) { (void)f; (void)object; (void)focus_key_char; }

// GDraw helper functions
inline void * RADLINK IggyGDrawMallocAnnotated(SINTa size, const char *file, int line) { (void)size; (void)file; (void)line; return 0; }
#define IggyGDrawMalloc(size)  IggyGDrawMallocAnnotated(size, __FILE__, __LINE__) IDOCN

inline void RADLINK IggyGDrawFree(void *ptr) { (void)ptr; }
inline void RADLINK IggyGDrawSendWarning(Iggy *f, char const *message, ...) { (void)f; (void)message; }
inline void RADLINK IggyWaitOnFence(void *id, U32 fence) { (void)id; (void)fence; }
inline void RADLINK IggyDiscardVertexBufferCallback(void *owner, void *vertex_buffer) { (void)owner; (void)vertex_buffer; }
inline void RADLINK IggyPlayerDebugEnableFilters(Iggy *f, rrbool enable) { (void)f; (void)enable; }
inline void RADLINK IggyPlayerDebugSetTime(Iggy *f, F64 time) { (void)f; (void)time; }

IDOCN inline void RADLINK IggyPlayerDebugBatchStartFrame(void) {}
IDOCN inline void RADLINK IggyPlayerDebugBatchInit(void) {}
IDOCN inline void RADLINK IggyPlayerDebugBatchMove(S32 dir) { (void)dir; }
IDOCN inline void RADLINK IggyPlayerDebugBatchSplit(void) {}
IDOCN inline void RADLINK IggyPlayerDebugBatchChooseEnd(S32 end) { (void)end; }

// Debugging
IDOCN inline void RADLINK IggyPlayerDebugUpdateReadyToTickWithFakeRender(Iggy *f) { (void)f; }
IDOCN inline void RADLINK IggyDebugBreakOnAS3Exception(void) {}

typedef struct
{
   S32 size;
   char *source_file;
   S32   source_line;
   char *iggy_file;
   char *info;
} IggyLeakResultData;

typedef void RADLINK IggyLeakResultCallback(IggyLeakResultData *data);

typedef struct
{
   char *subcategory;
   S32   subcategory_stringlen;
   S32   static_allocation_count;
   S32   static_allocation_bytes;
   S32   dynamic_allocation_count;
   S32   dynamic_allocation_bytes;
} IggyMemoryUseInfo;

inline rrbool RADLINK IggyDebugGetMemoryUseInfo(Iggy *player, IggyLibrary lib, char const *category_string, S32 category_stringlen, S32 iteration, IggyMemoryUseInfo *data) { (void)player; (void)lib; (void)category_string; (void)category_stringlen; (void)iteration; (void)data; return 0; }
inline void RADLINK IggyDebugSetLeakResultCallback(IggyLeakResultCallback *leak_result_func) { (void)leak_result_func; }

IDOCN inline void RADLINK iggy_sync_check_todisk(char *filename_or_null, U32 flags) { (void)filename_or_null; (void)flags; }
IDOCN inline void RADLINK iggy_sync_check_fromdisk(char *filename_or_null, U32 flags) { (void)filename_or_null; (void)flags; }
IDOCN inline void RADLINK iggy_sync_check_end(void) {}
#define IGGY_SYNCCHECK_readytotick  1U  IDOCN

RADDEFEND

#endif
