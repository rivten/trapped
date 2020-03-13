
struct game_state
{
    f32 Time;
};

gb_global game_state GameState = {};

void GameUpdateAndRender(game_input* Input, renderer_state* Renderer)
{
    GameState.Time += Input->dtForFrame;
	PushTile(Renderer, V2(0.0f, 0.0f), V2(0, 13), V4(1.0f, 1.0f, 1.0f, 1.0f));
	PushTile(Renderer, V2(10, 10), V2(15, 0), V4(1.0f, 1.0f, 1.0f, 1.0f));
    
    if(Input->FKeyPressed[1])
    {
        PushTextWithShadow(Renderer, "Hello, sailor!", V2(100 * Sin(0.5f * GameState.Time), 0), V4(1, 0, 1, 1));
    }
}

