
struct game_button_state
{
    int HalfTransitionCount;
    b32 EndedDown;
};
    
struct game_controller_input
{
    b32 IsConnected;
    b32 IsAnalog;
    f32 StickAverageX;
    f32 StickAverageY;
    f32 ClutchMax; // NOTE(casey): This is the "dodge" clutch, eg. triggers or space bar?
    
    union
    {
        game_button_state Buttons[12];
        struct
        {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;
            
            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;
            
            game_button_state LeftShoulder;
            game_button_state RightShoulder;
            
            game_button_state Back;
            game_button_state Start;
            
            // NOTE(casey): All buttons must be added above this line
            
            game_button_state Terminator;
        };
    };
};
    
enum game_input_mouse_button
{
    PlatformMouseButton_Left,
    PlatformMouseButton_Middle,
    PlatformMouseButton_Right,
    PlatformMouseButton_Extended0,
    PlatformMouseButton_Extended1,
    
    PlatformMouseButton_Count,
};

struct game_input
{
    f32 dtForFrame;
    
    game_controller_input KeyboardController;
    
    // NOTE(casey): Signals back to the platform layer
    b32 QuitRequested;
    
    // NOTE(casey): For debugging only
    game_button_state MouseButtons[PlatformMouseButton_Count];
    v3 ClipSpaceMouseP;
    b32 ShiftDown, AltDown, ControlDown;
    b32 FKeyPressed[13]; // NOTE(casey): 1 is F1, etc., for clarity - 0 is not used!
};

inline b32 WasPressed(game_button_state State)
{
    b32 Result = ((State.HalfTransitionCount > 1) ||
                  ((State.HalfTransitionCount == 1) && (State.EndedDown)));
    
    return(Result);
}

inline b32 IsDown(game_button_state State)
{
    b32 Result = (State.EndedDown);
    
    return(Result);
}

