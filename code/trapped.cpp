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

gb_global renderer_state RendererState = {};

void AppInit()
{
	RendererInit(&RendererState);
}

void AppFrame()
{
	RendererBeginFrame(&RendererState);

	PushTile(&RendererState, 0, V2(0.0f, 0.0f), V2(0, 13), V4(1.0f, 1.0f, 1.0f, 1.0f));
	PushTile(&RendererState, 0, V2(10, 10), V2(15, 0), V4(1.0f, 1.0f, 1.0f, 1.0f));

	RendererEndFrame(&RendererState);
}

void AppCleanup()
{
	RendererShutdown();
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
