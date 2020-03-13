#define STB_TRUETYPE_IMPLEMENTATION
//#include "stb_rect_pack.h"
#include "stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TEXTURE_ARRAY_DIM 512

struct textured_vertex
{
	v4 P;
	v2 UV;
	u32 Color;
    u32 TextureIndex;
};

struct renderer_state
{
	sg_pipeline Pipeline;
	sg_bindings Bindings;
	sg_buffer VertexBuffer;
	sg_buffer IndexBuffer;

	u32 VertexCount;
	textured_vertex* VertexArray;

	u32 IndexCount;
	u16* IndexArray;

    u32 MaxQuadCount;
	u32 MaxVertexCount;
	u32 MaxIndexCount;

	u32 FontCharBegin;
	u32 FontCharCount;
	stbtt_bakedchar* BakedCharData;
};

enum texture_type
{
    TextureType_White,
    TextureType_Tileset,
    TextureType_FontAtlas,

    TextureType_Count,
};

gb_internal void RendererInit(renderer_state* State)
{
	sg_desc GFXDesc = {};
	sg_setup(&GFXDesc);

    State->MaxQuadCount = 2*256;
	State->MaxVertexCount = 4 * State->MaxQuadCount;
	State->MaxIndexCount = 6 * State->MaxQuadCount;

	u32 VertexSize = State->MaxVertexCount * sizeof(textured_vertex);
	State->VertexArray = (textured_vertex *)gb_malloc(VertexSize);

	u32 IndexSize = State->MaxIndexCount * sizeof(u16);
	State->IndexArray = (u16 *)gb_malloc(IndexSize);

	sg_buffer_desc VertexBufferDesc = {};
	VertexBufferDesc.size = VertexSize;
	VertexBufferDesc.content = 0;
	VertexBufferDesc.type = SG_BUFFERTYPE_VERTEXBUFFER;
	VertexBufferDesc.usage = SG_USAGE_STREAM;
	VertexBufferDesc.label = "VertexBuffer";
	State->VertexBuffer = sg_make_buffer(&VertexBufferDesc);

	sg_buffer_desc IndexBufferDesc = {};
	IndexBufferDesc.size = IndexSize;
	IndexBufferDesc.content = 0;
	IndexBufferDesc.type = SG_BUFFERTYPE_INDEXBUFFER;
	IndexBufferDesc.usage = SG_USAGE_STREAM;
	IndexBufferDesc.label = "IndexBuffer";
	State->IndexBuffer = sg_make_buffer(&IndexBufferDesc);

    u32 SliceContentSize = 4 * TEXTURE_ARRAY_DIM * TEXTURE_ARRAY_DIM;
	u32 ImageContentSize = TextureType_Count * SliceContentSize;
	u8* PixelContent = (u8 *)gb_malloc(ImageContentSize);
	defer(gb_mfree(PixelContent));

    {
        // NOTE(hugo): Load the white texture
        for(u32 PixelByteIndex = 0; PixelByteIndex < SliceContentSize; ++PixelByteIndex)
        {
            (PixelContent + TextureType_White * SliceContentSize)[PixelByteIndex] = 0xFF;
        }
    }

	{
		// NOTE(hugo): Loading the tileset bitmap
		gbFile* TilesetFile = gb_file_get_standard(gbFileStandard_Input);
		gbFileError TilesetOpenFileError = gb_file_open_mode(TilesetFile, gbFileMode_Read, "../data/roguelike_tileset.png");
		Assert(TilesetOpenFileError == gbFileError_None);

		i64 TilesetFileSize = gb_file_size(TilesetFile);
		Assert(TilesetFileSize > 0);

		u8* TilesetFileContent = (u8 *)gb_malloc(TilesetFileSize);
		gb_file_read(TilesetFile, TilesetFileContent, TilesetFileSize);
		defer(gb_mfree(TilesetFileContent));

		gbFileError TilesetCloseFileError = gb_file_close(TilesetFile);
		Assert(TilesetCloseFileError == gbFileError_None);

		i32 TilesetWidth = 0;
		i32 TilesetHeight = 0;
		i32 TilesetChannelCount = 0;
		i32 DesiredChannels = 4;

		u8* TilesetPixels = stbi_load_from_memory(TilesetFileContent, TilesetFileSize, &TilesetWidth, &TilesetHeight, &TilesetChannelCount, DesiredChannels);
		defer(stbi_image_free(TilesetPixels));

		u32 TilesetByteSize = TilesetWidth * TilesetHeight * DesiredChannels;

		for(u32 ByteIndex = 0; ByteIndex < TilesetByteSize; ++ByteIndex)
		{
			if(ByteIndex % 4 != 3)
			{
				TilesetPixels[ByteIndex] = 255;
			}
		}

		gb_memcopy(PixelContent + TextureType_Tileset * SliceContentSize, TilesetPixels, TilesetByteSize);
	}

	{
		// NOTE(hugo): Loading the font bitmap
		gbFile* FontFile = gb_file_get_standard(gbFileStandard_Input);
		gbFileError FontOpenFileError = gb_file_open_mode(FontFile, gbFileMode_Read, "../data/LiberationMono-Regular.ttf");
		Assert(FontOpenFileError == gbFileError_None);

		i64 FontFileSize = gb_file_size(FontFile);
		Assert(FontFileSize > 0);

		u8* FontFileContent = (u8 *)gb_malloc(FontFileSize);
		gb_file_read(FontFile, FontFileContent, FontFileSize);
		defer(gb_mfree(FontFileContent));

		gbFileError FontCloseFileError = gb_file_close(FontFile);
		Assert(FontCloseFileError == gbFileError_None);

		State->FontCharBegin = 32;
		State->FontCharCount = 96;
		State->BakedCharData = (stbtt_bakedchar *)gb_malloc(State->FontCharCount * sizeof(stbtt_bakedchar));
		f32 FontHeight = 32.0f;

		// TODO(hugo): Do this on the stack ??
		u8* TempFontPixels = (u8 *)gb_malloc(TEXTURE_ARRAY_DIM * TEXTURE_ARRAY_DIM);
		defer(gb_mfree(TempFontPixels));

		stbtt_BakeFontBitmap(FontFileContent, 0, FontHeight, TempFontPixels, TEXTURE_ARRAY_DIM, TEXTURE_ARRAY_DIM, State->FontCharBegin, State->FontCharCount, State->BakedCharData);

		u32 FontByteSize = SliceContentSize;
		u8* FontPixels = (u8 *)gb_malloc(FontByteSize);
		defer(gb_mfree(FontPixels));
		for(u32 ByteIndex = 0; ByteIndex < TEXTURE_ARRAY_DIM * TEXTURE_ARRAY_DIM; ++ByteIndex)
		{
			FontPixels[4 * ByteIndex + 0] = 255;
			FontPixels[4 * ByteIndex + 1] = 255;
			FontPixels[4 * ByteIndex + 2] = 255;
			FontPixels[4 * ByteIndex + 3] = TempFontPixels[ByteIndex];
		}

		gb_memcopy(PixelContent + TextureType_FontAtlas * SliceContentSize, FontPixels, FontByteSize);
	}

    sg_image_desc ImageDesc = {};
    ImageDesc.width = TEXTURE_ARRAY_DIM;
    ImageDesc.height = TEXTURE_ARRAY_DIM;
	ImageDesc.layers = TextureType_Count;
    ImageDesc.usage = SG_USAGE_IMMUTABLE;
    ImageDesc.type = SG_IMAGETYPE_ARRAY;
    ImageDesc.min_filter = SG_FILTER_NEAREST;
    ImageDesc.mag_filter = SG_FILTER_NEAREST;
    ImageDesc.wrap_u = SG_WRAP_MIRRORED_REPEAT;
    ImageDesc.wrap_v = SG_WRAP_MIRRORED_REPEAT;
    ImageDesc.wrap_w = SG_WRAP_MIRRORED_REPEAT;
    ImageDesc.pixel_format = SG_PIXELFORMAT_RGBA8;
    ImageDesc.content.subimage[0][0].ptr = PixelContent;
    ImageDesc.content.subimage[0][0].size = ImageContentSize;
    sg_image Image = sg_make_image(&ImageDesc);

	sg_shader_desc ShaderDesc = {};
	ShaderDesc.attrs[0].name = "VertP";
	ShaderDesc.attrs[1].name = "VertUV";
	ShaderDesc.attrs[2].name = "VertColor";
	ShaderDesc.attrs[3].name = "VertTexIndex";
	ShaderDesc.vs.source = R"FOO(
		#version 330 core

		in vec4 VertP;
		in vec2 VertUV;
		in vec4 VertColor;
        in int VertTexIndex;

		smooth out vec2 FragUV;
		smooth out vec4 FragColor;
        flat out int FragTexIndex;

		void main()
		{
			gl_Position = VertP;
			FragUV = VertUV;
			FragColor = VertColor;
            FragTexIndex = VertTexIndex;
		}
	)FOO";

	ShaderDesc.fs.source = R"FOO(
		#version 330 core
		smooth in vec2 FragUV;
		smooth in vec4 FragColor;
        flat in int FragTexIndex;

		uniform sampler2DArray TextureSampler;

		out vec4 FinalColor;

        vec4 Hadamard(vec4 A, vec4 B)
        {
            return(vec4(A.x * B.x, A.y * B.y, A.z * B.z, A.w * B.w));
        }

		void main()
		{
            vec3 ArrayUV = vec3(FragUV.x, FragUV.y, float(FragTexIndex));
            vec4 TextureColor = texture(TextureSampler, ArrayUV);
			FinalColor = Hadamard(FragColor, TextureColor);
		}
	)FOO";
	ShaderDesc.fs.images[0].type = SG_IMAGETYPE_ARRAY;
	ShaderDesc.fs.images[0].name = "TextureSampler";
	sg_shader Shader = sg_make_shader(&ShaderDesc);

	sg_pipeline_desc PipelineDesc = {};
	PipelineDesc.shader = Shader,
	PipelineDesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT4;
	PipelineDesc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
	PipelineDesc.layout.attrs[2].format = SG_VERTEXFORMAT_UBYTE4N;
	PipelineDesc.layout.attrs[3].format = SG_VERTEXFORMAT_FLOAT;
	PipelineDesc.index_type = SG_INDEXTYPE_UINT16;
    PipelineDesc.blend.enabled = true;
    PipelineDesc.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    PipelineDesc.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	PipelineDesc.label = "Pipeline";

	State->Pipeline = sg_make_pipeline(&PipelineDesc);

	State->Bindings.vertex_buffers[0] = State->VertexBuffer;
	State->Bindings.index_buffer = State->IndexBuffer;
	State->Bindings.fs_images[0] = Image;
}

