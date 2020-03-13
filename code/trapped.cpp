#include <stdio.h>

#define SOKOL_WIN32_FORCE_MAIN
#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "sokol_app.h"
#include "sokol_gfx.h"

#include "sokol_time.h"

#undef _GNU_SOURCE
#undef _LARGEFILE64_SOURCE
#undef WIN32_LEAN_AND_MEAN
#define GB_IMPLEMENTATION
#include "gb.h"
#undef F32_MAX
#define F32_MAX 3.40282346e+38f

#define Assert(cond) GB_ASSERT(cond)
#define ArrayCount(x) gb_count_of(x)
#define ZeroStruct(x) gb_zero_item(x)

#include "types.h"
#include "intrinsics.h"
#include "math.h"

#include "renderer.cpp"
#include "input.h"

gb_global u64 LastTime = 0;
gb_global renderer_state RendererState = {};
gb_global game_input GlobalInput[2] = {};
gb_global game_input* NewInput = nullptr;
gb_global game_input* OldInput = nullptr;

#include "game.h"

void AppInit()
{
	RendererInit(&RendererState);

    stm_setup();

    NewInput = &GlobalInput[0];
    OldInput = &GlobalInput[1];
}

void AppFrame()
{
    {
        const double dt = stm_sec(stm_laptime(&LastTime));

        NewInput->dtForFrame = dt;
    }

	RendererBeginFrame(&RendererState);

    GameUpdateAndRender(NewInput, &RendererState);
    if(NewInput->QuitRequested)
    {
        sapp_quit();
    }

	RendererEndFrame(&RendererState);

    {
        game_input *Temp = NewInput;
        NewInput = OldInput;
        OldInput = Temp;

        // TODO(hugo)
        // Find out how to do this at the beginning of the frame
        // instead of the end. Because Sokol calls the event loop before the frame, so I cannot
        // clear the inputs at the beginning of the frame otherwise I would clean what
        // was just written...
        game_controller_input* OldKeyboardController = &OldInput->KeyboardController;
        game_controller_input* NewKeyboardController = &NewInput->KeyboardController;
        *NewKeyboardController = {};
        for(int ButtonIndex = 0;
                ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
                ++ButtonIndex)
        {
            NewKeyboardController->Buttons[ButtonIndex].EndedDown =
                OldKeyboardController->Buttons[ButtonIndex].EndedDown;
        }
        for(u32 ButtonIndex = 0;
                ButtonIndex < PlatformMouseButton_Count;
                ++ButtonIndex)
        {
            NewInput->MouseButtons[ButtonIndex] = OldInput->MouseButtons[ButtonIndex];
            NewInput->MouseButtons[ButtonIndex].HalfTransitionCount = 0;
        }
        ZeroStruct(NewInput->FKeyPressed);
    }
}

void AppCleanup()
{
	RendererShutdown();
}

gb_internal void ProcessKeyboardMessage(game_button_state* NewState, b32 IsDown)
{
    if(NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

void AppEvent(const sapp_event* Event)
{
	switch(Event->type)
	{
		case SAPP_EVENTTYPE_KEY_DOWN:
		case SAPP_EVENTTYPE_KEY_UP:
			{
                b32 IsDown = Event->type == SAPP_EVENTTYPE_KEY_DOWN;
				if(!Event->key_repeat)
				{
					if(Event->key_code == SAPP_KEYCODE_LEFT)
					{
                        ProcessKeyboardMessage(&NewInput->KeyboardController.MoveLeft, IsDown);
					}
					else if(Event->key_code == SAPP_KEYCODE_RIGHT)
					{
                        ProcessKeyboardMessage(&NewInput->KeyboardController.MoveRight, IsDown);
					}
					else if(Event->key_code == SAPP_KEYCODE_UP)
					{
                        ProcessKeyboardMessage(&NewInput->KeyboardController.MoveUp, IsDown);
					}
					else if(Event->key_code == SAPP_KEYCODE_DOWN)
					{
                        ProcessKeyboardMessage(&NewInput->KeyboardController.MoveDown, IsDown);
					}
					else if(Event->key_code >= SAPP_KEYCODE_F1 && Event->key_code <= SAPP_KEYCODE_F12)
					{
						NewInput->FKeyPressed[Event->key_code + 1 - SAPP_KEYCODE_F1] = IsDown;
					}
				}
			} break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            {
                f32 MouseX = Event->mouse_x;
                f32 MouseY = Event->mouse_y;
                NewInput->ClipSpaceMouseP.x = ClampBinormalMapToRange(0.0f, MouseX, sapp_width());
                NewInput->ClipSpaceMouseP.y = ClampBinormalMapToRange(0.0f, MouseY, sapp_height());
                NewInput->ClipSpaceMouseP.z = 0.0f;
            } break;
        case SAPP_EVENTTYPE_MOUSE_UP:
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            {
                b32 IsDown = Event->type == SAPP_EVENTTYPE_MOUSE_DOWN;
                ProcessKeyboardMessage(&NewInput->MouseButtons[Event->mouse_button], IsDown);

            } break;
		default:
			{
			} break;
	}
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
