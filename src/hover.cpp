#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OLC_PGEX_MINIAUDIO
#include "olcPGEX_MiniAudio.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 M_PI/2	// 1.57079632679
#endif

class Game : public olc::PixelGameEngine
{
public:
	Game()
	{
		sAppName = "Hover";
	}

	// todo: this struct is wrongly named cargo, it is the content of the world map
	struct cargo {
		olc::vf2d pos;			// position in world space
		int cargoType;			// type of cargo: 
	};

	struct ClipRect {
		olc::vf2d tl;
		olc::vf2d br;
	};

	// *** Sounds ***
	olc::MiniAudio ma_engine;
	int sound_bgm_id = 0;
	int sound_pickup_id = 0;
	int sound_drop_id = 0;
	int sound_purge_id = 0;
	int sound_crash_id = 0;
	int sound_ship_id = 0;
	bool sound_altitude_alert_play = false;
	bool sound_crash_play = false;
	bool sound_drop_play = false;
	bool sound_pickup_play = false;
	bool sound_purge_play = false;
	bool sound_music_toggle = true;
	float engine_sound_speed = 1.0;

	olc::Sprite* spr_orb = nullptr;	olc::Decal* dec_orb = nullptr;
	olc::Sprite* spr_chest = nullptr;	olc::Decal* dec_chest = nullptr;
	olc::Sprite* spr_startpad = nullptr;	olc::Decal* dec_startpad = nullptr;
	olc::Sprite* spr_bg_tile = nullptr;	olc::Decal* dec_bg_tile = nullptr;
	olc::Sprite* spr_ship = nullptr;	olc::Decal* dec_ship = nullptr;
	olc::Sprite* spr_minimap = nullptr; olc::Decal* dec_minimap = nullptr;

	// *** Ship ***
	int ship_inventory_capasity = 3;
	std::vector<cargo> inventory;
	// for the warped decal 
	std::array<olc::vf2d, 4> points;
	olc::vf2d* pSelected = nullptr;
	// ship, 4 engines 
	//   1     4
	//      X
	//   2     3
	float boostScale = 1.0f;
	float throttle1 = { 0.0f };										// 0-100% [0.0 - 1.0]
	float throttle2 = { 0.0f };										// 0-100% [0.0 - 1.0]
	float throttle3 = { 0.0f };										// 0-100% [0.0 - 1.0]
	float throttle4 = { 0.0f };										// 0-100% [0.0 - 1.0]
	float ship_net_weight = 900.0f;
	float ship_weight = 900.0f;										// grams
	float ship_velocity_x = { 0.0f };
	float ship_velocity_y = { 0.0f };
	float ship_velocity_z = { 0.0f };
	olc::vf2d ship_cap_vel_xy = { 100.0f, 100.0f };
	float ship_cap_vel_z = 300.0f;
	float ship_velocity_to_player_scale = 15.0f;
	float ship_autolevel_toggle = false;
	float ship_autothrottle_toggle = true;
	float ship_response = 2.0f;										// respons factor on the movement
	float ship_avr_throttle = 0.0f;
	float ship_idle_throttle = 0.01f;
	float ship_max_thrust = 4000.0f;								// grams
	float ship_max_angle = 0.7853981634f; 							// M_PI_4 pi/4 = 45 deg
	bool ship_crashed = false;
	int ship_docket_at_cargoType = 0;
	bool ship_tilt_key_held = false;
	bool ship_throttle_key_held = false;
	float auto_alt_hold = 0;
	float ship_angle_x = 0.0;
	float ship_angle_y = 0.0;
	olc::vf2d ship_pos;
	olc::vi2d ship_on_screen_pos;
	int cargo_weight = 0;

	// *** Game ***
	// first char based game map, don't need it, can generate it
	std::string game_map;
	olc::vi2d charmap_dim = { 16, 20 };
	olc::vi2d instructions_pos = { 50, 50 };
	float game_critical_landing_velocity = -150.0f;
	float last_velocity_before_crashlanding = 0.0f;					// checking for pancake effect
	float velocity_alert_warning_threshhold = 0.6f;
	enum state { INTRO, GAMEON, THEEND, GAMEWON };
	enum state game_state = state::INTRO;
	float game_object_proximity_limit = 10.0f;
	float game_clip_objects_radius = 120.0f;
	ClipRect clip_rectangle = {{ -200.0f, -140.0f }, { 200.0f, 140.0f} };
	bool game_toggle_intro = false;
	float gravity = 0.2f;
	float altitude = 0.0f;									// 0m  sea level
	float max_altitude = 140.0f;
	olc::vf2d world_max = { 1000.0f,1000.0f }; // *16.0f / 9.0f };
	olc::vi2d minimap_size = { 100, 100 };
	olc::vi2d minimap_position;
	olc::vf2d startpos;			// doh
	olc::vf2d dropzone;  		// dropzone
	std::vector<cargo> cargos;	// yea you guessed it
	bool cargo_no_pickup = false;
	olc::vi2d inventory_pos = { 10, 50 };
	bool mouse_control_toggle = false;
	bool hud_toggle = true;
	olc::vf2d instrument_pos = { 500.0f, 200.0f };

	// need some temperary stuff
	std::string tmpstr;			
	std::stringstream ss;

	// timers
	bool timer_descent_vel_alert_active = false;
	bool timer_toggle_on_state = false;

	// do the blinking/alert stick shaker etc...
	bool isToggled = false;
	std::chrono::steady_clock::time_point lastToggleTime;


	// *** Interface ***
	olc::vi2d mouse_pos, mouse_pos_old;
	int mouse_wheel_old;
	int mouse_wheel;

	// *** Player ***
	int player_points = 0;
	int player_deliveries = 0;


public:

	// scan through the map and place the objects
	void InitGameMap() {
		float sx = world_max.x / charmap_dim.x;
		float sy = world_max.y / charmap_dim.y;

		// wipe inventory
		inventory.clear();
		cargos.clear();

		// scan the rows
		char ch;
		for (int y = 0; y < charmap_dim.y; y++) {
			for (int x = 0; x < charmap_dim.x; x++) {
				// draw background images here

				switch (ch = game_map[y * charmap_dim.x + x])
				{
				case '*':	// startingpos
					startpos = { float(x * sx),float(y * sy) };
					cargos.push_back({ startpos,ch });
					break;
				case 'd':	// dropzone
					dropzone = { float(x * sx),float(y * sy) };
					cargos.push_back({ dropzone,ch });
					break;
				case ' ':
					cargos.push_back({ {float(x) * sx,float(y) * sy},ch });
					break;
				default:
					if (isdigit(ch)) {

						cargos.push_back({ {float(x) * sx,float(y) * sy},ch });
					}
					break;
				}
			}
		}
	}

	void DrawCargoPos(olc::vi2d txtpos, olc::vf2d p) {
		tmpstr = "CargoPos ";
		DrawString(txtpos, tmpstr);
		ss.str("");	ss << p.x; tmpstr = ss.str(); DrawString({ txtpos.x + 100, txtpos.y }, tmpstr, olc::YELLOW);
		ss.str("");	ss << p.y; tmpstr = ss.str(); DrawString({ txtpos.x + 160, txtpos.y }, tmpstr, olc::YELLOW);
	}

