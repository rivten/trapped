
gb_global v4 ColorPalette0[] // https://www.color-hex.com/color-palette/88278
{
	1.0f / 255.0f * V4(61, 49, 91, 255),
	1.0f / 255.0f * V4(70, 77, 112, 255),
	1.0f / 255.0f * V4(110, 138, 115, 255),
	1.0f / 255.0f * V4(155, 185, 122, 255),
	1.0f / 255.0f * V4(246, 247, 142, 255),
};

enum entity_type
{
	EntityType_Player,
	EntityType_Wall,
	EntityType_Ground,

	EntityType_Count,
};

gb_global v2 TileSetPosFromType[]
{
	V2(0, 13), // EntityType_Player,
	V2(15, 0), // EntityType_Wall,
	V2(8, 0), // EntityType_Ground,
};

enum entity_flag
{
	EntityFlag_None = 0,
	EntityFlag_RoomConnection = 1 << 0,
};

struct entity
{
	v2 P;
	v2 TileSetP;
	v4 Color;
	entity_type Type;
	u32 Flags;
};

struct game_state
{
    v2 PlayerPos;
	f32 Timer;

	u32 EntityCount;
	entity Entities[256];

	f32 JustSteppedOnConnectionTimer;
    f32 TextSize;
    b32 Initialized;
};

void PushEntity(game_state* GameState, v2 P, v4 C, entity_type Type, u32 Flags = EntityFlag_None)
{
	Assert(GameState->EntityCount < ArrayCount(GameState->Entities));
	entity* Entity = GameState->Entities + GameState->EntityCount++;
	Entity->P = P;
	Entity->Color = C;
	Entity->Type = Type;
	// NOTE(hugo): Do I need to store this ? Or could I compute it each render iteration ?
	Entity->TileSetP = TileSetPosFromType[Type];
	Entity->Flags = Flags;
}

void PushPlayer(game_state* GameState, v2 P, v4 C)
{
	PushEntity(GameState, P, C, EntityType_Player);
}

void PushWall(game_state* GameState, v2 P, v4 C)
{
	PushEntity(GameState, P, C, EntityType_Wall);
}

void PushGround(game_state* GameState, v2 P, v4 C, u32 Flags = EntityFlag_None)
{
	PushEntity(GameState, P, C, EntityType_Ground, Flags);
}

enum room_flag
{
	RoomFlag_None = 0,
	RoomFlag_LeftHole = 1 << 0,
	RoomFlag_RightHole = 1 << 1,
};

void CreateRoom(game_state* GameState, rectangle2i Rect, v4 WallC, v4 GroundC, u32 Flags = RoomFlag_None)
{
	for(i32 X = Rect.MinX; X <= Rect.MaxX; ++X)
	{
		PushWall(GameState, V2(X, Rect.MinY), WallC);
		PushWall(GameState, V2(X, Rect.MaxY), WallC);
		for(i32 Y = Rect.MinY + 1; Y < Rect.MaxY; ++Y)
		{
			if(X == Rect.MinX || X == Rect.MaxX)
			{
				b32 IsLeftHole = ((Flags & RoomFlag_LeftHole) != 0) && (X == Rect.MinX) && (Y == (Rect.MinY + Rect.MaxY)/2);
				b32 IsRightHole = ((Flags & RoomFlag_RightHole) != 0) && (X == Rect.MaxX) && (Y == (Rect.MinY + Rect.MaxY)/2);
				b32 IsHole = IsLeftHole || IsRightHole;

				if(IsHole)
				{
					PushGround(GameState, V2(X, Y), GroundC, EntityFlag_RoomConnection);
				}
				else
				{
					PushWall(GameState, V2(X, Y), WallC);
				}
			}
			else
			{
				PushGround(GameState, V2(X, Y), GroundC);
			}
		}
	}
}

gb_internal b32 MoveAllowed(game_state* GameState, v2 WantedP)
{
	for(u32 EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex)
	{
		entity* Entity = GameState->Entities + EntityIndex;
		if(Entity->Type == EntityType_Wall && WantedP.x == Entity->P.x && WantedP.y == Entity->P.y)
		{
			return(false);
		}
	}
	return(true);
}

gb_internal entity* GetUnderlyingEntity(game_state* GameState, v2 P)
{
	for(u32 EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex)
	{
		entity* Entity = GameState->Entities + EntityIndex;
		if(Entity->Type != EntityType_Player && P.x == Entity->P.x && P.y == Entity->P.y)
		{
			return(Entity);
		}
	}
	return(nullptr);
}

gb_global game_state GameState = {};

void GameUpdateAndRender(game_input* Input, renderer_state* Renderer)
{
    if(!GameState.Initialized)
    {
		v4 WallC = ColorPalette0[0];
		v4 GroundC = ColorPalette0[1];
		v4 PlayerC = ColorPalette0[2];
		CreateRoom(&GameState, RectMinMax(-5, -5, 5, 5), WallC, GroundC, RoomFlag_RightHole);
		CreateRoom(&GameState, RectMinMax(6, -5, 16, 5), GroundC, WallC, RoomFlag_LeftHole);
		PushPlayer(&GameState, V2(-2.0f, -1.0f), PlayerC);

        GameState.Initialized = true;
    }
    GameState.Timer += Input->dtForFrame;

	for(u32 EntityIndex = 0; EntityIndex < GameState.EntityCount; ++EntityIndex)
	{
		entity* Entity = GameState.Entities + EntityIndex;

		if(Entity->Type == EntityType_Player)
		{
			v2 WantedP = Entity->P;
			if(WasPressed(Input->KeyboardController.MoveLeft))
			{
				WantedP.x -= 1.0f;
			}
			if(WasPressed(Input->KeyboardController.MoveRight))
			{
				WantedP.x += 1.0f;
			}
			if(WasPressed(Input->KeyboardController.MoveUp))
			{
				WantedP.y += 1.0f;
			}
			if(WasPressed(Input->KeyboardController.MoveDown))
			{
				WantedP.y -= 1.0f;
			}

			if(MoveAllowed(&GameState, WantedP))
			{
				Entity->P = WantedP;

				entity* UnderlyingEntity = GetUnderlyingEntity(&GameState, WantedP);
				if(UnderlyingEntity)
				{
					if((UnderlyingEntity->Flags & EntityFlag_RoomConnection) != 0)
					{
						if(GameState.JustSteppedOnConnectionTimer <= -1.0f)
						{
							GameState.JustSteppedOnConnectionTimer = 5.0f;
						}
					}
					else
					{
						if(GameState.JustSteppedOnConnectionTimer <= 0.0f)
						{
							GameState.JustSteppedOnConnectionTimer = -1.0f;
						}
					}
				}
			}
		}
		PushTile(Renderer, Entity->P, Entity->TileSetP, Entity->Color);
	}

	if(GameState.JustSteppedOnConnectionTimer > 0.0f)
	{
		GameState.JustSteppedOnConnectionTimer -= Input->dtForFrame;
		f32 Alpha = Clamp01MapToRange(0.0f, GameState.JustSteppedOnConnectionTimer, 5.0f);
		v4 TextC = ColorPalette0[4];
		TextC.a = Alpha;
		PushTextWithShadow(Renderer, "New Room", V2(0.0f, 200.0f), TextC);
	}

	PushRect(Renderer, V2(0.0f, 0.0f), V2(100.0f, 100.0f), V4(1.0f, 0.0f, 0.0f, 0.5f));

}