gb_internal void RendererBeginFrame(renderer_state* State)
{
	State->VertexCount = 0;
	State->IndexCount = 0;
}

gb_internal void RendererEndFrame(renderer_state* State)
{
	u32 AppWidth = sapp_width();
	u32 AppHeight = sapp_height();
	sg_pass_action Action = {};
	Action.colors[0].action = SG_ACTION_CLEAR;
	Action.colors[0].val[0] = 0.0f;
	Action.colors[0].val[1] = 0.0f;
	Action.colors[0].val[2] = 0.0f;
	Action.colors[0].val[3] = 1.0f;

	sg_update_buffer(State->VertexBuffer, State->VertexArray, State->VertexCount * sizeof(textured_vertex));
	sg_update_buffer(State->IndexBuffer, State->IndexArray, State->IndexCount * sizeof(u16));

	sg_begin_default_pass(&Action, AppWidth, AppHeight);
	sg_apply_pipeline(State->Pipeline);
	sg_apply_bindings(&State->Bindings);
	sg_draw(0, State->IndexCount, 1);
	sg_end_pass();

	sg_commit();
}

void RendererShutdown()
{
	sg_shutdown();
}

inline void PushQuad(renderer_state* State, texture_type TextureType,
		v4 P0, v2 UV0, u32 C0,
		v4 P1, v2 UV1, u32 C1,
		v4 P2, v2 UV2, u32 C2,
		v4 P3, v2 UV3, u32 C3)
{
    u32 VertIndex = State->VertexCount;
    u32 IndexIndex = State->IndexCount;

    State->VertexCount += 4;
    State->IndexCount += 6;
    Assert(State->VertexCount <= State->MaxVertexCount);
    Assert(State->IndexCount <= State->MaxIndexCount);

    textured_vertex *Vert = State->VertexArray + VertIndex;
    u16 *Index = State->IndexArray + IndexIndex;

    v2 InvUV = {1.0f / (f32)TEXTURE_ARRAY_DIM, 1.0f / (f32)TEXTURE_ARRAY_DIM};

    UV0 = Hadamard(InvUV, UV0);
    UV1 = Hadamard(InvUV, UV1);
    UV2 = Hadamard(InvUV, UV2);
    UV3 = Hadamard(InvUV, UV3);

    Assert(TextureType < TextureType_Count);
    u32 TextureIndex = (u32)(TextureType);
    Vert[0].P = P3;
    Vert[0].UV = UV3;
    Vert[0].Color = C3;
    Vert[0].TextureIndex = TextureIndex;
    
    Vert[1].P = P0;
    Vert[1].UV = UV0;
    Vert[1].Color = C0;
    Vert[1].TextureIndex = TextureIndex;
    
    Vert[2].P = P2;
    Vert[2].UV = UV2;
    Vert[2].Color = C2;
    Vert[2].TextureIndex = TextureIndex;
    
    Vert[3].P = P1;
    Vert[3].UV = UV1;
    Vert[3].Color = C1;
    Vert[3].TextureIndex = TextureIndex;
    
    u32 BaseIndex = VertIndex;
    u16 VI = (u16)BaseIndex;
    Assert((u32)VI == BaseIndex);
    
    Index[0] = VI + 0;
    Index[1] = VI + 1;
    Index[2] = VI + 2;
    Index[3] = VI + 1;
    Index[4] = VI + 3;
    Index[5] = VI + 2;
}

