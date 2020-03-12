struct textured_vertex
{
	v4 P;
	v2 UV;
	u32 Color;
    u32 TextureIndex;
};

gb_global sg_pipeline GlobalPipeline = {};
gb_global sg_bindings GlobalBindings = {};
gb_global sg_buffer GlobalVertexBuffer = {};
gb_global sg_buffer GlobalIndexBuffer = {};

gb_global u32 GlobalVertexCount = 0;
gb_global textured_vertex* GlobalVertexArray = 0;

gb_global u32 GlobalIndexCount = 0;
gb_global u16* GlobalIndexArray = 0;

gb_global u32 GlobalMaxVertexCount = (1 << 8);
gb_global u32 GlobalMaxIndexCount = (1 << 8);

void RendererInit()
{
	sg_desc GFXDesc = {};
	sg_setup(&GFXDesc);

	u32 VertexSize = GlobalMaxVertexCount * sizeof(textured_vertex);
	GlobalVertexArray = (textured_vertex *)gb_malloc(VertexSize);

	u32 IndexSize = GlobalMaxIndexCount * sizeof(u16);
	GlobalIndexArray = (u16 *)gb_malloc(IndexSize);

	sg_buffer_desc VertexBufferDesc = {};
	VertexBufferDesc.size = VertexSize;
	VertexBufferDesc.content = 0;
	VertexBufferDesc.type = SG_BUFFERTYPE_VERTEXBUFFER;
	VertexBufferDesc.usage = SG_USAGE_STREAM;
	VertexBufferDesc.label = "VertexBuffer";
	GlobalVertexBuffer = sg_make_buffer(&VertexBufferDesc);

	sg_buffer_desc IndexBufferDesc = {};
	IndexBufferDesc.size = IndexSize;
	IndexBufferDesc.content = 0;
	IndexBufferDesc.type = SG_BUFFERTYPE_INDEXBUFFER;
	IndexBufferDesc.usage = SG_USAGE_STREAM;
	IndexBufferDesc.label = "IndexBuffer";
	GlobalIndexBuffer = sg_make_buffer(&IndexBufferDesc);

	u32 ImageContentSize = sizeof(u32) * 2 * TEXTURE_ARRAY_DIM * TEXTURE_ARRAY_DIM;
	void* PixelContent = gb_malloc(ImageContentSize);
	defer(gb_mfree(PixelContent));

	gbFile* TilesetFile = gb_file_get_standard(gbFileStandard_Input);
	gbFileError TilesetOpenFileError = gb_file_open_mode(TilesetFile, gbFileMode_Read, "../data/roguelike_tileset.png");
	Assert(TilesetOpenFileError == gbFileError_None);

	i64 TilesetFileSize = gb_file_size(TilesetFile);
	Assert(TilesetFileSize > 0);

	u8* TilesetFileContent = (u8 *)gb_malloc(TilesetFileSize);
	gb_file_read(TilesetFile, TilesetFileContent, TilesetFileSize);

	gbFileError TilesetCloseFileError = gb_file_close(TilesetFile);
	Assert(TilesetCloseFileError == gbFileError_None);

	i32 TilesetWidth = 0;
	i32 TilesetHeight = 0;
	i32 TilesetChannelCount = 0;
	i32 DesiredChannels = 4;

	u8* TilesetPixels = stbi_load_from_memory(TilesetFileContent, TilesetFileSize, &TilesetWidth, &TilesetHeight, &TilesetChannelCount, DesiredChannels);
	gb_mfree(TilesetFileContent);

	gb_memcopy(PixelContent, TilesetPixels, TilesetWidth * TilesetHeight * DesiredChannels);

	stbi_image_free(TilesetPixels);

	sg_image_content ImageContent = {};
	ImageContent.subimage[0][0].ptr = PixelContent;
	ImageContent.subimage[0][0].size = ImageContentSize;

    sg_image_desc ImageDesc = {};
    ImageDesc.width = TEXTURE_ARRAY_DIM;
    ImageDesc.height = TEXTURE_ARRAY_DIM;
	ImageDesc.layers = 2;
    ImageDesc.usage = SG_USAGE_IMMUTABLE;
    ImageDesc.type = SG_IMAGETYPE_ARRAY;
    ImageDesc.min_filter = SG_FILTER_NEAREST;
    ImageDesc.mag_filter = SG_FILTER_NEAREST;
    ImageDesc.wrap_u = SG_WRAP_MIRRORED_REPEAT;
    ImageDesc.wrap_v = SG_WRAP_MIRRORED_REPEAT;
    ImageDesc.wrap_w = SG_WRAP_MIRRORED_REPEAT;
    ImageDesc.content = ImageContent;
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

		void main()
		{
            vec3 ArrayUV = vec3(FragUV.x, FragUV.y, float(FragTexIndex));
            float textureA = texture(TextureSampler, ArrayUV).a;
			FinalColor = FragColor * textureA;
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
	PipelineDesc.layout.attrs[3].format = SG_VERTEXFORMAT_UBYTE4;
	PipelineDesc.index_type = SG_INDEXTYPE_UINT16;
    PipelineDesc.blend.enabled = true;
    PipelineDesc.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    PipelineDesc.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	PipelineDesc.label = "Pipeline";

	GlobalPipeline = sg_make_pipeline(&PipelineDesc);

	GlobalBindings.vertex_buffers[0] = GlobalVertexBuffer;
	GlobalBindings.index_buffer = GlobalIndexBuffer;
	GlobalBindings.fs_images[0] = Image;
}

void RendererBeginFrame()
{
	GlobalVertexCount = 0;
	GlobalIndexCount = 0;
}

void RendererEndFrame()
{
	u32 AppWidth = sapp_width();
	u32 AppHeight = sapp_height();
	sg_pass_action Action = {};
	Action.colors[0].action = SG_ACTION_CLEAR;
	Action.colors[0].val[0] = 0.2f;
	Action.colors[0].val[1] = 0.2f;
	Action.colors[0].val[2] = 0.2f;
	Action.colors[0].val[3] = 1.0f;

	sg_update_buffer(GlobalVertexBuffer, GlobalVertexArray, GlobalVertexCount * sizeof(textured_vertex));
	sg_update_buffer(GlobalIndexBuffer, GlobalIndexArray, GlobalIndexCount * sizeof(u16));

	sg_begin_default_pass(&Action, AppWidth, AppHeight);
	sg_apply_pipeline(GlobalPipeline);
	sg_apply_bindings(&GlobalBindings);
	sg_draw(0, GlobalIndexCount, 1);
	sg_end_pass();

	sg_commit();
}

inline void PushQuad(u32 TextureIndex,
		v4 P0, v2 UV0, u32 C0,
		v4 P1, v2 UV1, u32 C1,
		v4 P2, v2 UV2, u32 C2,
		v4 P3, v2 UV3, u32 C3)
{
    u32 VertIndex = GlobalVertexCount;
    u32 IndexIndex = GlobalIndexCount;

    GlobalVertexCount += 4;
    GlobalIndexCount += 6;
    Assert(GlobalVertexCount <= GlobalMaxVertexCount);
    Assert(GlobalIndexCount <= GlobalMaxIndexCount);

    textured_vertex *Vert = GlobalVertexArray + VertIndex;
    u16 *Index = GlobalIndexArray + IndexIndex;

    v2 InvUV = {1.0f / (f32)TEXTURE_ARRAY_DIM, 1.0f / (f32)TEXTURE_ARRAY_DIM};

    UV0 = Hadamard(InvUV, UV0);
    UV1 = Hadamard(InvUV, UV1);
    UV2 = Hadamard(InvUV, UV2);
    UV3 = Hadamard(InvUV, UV3);

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

inline void PushQuad(u32 TextureIndex,
		v4 P0, v2 UV0, v4 C0,
		v4 P1, v2 UV1, v4 C1,
		v4 P2, v2 UV2, v4 C2,
		v4 P3, v2 UV3, v4 C3)
{
	PushQuad(TextureIndex,
			P0, UV0, RGBAPack4x8(255.0f * C0),
			P1, UV1, RGBAPack4x8(255.0f * C1),
			P2, UV2, RGBAPack4x8(255.0f * C2),
			P3, UV3, RGBAPack4x8(255.0f * C3));
}
inline void PushBitmap(u32 TextureIndex, rectangle2 SourceRect, rectangle2 DestRect, v4 Color)
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

	PushQuad(TextureIndex,
			P0, UV0, C0,
			P1, UV1, C1,
			P2, UV2, C2,
			P3, UV3, C3);
}