	bool OnUserCreate() override
	{
		// Called once at the start, so create things here

		boostScale = 1.0f;		// seecret boost function

		// *: starting position
		// d: drop pad
		// 0-9: cargo type,
		//	  0: no weight
		//  1-9: the higher the number the more it weighth 
		game_map = "9              9";
		game_map += "     0   0      ";
		game_map += "                ";
		game_map += "     0   0      ";
		game_map += "  5         3   " ;
		game_map += "                ";
		game_map += "  5         3   ";
		game_map += "       2        " ;
		game_map += "                ";
		game_map += "   d  2*2  1    ";
		game_map += "                ";
		game_map += "       2        ";
		game_map += "                ";
		game_map += "                ";
		game_map += "    0    6      ";
		game_map += "                ";
		game_map += "     2      8   ";
		game_map += "                ";
		game_map += "                ";
		game_map += "9      3       9";

		ship_on_screen_pos = { ScreenWidth() / 2,ScreenHeight() / 2 };
		minimap_position = { ScreenWidth() - minimap_size.x - 2, 10 };

		InitGameMap();
		ship_pos = startpos;

		sound_bgm_id = ma_engine.LoadSound("./res/wav/bgm.wav");
		ma_engine.SetVolume(sound_bgm_id, 0.5f);
		ma_engine.SetBackgroundPlay(true);

		sound_pickup_id = ma_engine.LoadSound("./res/wav/pickup.wav");
		sound_drop_id = ma_engine.LoadSound("./res/wav/drop.wav");
		sound_crash_id = ma_engine.LoadSound("./res/wav/crash.wav");
		sound_purge_id = ma_engine.LoadSound("./res/wav/purge.wav");
		sound_ship_id = ma_engine.LoadSound("./res/wav/engine.wav");
		ma_engine.SetVolume(sound_ship_id, 0.2f);
		ma_engine.Toggle(sound_ship_id, true);

		spr_orb = new olc::Sprite("./res/img/orb.png");	dec_orb = new olc::Decal(spr_orb);
		spr_chest = new olc::Sprite("./res/img/chest.png");	dec_chest = new olc::Decal(spr_chest);
		spr_startpad = new olc::Sprite("./res/img/startpad.png");	dec_startpad = new olc::Decal(spr_startpad);
		spr_bg_tile = new olc::Sprite("./res/img/bg_tile.png");	dec_bg_tile = new olc::Decal(spr_bg_tile);
		spr_ship = new olc::Sprite("./res/img/ship.png");	dec_ship = new olc::Decal(spr_ship);
		spr_minimap = new olc::Sprite(minimap_size.x, minimap_size.y); dec_minimap = new olc::Decal(spr_minimap);

		return true;
	}