inline void PushQuad(renderer_state* State, texture_type TextureType,
		v4 P0, v2 UV0, v4 C0,
		v4 P1, v2 UV1, v4 C1,
		v4 P2, v2 UV2, v4 C2,
		v4 P3, v2 UV3, v4 C3)
{
	PushQuad(State, TextureType,
			P0, UV0, RGBAPack4x8(255.0f * C0),
			P1, UV1, RGBAPack4x8(255.0f * C1),
			P2, UV2, RGBAPack4x8(255.0f * C2),
			P3, UV3, RGBAPack4x8(255.0f * C3));
}
inline void PushBitmap(renderer_state* State, texture_type TextureType, rectangle2 SourceRect, rectangle2 DestRect, v4 Color)
{
    // NOTE(hugo): This might look a little bit fancy... and it is !!
    // Without this line, the rendering of exact pixel perfect tileset is fucked up
    // at certain resolution. 
    // I could exactly figure out why this is the case but it seems that the rounding
    // might be the answer...
    v2 UVOffset = (-0.5f / (f32)TEXTURE_ARRAY_DIM) * V2(1, 1);

	v4 P0 = V4(2.0f * DestRect.Min.x / (float)sapp_width(), 2.0f * DestRect.Min.y / (float)sapp_height(), 0.0f, 1.0f);
	v2 UV0 = V2(SourceRect.Min.x, SourceRect.Max.y) + UVOffset;
	v4 C0 = Color;

	v4 P1 = V4(2.0f * DestRect.Max.x / (float)sapp_width(), 2.0f * DestRect.Min.y / (float)sapp_height(), 0.0f, 1.0f); 
	v2 UV1 = V2(SourceRect.Max.x, SourceRect.Max.y) + UVOffset; 
	v4 C1 = Color;

	v4 P2 = V4(2.0f * DestRect.Max.x / (float)sapp_width(), 2.0f * DestRect.Max.y / (float)sapp_height(), 0.0f, 1.0f);
	v2 UV2 = V2(SourceRect.Max.x, SourceRect.Min.y) + UVOffset;
	v4 C2 = Color;

	v4 P3 = V4(2.0f * DestRect.Min.x / (float)sapp_width(), 2.0f * DestRect.Max.y / (float)sapp_height(), 0.0f, 1.0f);
	v2 UV3 = V2(SourceRect.Min.x, SourceRect.Min.y) + UVOffset;
	v4 C3 = Color;

	PushQuad(State, TextureType,
			P0, UV0, C0,
			P1, UV1, C1,
			P2, UV2, C2,
			P3, UV3, C3);
}

