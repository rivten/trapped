#include <stdio.h>

#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "sokol_app.h"
#include "sokol_gfx.h"

#undef _GNU_SOURCE
#undef _LARGEFILE64_SOURCE
#undef WIN32_LEAN_AND_MEAN
#define GB_IMPLEMENTATION
#include "gb.h"
#undef F32_MAX
#define F32_MAX 3.40282346e+38f

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

    PushText(&RendererState, "Hello, sailor!", V2(2.0f, 2.0f), V4(0, 0, 0, 1));
    PushText(&RendererState, "Hello, sailor!", V2(0, 0), V4(1, 0, 1, 1));

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