	void TimerUpdateTrigger(float fElapsedTime, float time_until_trigger) {
		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lastToggleTime).count() >= time_until_trigger) {
			timer_toggle_on_state = !timer_toggle_on_state;
			lastToggleTime = std::chrono::steady_clock::now();
		}
	}


	bool OnUserUpdate(float fElapsedTime) override
	{
		// called once per frame
		mouse_pos_old = mouse_pos;
		mouse_pos = GetMousePos();

		// olc::vi2d yokeStrength;
		// yokeStrength = mouse_pos - ship_pos;

		TimerUpdateTrigger(fElapsedTime, 200);

		Clear(olc::VERY_DARK_BLUE);

		// Draw mouse cursor
		//DrawLineDecal(mouse_pos - olc::vi2d{ 5,5 }, mouse_pos + olc::vi2d{ 5,5 }, olc::RED);
		//DrawLineDecal(mouse_pos - olc::vi2d{ -5,5 }, mouse_pos + olc::vi2d{ -5,5 }, olc::RED);
		//ss.str(""); ss << "(" << mouse_pos.x << "," << mouse_pos.y << ")";
		// DrawString(mouse_pos, ss.str(), olc::RED);



		// Main game loop
		if (game_state == state::GAMEON) {
			// calculate angle from thrust differentials (simple version) max angle = 45 deg
			// todo: wrong way, must improve...
			//       Should be able to have max throttle at any angle without the ship rights itself
			//		 and reverts to average throttle...
			ship_angle_x = float(M_PI_2) * (((throttle1 + throttle2) - (throttle3 + throttle4)) / 2);
			ship_angle_y = float(M_PI_2) * (((throttle1 + throttle4) - (throttle2 + throttle3)) / 2);

			// boost throttle if shift is held,  hush now
			if (GetKey(olc::Key::SHIFT).bHeld)
				boostScale = 4.0f;
			else 
				boostScale = 1.0f;

			// Add mouse support
			mouse_wheel_old = mouse_wheel;
			mouse_wheel = GetMouseWheel();
			int mouse_dead_zone;
			mouse_dead_zone = 10;

			olc::vf2d mouse_ship_response;
			mouse_ship_response.x = float(abs(mouse_pos.x - ship_on_screen_pos.x)); 
			mouse_ship_response.y = float(abs(mouse_pos.y - ship_on_screen_pos.y)); 

			if (ship_autolevel_toggle) {
				mouse_ship_response.x *= 0.05f;
				mouse_ship_response.y *= 0.05f;
			}
			else if (!ship_autolevel_toggle) {
				mouse_ship_response.x *= 0.01f;
				mouse_ship_response.y *= 0.01f;
			}

			if (mouse_control_toggle) {
				// move right 
				if (mouse_pos.x >= (ship_on_screen_pos.x + mouse_dead_zone)) {
					throttle1 += fElapsedTime * mouse_ship_response.x;
					throttle2 += fElapsedTime * mouse_ship_response.x;
					throttle3 -= fElapsedTime * mouse_ship_response.x;
					throttle4 -= fElapsedTime * mouse_ship_response.x;
					if (throttle1 < ship_idle_throttle) throttle1 = ship_idle_throttle;	if (throttle1 > 1.0) throttle1 = 1.0;
					if (throttle2 < ship_idle_throttle) throttle2 = ship_idle_throttle;	if (throttle2 > 1.0) throttle2 = 1.0;
					if (throttle3 < ship_idle_throttle) throttle3 = ship_idle_throttle;	if (throttle3 > 1.0) throttle3 = 1.0;
					if (throttle4 < ship_idle_throttle) throttle4 = ship_idle_throttle;	if (throttle4 > 1.0) throttle4 = 1.0;
				}
				// move left 
				if (mouse_pos.x < (ship_on_screen_pos.x - mouse_dead_zone)) {
					throttle1 -= fElapsedTime * mouse_ship_response.x;
					throttle2 -= fElapsedTime * mouse_ship_response.x;
					throttle3 += fElapsedTime * mouse_ship_response.x;
					throttle4 += fElapsedTime * mouse_ship_response.x;
					if (throttle1 < ship_idle_throttle) throttle1 = ship_idle_throttle;	if (throttle1 > 1.0) throttle1 = 1.0;
					if (throttle2 < ship_idle_throttle) throttle2 = ship_idle_throttle;	if (throttle2 > 1.0) throttle2 = 1.0;
					if (throttle3 < ship_idle_throttle) throttle3 = ship_idle_throttle;	if (throttle3 > 1.0) throttle3 = 1.0;
					if (throttle4 < ship_idle_throttle) throttle4 = ship_idle_throttle;	if (throttle4 > 1.0) throttle4 = 1.0;
				}

				// move down
				if (mouse_pos.y >= (ship_on_screen_pos.y + mouse_dead_zone)) {
					throttle1 += fElapsedTime * mouse_ship_response.y;
					throttle2 -= fElapsedTime * mouse_ship_response.y;
					throttle3 -= fElapsedTime * mouse_ship_response.y;
					throttle4 += fElapsedTime * mouse_ship_response.y;
					if (throttle1 < ship_idle_throttle) throttle1 = ship_idle_throttle; if (throttle1 > 1.0) throttle1 = 1.0;
					if (throttle2 < ship_idle_throttle) throttle2 = ship_idle_throttle; if (throttle2 > 1.0) throttle2 = 1.0;
					if (throttle3 < ship_idle_throttle) throttle3 = ship_idle_throttle; if (throttle3 > 1.0) throttle3 = 1.0;
					if (throttle4 < ship_idle_throttle) throttle4 = ship_idle_throttle; if (throttle4 > 1.0) throttle4 = 1.0;
				}

				// move down
				if (mouse_pos.y < (ship_on_screen_pos.y - mouse_dead_zone)) {
					throttle1 -= fElapsedTime * mouse_ship_response.y;
					throttle2 += fElapsedTime * mouse_ship_response.y;
					throttle3 += fElapsedTime * mouse_ship_response.y;
					throttle4 -= fElapsedTime * mouse_ship_response.y;
					if (throttle1 < ship_idle_throttle) throttle1 = ship_idle_throttle;	if (throttle1 > 1.0) throttle1 = 1.0;
					if (throttle2 < ship_idle_throttle) throttle2 = ship_idle_throttle;	if (throttle2 > 1.0) throttle2 = 1.0;
					if (throttle3 < ship_idle_throttle) throttle3 = ship_idle_throttle;	if (throttle3 > 1.0) throttle3 = 1.0;
					if (throttle4 < ship_idle_throttle) throttle4 = ship_idle_throttle;	if (throttle4 > 1.0) throttle4 = 1.0;
				}
			}


			ship_throttle_key_held = false;
			// power to all engines
			if (GetKey(olc::Key::UP).bHeld || ((mouse_wheel > 0) && mouse_control_toggle) ) {
				float mouse_response = 1.0f;
				if (mouse_wheel > 0)
					mouse_response = 2.0f;

				if (altitude < max_altitude) {
					throttle1 += fElapsedTime * ship_response * mouse_response * boostScale; if (throttle1 > 1.0f * boostScale) throttle1 = 1.0f * boostScale;
					throttle2 += fElapsedTime * ship_response * mouse_response * boostScale; if (throttle2 > 1.0f * boostScale) throttle2 = 1.0f * boostScale;
					throttle3 += fElapsedTime * ship_response * mouse_response * boostScale; if (throttle3 > 1.0f * boostScale) throttle3 = 1.0f * boostScale;
					throttle4 += fElapsedTime * ship_response * mouse_response * boostScale; if (throttle4 > 1.0f * boostScale) throttle4 = 1.0f * boostScale;
					ship_throttle_key_held = true;
					auto_alt_hold = altitude;
				}
			}

			if (GetKey(olc::Key::DOWN).bHeld || ((mouse_wheel < 0) && mouse_control_toggle)) {
				throttle1 -= fElapsedTime * ship_response; if (throttle1 < ship_idle_throttle) throttle1 = ship_idle_throttle;
				throttle2 -= fElapsedTime * ship_response; if (throttle2 < ship_idle_throttle) throttle2 = ship_idle_throttle;
				throttle3 -= fElapsedTime * ship_response; if (throttle3 < ship_idle_throttle) throttle3 = ship_idle_throttle;
				throttle4 -= fElapsedTime * ship_response; if (throttle4 < ship_idle_throttle) throttle4 = ship_idle_throttle;
				ship_throttle_key_held = true;
				auto_alt_hold = altitude;
			}

			ship_tilt_key_held = false;
			// roll left
			// Increse 3 and 4
			// decrese 1 and 2
			// int mouse_deadzone = 50;
			if (GetKey(olc::Key::A).bHeld) {
				throttle1 -= fElapsedTime * ship_response;
				throttle2 -= fElapsedTime * ship_response;
				throttle3 += fElapsedTime * ship_response;
				throttle4 += fElapsedTime * ship_response;
				if (throttle1 < ship_idle_throttle) throttle1 = ship_idle_throttle;	if (throttle1 > 1.0) throttle1 = 1.0;
				if (throttle2 < ship_idle_throttle) throttle2 = ship_idle_throttle;	if (throttle2 > 1.0) throttle2 = 1.0;
				if (throttle3 < ship_idle_throttle) throttle3 = ship_idle_throttle;	if (throttle3 > 1.0) throttle3 = 1.0;
				if (throttle4 < ship_idle_throttle) throttle4 = ship_idle_throttle;	if (throttle4 > 1.0) throttle4 = 1.0;
				ship_tilt_key_held = true;
			}
			// roll right
			// Increse 1 and 2
			// decrese 3 and 4
			if (GetKey(olc::Key::D).bHeld) {
				throttle1 += fElapsedTime * ship_response;
				throttle2 += fElapsedTime * ship_response;
				throttle3 -= fElapsedTime * ship_response;
				throttle4 -= fElapsedTime * ship_response;
				if (throttle1 < ship_idle_throttle) throttle1 = ship_idle_throttle;	if (throttle1 > 1.0) throttle1 = 1.0;
				if (throttle2 < ship_idle_throttle) throttle2 = ship_idle_throttle;	if (throttle2 > 1.0) throttle2 = 1.0;
				if (throttle3 < ship_idle_throttle) throttle3 = ship_idle_throttle;	if (throttle3 > 1.0) throttle3 = 1.0;
				if (throttle4 < ship_idle_throttle) throttle4 = ship_idle_throttle;	if (throttle4 > 1.0) throttle4 = 1.0;
				ship_tilt_key_held = true;
			}
			// pitch forward
			// Increse 2 and 3
			// decrese 1 and 4
			if (GetKey(olc::Key::W).bHeld) {
				throttle1 -= fElapsedTime * ship_response;
				throttle2 += fElapsedTime * ship_response;
				throttle3 += fElapsedTime * ship_response;
				throttle4 -= fElapsedTime * ship_response;
				if (throttle1 < ship_idle_throttle) throttle1 = ship_idle_throttle;	if (throttle1 > 1.0) throttle1 = 1.0;
				if (throttle2 < ship_idle_throttle) throttle2 = ship_idle_throttle;	if (throttle2 > 1.0) throttle2 = 1.0;
				if (throttle3 < ship_idle_throttle) throttle3 = ship_idle_throttle;	if (throttle3 > 1.0) throttle3 = 1.0;
				if (throttle4 < ship_idle_throttle) throttle4 = ship_idle_throttle;	if (throttle4 > 1.0) throttle4 = 1.0;
				ship_tilt_key_held = true;
			}
			// pitch back
			// Increse 1 and 4
			// decrese 2 and 3
			if (GetKey(olc::Key::S).bHeld) {
				throttle1 += fElapsedTime * ship_response;
				throttle2 -= fElapsedTime * ship_response;
				throttle3 -= fElapsedTime * ship_response;
				throttle4 += fElapsedTime * ship_response;
				if (throttle1 < ship_idle_throttle) throttle1 = ship_idle_throttle; if (throttle1 > 1.0) throttle1 = 1.0;
				if (throttle2 < ship_idle_throttle) throttle2 = ship_idle_throttle; if (throttle2 > 1.0) throttle2 = 1.0;
				if (throttle3 < ship_idle_throttle) throttle3 = ship_idle_throttle; if (throttle3 > 1.0) throttle3 = 1.0;
				if (throttle4 < ship_idle_throttle) throttle4 = ship_idle_throttle; if (throttle4 > 1.0) throttle4 = 1.0;
				ship_tilt_key_held = true;
			}


			if (!game_toggle_intro) {
				// calculate throttle average from engines
				ship_avr_throttle = (throttle1 + throttle2 + throttle3 + throttle4) / 4;

				// reset the throttle to neutral position
				if (GetKey(olc::Key::SPACE).bPressed) {
					ship_autolevel_toggle = !ship_autolevel_toggle;
				}

				if (ship_autolevel_toggle)
					DrawString({ ScreenWidth() / 2 - 12 * 8 / 2, 25 }, "Autoleveling ON", olc::GREEN);

				// reset the throttle to neutral position
				if (GetKey(olc::Key::T).bPressed) {
					ship_autothrottle_toggle = !ship_autothrottle_toggle;
				}


				// autolevel the ship if not any input from player
				if (ship_autolevel_toggle && !ship_tilt_key_held) {
					float autolevel_speed_scale = 2.0f;

					if (throttle1 < ship_avr_throttle)
						throttle1 += fElapsedTime * autolevel_speed_scale;
					else if (throttle1 > ship_avr_throttle)
						throttle1 -= fElapsedTime * autolevel_speed_scale;

					if (throttle2 < ship_avr_throttle)
						throttle2 += fElapsedTime * autolevel_speed_scale;
					else if (throttle2 > ship_avr_throttle)
						throttle2 -= fElapsedTime * autolevel_speed_scale;

					if (throttle3 < ship_avr_throttle)
						throttle3 += fElapsedTime * autolevel_speed_scale;
					else if (throttle3 > ship_avr_throttle)
						throttle3 -= fElapsedTime * autolevel_speed_scale;

					if (throttle4 < ship_avr_throttle)
						throttle4 += fElapsedTime * autolevel_speed_scale;
					else if (throttle4 > ship_avr_throttle)
						throttle4 -= fElapsedTime * autolevel_speed_scale;
				}


				float gameSpeed = 20.0f;
				// to hight, auto throtteling down
				if (altitude >= max_altitude)
					ship_velocity_z = 0.0;
				else {
					// thrust in z axis
					float angle_dec_thrust = cos(ship_angle_x/1.5f) * cos(ship_angle_y/1.5f); // invert it?
					ship_velocity_z += fElapsedTime * gameSpeed * angle_dec_thrust * ship_avr_throttle;
				}

				ship_velocity_z -= fElapsedTime * gameSpeed * gravity;
				ship_velocity_z -= fElapsedTime * ship_weight * 0.005f; // normalize weight
				ship_velocity_x += fElapsedTime * gameSpeed * sin(ship_angle_x);
				ship_velocity_y += fElapsedTime * gameSpeed * sin(ship_angle_y);

				if (game_state == state::GAMEON)
					last_velocity_before_crashlanding = ship_velocity_z; // *velocity_scale;    // for checking if you crashed hard into ground

				altitude += fElapsedTime * gameSpeed * ship_velocity_z;

				// cap ship velocity in xy
				if ((ship_velocity_x ) >  ship_cap_vel_xy.x) 	ship_velocity_x =   ship_cap_vel_xy.x ;
				if ((ship_velocity_x ) < -ship_cap_vel_xy.x)	ship_velocity_x = -(ship_cap_vel_xy.x );
				if ((ship_velocity_y ) >  ship_cap_vel_xy.y) 	ship_velocity_y =   ship_cap_vel_xy.y ;
				if ((ship_velocity_y ) < -ship_cap_vel_xy.y)	ship_velocity_y = -(ship_cap_vel_xy.y );

				ship_pos.x += fElapsedTime * gameSpeed / 4 * ship_velocity_x;
				ship_pos.y += fElapsedTime * gameSpeed / 4 * ship_velocity_y;

				// limit the ship inside the map area , bounch back
				if (ship_pos.x <        0.0f) { ship_pos.x =        0.0f; ship_velocity_x *= -1.0f; }
				if (ship_pos.x > world_max.x) { ship_pos.x = world_max.x; ship_velocity_x *= -1.0f; }
				if (ship_pos.y <        0.0f) { ship_pos.y =        0.0f; ship_velocity_y *= -1.0f; }
				if (ship_pos.y > world_max.y) { ship_pos.y = world_max.y; ship_velocity_y *= -1.0f; }

				if (altitude < 0.0f) {
					altitude = 0.0f;
					ship_velocity_x = 0.0f; // x
					ship_velocity_y = 0.0f; // y
					ship_velocity_z = 0.0f; // z
				}

				// engine speed sound reletive to the avg throttle
				engine_sound_speed = 1.0f + 2.0f * ship_avr_throttle;
				ma_engine.SetPitch(sound_ship_id, engine_sound_speed);

			} // physics update toggled off if game_toggle_intro

			// toggle the intro/instruction page
			if (GetKey(olc::Key::F1).bPressed) {
				game_toggle_intro = !game_toggle_intro;
			}




			if (!cargo_no_pickup) {
				ship_docket_at_cargoType = CheckDropPickupOnLanding();

				if (ship_docket_at_cargoType != 0) {
					switch (ship_docket_at_cargoType) {
					case 'd':
						tmpstr = "DROPZONE";
						break;
					case '*':
						tmpstr = "STARTPAD";
						break;
					default:
						tmpstr = "";
						break;
					}

					// show on screen
					DrawString({ ScreenWidth() / 2 - 40, 10 }, "Docked on", olc::GREEN);
					DrawString({ ScreenWidth() / 2 + 40, 10 }, tmpstr, olc::YELLOW);
				}
			}

			// no more cargo to pick up, if you deliver this you win -------
			int items_in_cargos = int(std::count_if(cargos.begin(), cargos.end(),
				[](cargo c) { return isdigit(c.cargoType); }));

//			if (cargos.size() == 2) {  // only startpad and dropzone left
			if (items_in_cargos == 0) {  // only startpad and dropzone left
				if (inventory.size() == 0)   // and inventory delivered
					game_state = state::GAMEWON;  // goal!!!
			}

			//// Show inventory and calculate weight
			//cargo_weight = ShowInventory(inventory_pos);
			//ship_weight = ship_net_weight + cargo_weight;

			//tmpstr = "Cargo Weight";
			//DrawString({ 100,10 }, tmpstr, olc::GREEN);
			//ss.str(""); ss << std::setw(4) << cargo_weight;
			//DrawString({ 100+8*8, 18 }, ss.str(), olc::RED);

			//tmpstr = "Score";
			//DrawString({ 10,10 }, tmpstr, olc::GREEN);
			//ss.str(""); ss << std::setw(5) << int(player_points);
			//DrawString({ 10,18 }, ss.str(), olc::RED);

			// Dont draw anything while showing the intro screen
			if (!game_toggle_intro) {
				DrawGameMapOnScreen(ship_pos);
				DrawMinimap(minimap_position, ship_pos);
				DrawShip();
				DrawMouseCursor(mouse_control_toggle);
				DrawHUD(hud_toggle);
			}
			else
				Instructions(instructions_pos + olc::vi2d{ 30,50 });

			// show alert if decending dangerously fast
			timer_descent_vel_alert_active = false;
			if (int(altitude) != 0 && ship_velocity_z /* * velocity_scale */ < (game_critical_landing_velocity + 40.0f) ) {
				timer_descent_vel_alert_active = true;
			}

			// check z velocity on "landing"
			if (int(altitude) == 0 && last_velocity_before_crashlanding*15.0f < game_critical_landing_velocity) {
				game_state = state::THEEND;
				ship_crashed = true;
				sound_crash_play = true;
			}

			// debug: Don't need this right?  
			if (ship_crashed)
				game_state = state::THEEND;

			// play the crash sound
			if (sound_crash_play) {
				sound_crash_play = false;
				ma_engine.Toggle(sound_crash_id, true);
				ma_engine.Toggle(sound_ship_id, true);	// silence the ships engines
			}

			// play the altitude alert sound
			if (sound_altitude_alert_play) {
				sound_altitude_alert_play = false;
				// this triggers every 500ms,  it frikes out the wave_engine...  so silence it is for now
				// miniaudio could cope tho...
			}

			// play the cargo pickup sound
			if (sound_pickup_play) {
				sound_pickup_play = false;
				ma_engine.Toggle(sound_pickup_id, true);
			}

			// play the drop sound
			if (sound_drop_play) {
				sound_drop_play = false;
				ma_engine.Toggle(sound_drop_id, true);

			}

			// play the purge sound
			if (sound_purge_play) {
				sound_purge_play = false;
				ma_engine.Toggle(sound_purge_id, true);

			}

			// Purge the last item from inventory
			if (GetKey(olc::Key::P).bPressed) {
				if (inventory.size() > 0) {
					inventory.erase(inventory.end() - 1);
					sound_purge_play = true;
				}
			}

			// must start the sounds again if they are finished playing
			if (!ma_engine.IsPlaying(sound_ship_id)) 
				ma_engine.Toggle(sound_ship_id, true);

			// only loop the music if the music is turned on 
			if ( sound_music_toggle)
				if (!ma_engine.IsPlaying(sound_bgm_id))
					ma_engine.Toggle(sound_bgm_id, true);



		} // endif: state_GAMEON ---


		// shift commands
		if (GetKey(olc::Key::SHIFT).bHeld) {
			// toggle mouse control
			if (GetKey(olc::Key::M).bPressed) {
				mouse_control_toggle = !mouse_control_toggle;
			}
		}
		else {
			// Toggle background music
			if (GetKey(olc::Key::M).bPressed) {
				sound_music_toggle = !sound_music_toggle;
				ma_engine.Toggle(sound_bgm_id);
			}
		}


		// Game is Won state
		if (game_state == state::GAMEWON) {
			GameWon();

			if (GetKey(olc::Key::ENTER).bReleased || GetKey(olc::Key::SPACE).bReleased) {
				RestartGame();
			}
		} // state::GAMEWON

		// Intro state, Set up a new game
		if (game_state == state::INTRO) {
			player_deliveries = 0;
			ship_crashed = false;
			last_velocity_before_crashlanding = 0.0f;
			player_points = 0;
			ship_velocity_x = 0.0f;
			ship_velocity_y = 0.0f;
			ship_velocity_z = 0.0f;
			throttle1 = 0.0f;
			throttle2 = 0.0f;
			throttle3 = 0.0f;
			throttle4 = 0.0f;

			Instructions(instructions_pos);
			if (GetKey(olc::Key::ENTER).bPressed || GetKey(olc::Key::SPACE).bPressed) {
				game_state = state::GAMEON;
				ma_engine.Toggle(sound_ship_id, true);
				if ( sound_music_toggle)
					ma_engine.Toggle(sound_bgm_id, true);
			}
		} // state::INTRO

		// Game ended, or user aborted
		if (game_state == state::THEEND) {
			EndGame();

			// SPACE: continue if not crashed, restarts if crashed
			if (GetKey(olc::Key::SPACE).bReleased) {
				if (!ship_crashed) {
					game_state = state::GAMEON;
					// ma_engine.Toggle(sound_engine_id, false);
					ma_engine.Pause(sound_ship_id);
					// wave_engine.SetOutputVolume(0.8f);
				}
				else
					RestartGame();
			}

			// ENTER: restart
			if (GetKey(olc::Key::ENTER).bReleased) {
				RestartGame();
			}

		}  // state::THEEND

		// Escape to THEEND, or quit if pressed while in THEEND state
		if (GetKey(olc::Key::ESCAPE).bPressed || GetKey(olc::Key::BACK).bPressed) {
			if (game_state == state::THEEND) {
				return false;
			}
			else {
				game_state = state::THEEND;
			}
		}

		// If not playing, kill the volume
		if (game_state == state::INTRO) {
			// wave_engine.SetOutputVolume(0.0f);
			ma_engine.Pause(sound_ship_id);
		}

		return true;
	} // end Update ---


	void DrawHUD(bool show) {
		if (show) {
			DrawAltitude(instrument_pos);
			DrawZVelocity(instrument_pos + olc::vf2d{30.0f, 0.0f}, ship_velocity_z); // *velocity_scale);
			DrawThrottle(instrument_pos + olc::vf2d{60.0f, 0.0f});

			// Show inventory and calculate weight
			cargo_weight = ShowInventory(inventory_pos);
			ship_weight = ship_net_weight + cargo_weight;

			tmpstr = "Cargo Weight";
			DrawString({ 100,10 }, tmpstr, olc::GREEN);
			ss.str(""); ss << std::setw(4) << cargo_weight;
			DrawString({ 100 + 8 * 8, 18 }, ss.str(), olc::RED);

			tmpstr = "Score";
			DrawString({ 10,10 }, tmpstr, olc::GREEN);
			ss.str(""); ss << std::setw(5) << int(player_points);
			DrawString({ 10,18 }, ss.str(), olc::RED);

		}
	}

	void DrawMouseCursor(bool show) {
		// show mouse cursor
		if (show) {
			DrawLineDecal(mouse_pos - olc::vi2d{ 5,5 }, mouse_pos + olc::vi2d{ 5,5 }, olc::WHITE);
			DrawLineDecal(mouse_pos - olc::vi2d{ -5,5 }, mouse_pos + olc::vi2d{ -5,5 }, olc::WHITE);
		}
	}

	void RestartGame() {
		game_state = state::INTRO;
		InitGameMap();
		ship_pos = startpos;
	}

	void Instructions(olc::vi2d pos) {
		int offsy = 10; int yc = 0;
		FillRect({ pos.x - 1,  pos.y - 1 }, { 500 + 2, 240 + 2 }, olc::VERY_DARK_GREY);
		DrawRect({ pos.x, pos.y }, { 500, 240 }, olc::GREEN);

		ss << std::fixed << std::setprecision(2);
		DrawString({ ScreenWidth() / 2 - 10 * 16 / 2,24 }, "Hover run", olc::YELLOW, 2);

		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                Made for the olcCodeJam 2024               ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                                                           ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "  The game is played by watching the altitude carefully.   ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "   You are running missions gathering cargo by landing     ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, " softly on top of them,you can pick up as many as you like.", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "    After pickup you fly and land on the dropzone(chest)   ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, " Be aware that when you roll and pitch you loose altitude  ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "     and you need to increase throttle to stay airborn.    ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "    If you land with a speed over -130, ship wreck! :)     ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "       Simple mouse control, mouse wheel is throttle       ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "  If you are using the keys then center the cursor on ship ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                                                           ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "         A,D,W,S Roll/Pitch     UP,DOWN Throttle           ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "         SPACE Autoleveling     P Purge from inventory     ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "         F1 This page           M Music                    ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                                                           ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                         Credits                           ", olc::DARK_YELLOW);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "             Javidx9 for his olcPixelGameEngine,           ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "          Moros1138 for his olcPGEX_MiniAudio wrapper,     ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                 David Reid for his miniaudio              ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                                                           ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                 Author: DragoneEye (discord)              ", olc::DARK_GREEN);
	}


	void EndGame() {
		int offsy = 10;
		int offsx = ScreenWidth() / 4;
		int asdf = ScreenHeight() / 4;
		FillRect({ offsx - 1, asdf - 1 }, { 300 + 2, 220 + 2 }, olc::VERY_DARK_GREY);
		DrawRect({ offsx, asdf }, { 300, 220 }, olc::RED);

		if (ship_crashed) {
			DrawString({ offsx + 10, asdf + offsy * 1 }, "Oh holy pancake...", olc::GREY);
			DrawString({ offsx + 10, asdf + offsy * 5 }, "What a spectacular crash!", olc::GREY);
			DrawString({ offsx + 10, asdf + offsy * 6 }, "Groundbreaking velocity:", olc::GREY);
			ss.str(""); ss << last_velocity_before_crashlanding*15.0f;
			DrawString({ offsx + 10 + 28 * 8, asdf + offsy * 6 }, ss.str(), olc::RED);
			DrawString({ offsx + 10, asdf + offsy * 18 }, "SPACE/ENTER to restart,ESC/BACK quit", olc::RED);
		}
		else {
			DrawString({ offsx + 10, asdf + offsy * 1 }, "        User aborted!", olc::GREY);
			DrawString({ offsx + 10, asdf + offsy * 7 }, "I am sorry to see you go...", olc::GREY);
			DrawString({ offsx + 10, asdf + offsy * 8 }, "  Hope you had fun! L8r o7 ", olc::GREY);
			DrawString({ offsx + 10, asdf + offsy * 16 }, " SPACE    (Continue)", olc::GREEN);
			DrawString({ offsx + 10, asdf + offsy * 17 }, " ENTER    (Restart)", olc::YELLOW);
			DrawString({ offsx + 10, asdf + offsy * 18 }, " ESC/BACK (quit)", olc::RED);
		}
		DrawString({ offsx + 10, asdf + offsy * 3 }, "Score: ", olc::GREY);
		ss.str(""); ss << std::setw(5) << player_points;
		DrawString({ offsx + 10 + 7 * 8, asdf + offsy * 3 }, ss.str(), olc::GREEN);

		if (ship_crashed) {
			DrawLine({ offsx + 10 + 7 * 8, asdf + offsy * 3 + 3 }, { offsx + 10 + 7 * 8 + 5 * 8, asdf + offsy * 3 + 3 }, olc::RED);
		}

		DrawString({ offsx + 10 + 20 * 8, asdf + offsy * 3 }, "Runs: ", olc::GREY);
		ss.str(""); ss << std::setw(0) << player_deliveries;
		DrawString({ offsx + 10 + 27 * 8, asdf + offsy * 3 }, ss.str(), olc::GREEN);
	}

	void GameWon() {
		int offsy = 10;
		int offsx = ScreenWidth() / 4;
		int asdf = ScreenHeight() / 4;
		FillRect({ offsx - 1, asdf - 1 }, { 300 + 2, 200 + 2 }, olc::VERY_DARK_GREY);
		DrawRect({ offsx, asdf }, { 300, 200 }, olc::GREEN);

		DrawString({ offsx + 10, asdf + offsy * 1 }, "       You runned the game !!! ", olc::GREEN);
		DrawString({ offsx + 10, asdf + offsy * 3 }, "    kewl, you are truly a master ! ", olc::GREEN);

		DrawString({ offsx + 10, asdf + offsy * 7 }, " Your score is ", olc::GREEN);
		ss.str(""); ss << std::setw(5) << player_points;
		DrawString({ offsx + 17 * 8, asdf + offsy * 7 }, ss.str(), olc::RED);
		DrawString({ offsx + 10, asdf + offsy * 8 }, " and you did ", olc::GREEN);
		ss.str(""); ss << std::setw(5) << player_deliveries;
		DrawString({ offsx + 17 * 8, asdf + offsy * 8 }, ss.str(), olc::RED);
		DrawString({ offsx + 22 * 8, asdf + offsy * 8 }, " runs", olc::GREEN);

		DrawString({ offsx + 10, asdf + offsy * 14 }, " Press Space or Enter to try again.", olc::GREEN);
		DrawString({ offsx + 10, asdf + offsy * 15 }, "           Esc/Back to quit", olc::RED);
	}


	int ShowInventory(olc::vi2d pos) {
		int offs = 10;
		int j;
		int inv_weight = 0;
		int cargo_weight = 0;
		if (inventory.size() > 0) {
			for (j = 0; j < inventory.size(); ++j) {
				ss.str(""); ss << static_cast<char>(inventory[j].cargoType);
				DrawString({ 13, 62 + j * offs }, ss.str(), olc::GREEN);
				inv_weight += (inventory[j].cargoType - 48) * 100;
			}
			DrawRectDecal({ float(pos.x), float(pos.y + 8) }, { 80.0f, float(offs * j + 3 ) });
		}
		else {
			DrawStringDecal({ float(pos.x + 10 + 8 * 8), float(pos.y) }, "Empty", olc::RED);
		}

		DrawStringDecal({ float(pos.x), float(pos.y) }, "Inventory", olc::GREEN);

		return inv_weight;	// return the weight of inventory
	}

	void CountTheChicken(int cargoType) {
		switch (cargoType)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			player_points += cargoType * 23;
			player_deliveries++;
			break;

		default:
			break;
		}
	}

	int CheckDropPickupOnLanding() {
		int cargoType = 0;
		if (int(altitude) == 0) {
			// Anything here?
			for (int i = 0; i < cargos.size(); ++i) {
				if (fabs(ship_pos.x - cargos[i].pos.x) < game_object_proximity_limit) {
					if (fabs(ship_pos.y - cargos[i].pos.y) < game_object_proximity_limit) {
						// yay, something here.
						cargoType = cargos[i].cargoType;   // return docked at cargoType 
						// is it some cargo? then move it into inventory
						if (isdigit(cargos[i].cargoType)) {
							inventory.push_back(cargos[i]);
							sound_pickup_play = true;
							// cargos.erase(cargos.begin() + i);  // off the map with it
							cargos[i].cargoType = ' ';	// do not erase, but change it to gb_tile
						}
						else {
							// if it is the drop point, yay happy you, unload and be happy
							if (cargos[i].cargoType == 'd') {
								// drop it all
								if (inventory.size() > 0) {
									for (int j = 0; j < inventory.size(); ++j) {
										CountTheChicken(inventory[j].cargoType);  // offload 
									}
									inventory.clear();
									sound_drop_play = true;
								}
							}
						}
					}
				}
			}
		}
		return cargoType;
	}


	void DrawGameMapOnScreen(olc::vf2d ship_pos) {
		float cx;
		float cy;
		olc::Pixel col = olc::GREEN;

		// how many tiles goes in each axis
		float bg_tiles_in_x = world_max.x / charmap_dim.x;
		float bg_tiles_in_y = world_max.y / charmap_dim.y;
		
		// now figure out how big the tiles must be
		float bg_tile_scale_x =  bg_tiles_in_x / dec_bg_tile->sprite->width;
		float bg_tile_scale_y = bg_tiles_in_y / dec_bg_tile->sprite->height;

		float center_y;
		float center_x;

		// draw background first
		for (int i = 0; i < cargos.size(); ++i) {
			cx = (cargos[i].pos.x - ship_pos.x + ship_on_screen_pos.x);
			cy = (cargos[i].pos.y - ship_pos.y + ship_on_screen_pos.y);
			// don't draw the object if it is outside the clip radius
			// Radius
			//if (sqrt((ship_pos.x - cargos[i].pos.x) * (ship_pos.x - cargos[i].pos.x) + (ship_pos.y - cargos[i].pos.y) * (ship_pos.y - cargos[i].pos.y)) < game_clip_objects_radius) {
			//	center_x = cx - (spr_bg_tile->width * bg_tile_scale_x)/2 ;
			//	center_y = cy - (spr_bg_tile->height * bg_tile_scale_y)/2 ;
			//	DrawDecal(olc::vf2d{ center_x, center_y }, dec_bg_tile, olc::vf2d{ bg_tile_scale_x, bg_tile_scale_y });
			//}

			// clip rect
			if ((cargos[i].pos.x - ship_pos.x) > clip_rectangle.tl.x && (cargos[i].pos.x - ship_pos.x) < clip_rectangle.br.x 
				&& (cargos[i].pos.y - ship_pos.y) > clip_rectangle.tl.y && (cargos[i].pos.y - ship_pos.y) < clip_rectangle.br.y) {
				center_x = cx - (spr_bg_tile->width * bg_tile_scale_x) / 2;
				center_y = cy - (spr_bg_tile->height * bg_tile_scale_y) / 2;
				DrawDecal(olc::vf2d{ center_x, center_y }, dec_bg_tile, olc::vf2d{ bg_tile_scale_x, bg_tile_scale_y });
			}


		}

		// then draw the objects on top
		for (int i = 0; i < cargos.size(); ++i) {
			cx = (cargos[i].pos.x - ship_pos.x + ship_on_screen_pos.x);
			cy = (cargos[i].pos.y - ship_pos.y + ship_on_screen_pos.y);

			switch (cargos[i].cargoType)
			{
			case 'd':
				col = olc::RED;
				break;
			default:
				col = olc::GREEN;
				break;
			}

			float dec_scale;
			// float dec_scaley;

			float center_y;
			float center_x;
			// don't draw the object if it is outside the clip radius
			if (sqrt((ship_pos.x - cargos[i].pos.x) * (ship_pos.x - cargos[i].pos.x) + (ship_pos.y - cargos[i].pos.y) * (ship_pos.y - cargos[i].pos.y)) < game_clip_objects_radius) {

				switch (cargos[i].cargoType)
				{
				case '*':
					dec_scale = 0.3f;
					center_x = cx - (spr_startpad->width * dec_scale) / 2;
					center_y = cy - (spr_startpad->height * dec_scale) / 2;
					DrawDecal(olc::vf2d{ center_x, center_y }, dec_startpad, olc::vf2d{ dec_scale,dec_scale });
					break;
				case 'd':
					dec_scale = 0.1f;
					center_x = cx - (spr_chest->width * dec_scale) / 2;
					center_y = cy - (spr_chest->height * dec_scale) / 2;
					DrawDecal(olc::vf2d{ center_x, center_y }, dec_chest, olc::vf2d{ dec_scale,dec_scale });
					break;
				case ' ':
					break;
				default:
					// Draw the orb
					dec_scale = 0.1f;
					center_x = cx - (spr_orb->width * dec_scale) / 2;
					center_y = cy - (spr_orb->height * dec_scale) / 2;
					DrawDecal(olc::vf2d{ center_x, center_y }, dec_orb, olc::vf2d{ dec_scale,dec_scale });
					break;
				}

				if (cargos[i].cargoType != ' ') {
					ss.str(""); ss << std::setw(1) << static_cast<char>(cargos[i].cargoType);
					DrawStringDecal({ cx - 3.0f,cy - 3.0f }, ss.str());
					DrawStringDecal({ float(int(cx) - 3),float(int(cy) - 3 )}, ss.str());
				}
			}

		}
	}


	void DrawShip() {
		float dec_scale = 0.2f + 0.4f * (altitude / max_altitude);
		float center_x = ship_on_screen_pos.x - (spr_ship->width * dec_scale) / 2;
		float center_y = ship_on_screen_pos.y - (spr_ship->height * dec_scale) / 2;


		// now try tilting it using ships_angle_x
		// when tilting forward (north), tl and tr should get narrower and bl and br should get wider
		float tl_x = ship_on_screen_pos.x + (spr_ship->width * dec_scale) / 2;
		float tl_y = ship_on_screen_pos.y + (spr_ship->height * dec_scale) / 2;

		float bl_x = ship_on_screen_pos.x + (spr_ship->width * dec_scale) / 2;
		float bl_y = ship_on_screen_pos.y - (spr_ship->height * dec_scale) + (spr_ship->height * dec_scale) / 2;

		float br_x = ship_on_screen_pos.x - (spr_ship->width * dec_scale) + (spr_ship->width * dec_scale) / 2;
		float br_y = ship_on_screen_pos.y - (spr_ship->height * dec_scale) + (spr_ship->height * dec_scale) / 2;

		float tr_x = ship_on_screen_pos.x - (spr_ship->width * dec_scale) + (spr_ship->width * dec_scale) / 2;
		float tr_y = ship_on_screen_pos.y + (spr_ship->height * dec_scale) / 2;

		// ships_angle_y: tl and tr narrow
		//                bl and br wider

		// tilt in y -156 -> 0
		if (ship_angle_y <= 0) {
			tl_x = tl_x - tl_x * sin(ship_angle_y) * dec_scale * 0.1f;
			tl_y = tl_y + tl_y * sin(ship_angle_y) * dec_scale * 0.1f;
			tr_x = tr_x + tr_x * sin(ship_angle_y) * dec_scale * 0.1f;
			tr_y = tr_y + tr_y * sin(ship_angle_y) * dec_scale * 0.1f;

			bl_x = bl_x + bl_x * sin(ship_angle_y) * dec_scale * 0.1f;
			bl_y = bl_y - bl_y * sin(ship_angle_y) * dec_scale * 0.1f;
			br_x = br_x - br_x * sin(ship_angle_y) * dec_scale * 0.1f;
			br_y = br_y - br_y * sin(ship_angle_y) * dec_scale * 0.1f;
		}


		// tilt in y 0 ->  1.56
		if (ship_angle_y > 0) {
			tl_x = tl_x - tl_x * sin(ship_angle_y) * dec_scale * 0.1f;
			tl_y = tl_y - tl_y * sin(ship_angle_y) * dec_scale * 0.1f;
			tr_x = tr_x + tr_x * sin(ship_angle_y) * dec_scale * 0.1f;
			tr_y = tr_y - tr_y * sin(ship_angle_y) * dec_scale * 0.1f;

			bl_x = bl_x + bl_x * sin(ship_angle_y) * dec_scale * 0.1f;
			bl_y = bl_y + bl_y * sin(ship_angle_y) * dec_scale * 0.1f;
			br_x = br_x - br_x * sin(ship_angle_y) * dec_scale * 0.1f;
			br_y = br_y + br_y * sin(ship_angle_y) * dec_scale * 0.1f;
		}

		// tilt in x angle -1.56  -> 0
		if (ship_angle_x <= 0) {
			tl_x = tl_x + tl_x * sin(ship_angle_x) * dec_scale * 0.1f;
			tl_y = tl_y - tl_y * sin(ship_angle_x) * dec_scale * 0.1f;
			tr_x = tr_x - tr_x * sin(ship_angle_x) * dec_scale * 0.1f;
			tr_y = tr_y + tr_y * sin(ship_angle_x) * dec_scale * 0.1f;

			bl_x = bl_x + bl_x * sin(ship_angle_x) * dec_scale * 0.1f;
			bl_y = bl_y + bl_y * sin(ship_angle_x) * dec_scale * 0.1f;
			br_x = br_x - br_x * sin(ship_angle_x) * dec_scale * 0.1f;
			br_y = br_y - br_y * sin(ship_angle_x) * dec_scale * 0.1f;
		}
		// tilt in x angle 0  -> 1.56
		if (ship_angle_x > 0) {
			tl_x = tl_x - tl_x * sin(ship_angle_x) * dec_scale * 0.1f;
			tl_y = tl_y - tl_y * sin(ship_angle_x) * dec_scale * 0.1f;
			tr_x = tr_x + tr_x * sin(ship_angle_x) * dec_scale * 0.1f;
			tr_y = tr_y + tr_y * sin(ship_angle_x) * dec_scale * 0.1f;

			bl_x = bl_x - bl_x * sin(ship_angle_x) * dec_scale * 0.1f;
			bl_y = bl_y + bl_y * sin(ship_angle_x) * dec_scale * 0.1f;
			br_x = br_x + br_x * sin(ship_angle_x) * dec_scale * 0.1f;
			br_y = br_y - br_y * sin(ship_angle_x) * dec_scale * 0.1f;
		}

		// warped decal drawing so we can tilt the ship.
		points[0] = { tl_x, tl_y };		// top left
		points[1] = { bl_x, bl_y };		// bottom left
		points[2] = { br_x, br_y };		// bottom right
		points[3] = { tr_x, tr_y };		// top right
		DrawWarpedDecal(dec_ship, points);
	}

	// TODO: draw this minimap into a decal
	void DrawMinimap(olc::vi2d mm_pos, olc::vf2d ship_pos) {
		float scale_x = float(minimap_size.x) / world_max.x;
		float scale_y = float(minimap_size.y) / world_max.y;
		mm_pos = {1,1};
		SetPixelMode(olc::Pixel::Mode::ALPHA);
		// olc::Sprite *oldDrawTarget = GetDrawTarget();
		SetDrawTarget(spr_minimap);
		Clear(olc::VERY_DARK_GREY);

		// draw cargos
		for (int i = 0; i < cargos.size(); ++i) {
			if (cargos[i].cargoType != ' ') {
				// TODO: calculate what size we need regarding the size of map versus size of minimap on screen
				// Draw({ mm_pos.x + int(cargos[i].pos.x * scale_x), mm_pos.y + int(cargos[i].pos.y * scale_y) }, olc::GREEN);
				FillCircle({ mm_pos.x + int(cargos[i].pos.x * scale_x), mm_pos.y + int(cargos[i].pos.y * scale_y) }, 1, olc::GREEN);
			}
		}

		// draw dropzone
		FillCircle({ mm_pos.x + int(dropzone.x * scale_x), mm_pos.y + int(dropzone.y * scale_y) }, 1, olc::RED);

		// draw ship
		DrawCircle({ mm_pos.x + int((ship_pos.x) * scale_x), mm_pos.y + int((ship_pos.y) * scale_y) }, 2, olc::RED);

		// minimap boundary
		// DrawRect({ mm_pos.x - 4,mm_pos.y - 4 }, { minimap_size.x + 3,minimap_size.y + 1 }, olc::YELLOW);

		dec_minimap->Update();
		DrawDecal( minimap_position, dec_minimap);
		SetDrawTarget(nullptr);
		SetPixelMode(olc::Pixel::Mode::NORMAL);
	}

	void DrawAltitude(olc::vf2d pos) {
		float BarHeight = 100;
		float BarWidth = 20;
		float scale = max_altitude / BarHeight;
		float AltBarHeight = altitude / scale;

		DrawRectDecal({ pos.x, pos.y }, { BarWidth, BarHeight });
		FillRectDecal({ pos.x + 1,(pos.y + 100) - (AltBarHeight - 2) }, { BarWidth - 2, (AltBarHeight - 4) }, olc::RED);

		tmpstr = "Alt";
		DrawStringDecal( pos + olc::vf2d{ 0.0f, -10.0f }, tmpstr, olc::GREY);
		ss.str(""); ss << std::setw(3) << int(altitude);
		tmpstr = ss.str();
		DrawStringDecal( pos + olc::vf2d{ 0.0f, 105.0f }, tmpstr, olc::YELLOW);
	}

	void DrawThrottle(olc::vf2d pos) {
		float BarHeight = 100;
		float BarWidth = 20;

		DrawRectDecal( pos, { BarWidth, BarHeight });
		FillRectDecal( pos + olc::vf2d{ 1.0f, pos.y -100.0f - (BarHeight*ship_avr_throttle - 2) }, { BarWidth - 2, BarHeight*ship_avr_throttle - 4 }, olc::RED);

		tmpstr = "Thr";
		DrawStringDecal(pos + olc::vf2d{ 0.0f, -10 }, tmpstr, olc::GREY);
		ss.str(""); ss << std::setw(3) << int(ship_avr_throttle);
		tmpstr = ss.str();
		DrawStringDecal( pos + olc::vf2d{ 0.0f, -105.0f }, tmpstr, olc::YELLOW);
	}


	void DrawZVelocity(olc::vf2d pos, float ship_vel) {
		float BarHeight = 100.0f;
		float BarWidth = 20.0f;
		float scale = game_critical_landing_velocity / float(BarHeight);
		float trueVel = ship_velocity_z * ship_velocity_to_player_scale; // *velocity_scale;
		float AltBarHeight = fabs(trueVel / scale);
		olc::Pixel col = olc::GREEN;

		if (AltBarHeight > BarHeight)
			AltBarHeight = BarHeight;

		if (trueVel < game_critical_landing_velocity * velocity_alert_warning_threshhold) {
			col = olc::RED;
		}

		DrawRectDecal(pos, { BarWidth, BarHeight });
		FillRectDecal(pos + olc::vf2d{ 1.0f, 100.0f - (AltBarHeight - 2) }, { BarWidth - 2, AltBarHeight - 4 }, col);

		// Velocity descent warning light
		if (timer_descent_vel_alert_active) {
			if (timer_toggle_on_state) {
				FillRectDecal({ pos.x - 1 * 8, pos.y - 10 }, { 5.0f * 8, 10.0f }, olc::RED);
				sound_altitude_alert_play = true;
			}
		}

		DrawStringDecal({ pos.x, pos.y - 10 }, "Vel", olc::GREY);
		ss.str(""); ss << std::setw(3) << int(trueVel);
		if (trueVel < 0)
			DrawStringDecal({ pos.x, pos.y + 105 }, ss.str(), olc::RED);
		else
			DrawStringDecal({ pos.x, pos.y + 105 }, ss.str(), olc::YELLOW);
	}

};	// Class Game


int main()
{
	Game hover;

	// 640x360 with no full screen and sync to monitor refresh rate
	if (hover.Construct(640, 360, 2, 2, false,true)) 
		hover.Start();

	return 0;
}