#define TILE_SIZE_IN_PIXELS 16
gb_internal void PushTile(renderer_state* State, v2 Pos, v2 TileSetPos, v4 Color)
{
    rectangle2 DestRect = RectCenterHalfDim(TILE_SIZE_IN_PIXELS * Pos, 0.5f * V2(TILE_SIZE_IN_PIXELS, TILE_SIZE_IN_PIXELS));
    rectangle2 SourceRect = RectMinDim(TILE_SIZE_IN_PIXELS * TileSetPos, V2(TILE_SIZE_IN_PIXELS, TILE_SIZE_IN_PIXELS));
	PushBitmap(State, TextureType_Tileset, SourceRect, DestRect, Color);
}

void PushChar(renderer_state* State, u8 C, v2* P, v4 Color)
{
    stbtt_aligned_quad Quad = {};
    Assert(C >= State->FontCharBegin);
    Assert(C < State->FontCharBegin + State->FontCharCount);
    stbtt_GetBakedQuad(State->BakedCharData, TEXTURE_ARRAY_DIM, TEXTURE_ARRAY_DIM, C - State->FontCharBegin, &P->x, &P->y, &Quad, true);

    rectangle2 SourceRect = {};
    SourceRect.Min.x = TEXTURE_ARRAY_DIM * Quad.s0;
    SourceRect.Min.y = TEXTURE_ARRAY_DIM * Quad.t0;
    SourceRect.Max.x = TEXTURE_ARRAY_DIM * Quad.s1;
    SourceRect.Max.y = TEXTURE_ARRAY_DIM * Quad.t1;

    rectangle2 DestRect = {};
    DestRect.Min.x = Quad.x0;
    DestRect.Min.y = -Quad.y1;
    DestRect.Max.x = Quad.x1;
    DestRect.Max.y = -Quad.y0;
    PushBitmap(State, TextureType_FontAtlas, SourceRect, DestRect, Color);
}

void PushText(renderer_state* State, char* Text, v2 P, v4 Color)
{
    v2 TempP = V2(P.x, -P.y);
    usize Len = strlen(Text);
    for(u32 Index = 0; Index < Len; ++Index)
    {
        PushChar(State, Text[Index], &TempP, Color);
    }
}

gb_internal void PushTextWithShadow(renderer_state* State, char* Text, v2 P, v4 Color)
{
    PushText(State, Text, P + V2(2, -2), V4(0, 0, 0, 1));
    PushText(State, Text, P, Color);
}

gb_internal void PushRect(renderer_state* State, v2 Min, v2 Max, v4 Color)
{
    rectangle2 SourceRect = {};
    SourceRect.Min = V2(0, 0);
    SourceRect.Max = V2(TEXTURE_ARRAY_DIM, TEXTURE_ARRAY_DIM);

    rectangle2 DestRect = {};
    DestRect.Min = Min;
    DestRect.Max = Max;
    PushBitmap(State, TextureType_White, SourceRect, DestRect, Color);
}
