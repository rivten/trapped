#include <stdio.h>

#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "sokol_app.h"
#include "sokol_gfx.h"

#undef _GNU_SOURCE
#undef _LARGEFILE64_SOURCE
#define GB_IMPLEMENTATION
#include "gb.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TEXTURE_ARRAY_DIM 512

#define Assert(cond) GB_ASSERT(cond)

#include "types.h"
#include "intrinsics.h"
#include "math.h"

#include "renderer.cpp"

void AppInit()
{
	RendererInit();
}

#define TILE_SIZE_IN_PIXELS 16
gb_internal void PushTile(u32 TextureIndex, v2 Pos, v2 TileSetPos, v4 Color)
{
    rectangle2 DestRect = RectCenterHalfDim(TILE_SIZE_IN_PIXELS * Pos, 0.5f * V2(TILE_SIZE_IN_PIXELS, TILE_SIZE_IN_PIXELS));
    rectangle2 SourceRect = RectMinDim(TILE_SIZE_IN_PIXELS * TileSetPos, V2(TILE_SIZE_IN_PIXELS, TILE_SIZE_IN_PIXELS));
	PushBitmap(TextureIndex, SourceRect, DestRect, Color);
}

void AppFrame()
{
	RendererBeginFrame();

	PushTile(0, V2(0.0f, 0.0f), V2(0, 13), V4(1.0f, 1.0f, 1.0f, 1.0f));
	PushTile(0, V2(10, 10), V2(15, 0), V4(1.0f, 1.0f, 1.0f, 1.0f));

	RendererEndFrame();
}

void AppCleanup()
{
	sg_shutdown();
}

void AppEvent(const sapp_event* Event)
{
}

sapp_desc sokol_main(int ArgumentCount, char** Arguments)
{
	sapp_desc AppDesc = {};
	AppDesc.init_cb = AppInit;
	AppDesc.frame_cb = AppFrame;
	AppDesc.cleanup_cb = AppCleanup;
	AppDesc.event_cb = AppEvent;
	AppDesc.width = 800;
	AppDesc.height = 600;
	return(AppDesc);
}
