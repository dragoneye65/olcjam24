#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OLC_SOUNDWAVE
#include "olcSoundWaveEngine.h"


#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// #define DEBUG_PRINT

class Game : public olc::PixelGameEngine
{
public:
	Game()
	{
		sAppName = "Hover";
	}

	olc::sound::WaveEngine wave_engine;
	olc::sound::Wave wave_altitude_alert;
	olc::sound::Wave wave_crash;
	olc::sound::Wave wave_pickup;
	olc::sound::Wave wave_drop;
	olc::sound::Wave wave_purge;
	olc::sound::Wave wave_bgm;
	olc::sound::Wave wave_engine_sound;
	olc::sound::PlayingWave it_altitude_alert;
	olc::sound::PlayingWave it_crash;
	olc::sound::PlayingWave it_pickup;
	olc::sound::PlayingWave it_drop;
	olc::sound::PlayingWave it_purge;
	olc::sound::PlayingWave it_bgm;
	olc::sound::PlayingWave it_engine_sound;

	olc::Sprite* spr_orb = nullptr;	olc::Decal* dec_orb = nullptr;
	olc::Sprite* spr_chest = nullptr;	olc::Decal* dec_chest = nullptr;
	olc::Sprite* spr_startpad = nullptr;	olc::Decal* dec_startpad = nullptr;
	olc::Sprite* spr_bg_tile = nullptr;	olc::Decal* dec_bg_tile = nullptr;
	olc::Sprite* spr_ship = nullptr;	olc::Decal* dec_ship = nullptr;

	// for the warped decal 
	std::array<olc::vf2d, 4> points;
	olc::vf2d* pSelected = nullptr;

	bool sound_altitude_alert_play = false;
	bool sound_crash_play = false;
	bool sound_drop_play = false;
	bool sound_pickup_play = false;
	bool sound_purge_play = false;

	double engine_sound_speed = 1.0;

	// first char based game map, don't need it, can generate it
	std::string game_map;
	olc::vi2d charmap_dim = { 16, 10 };

	enum state { INTRO, GAMEON, THEEND, GAMEWON };
	enum state game_state = state::INTRO;
	float game_critical_landing_velocity = -190.0f;
	float game_object_proximity_limit = 10.0f;
	float game_clip_objects_radius = 120.0f;

	olc::vi2d mouse_pos, mouse_pos_old;

	olc::vi2d instructions_pos = { 50, 50 };
	int player_points = 0;
	int player_deliveries = 0;

	// ship, 4 engines 
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
	float last_velocity_before_crashlanding = 0.0f;					// checking for pancake effect
	float velocity_alert_warning_threshhold = 0.6f;
	float ship_autolevel_toggle = true;
	float ship_autothrottle_toggle = true;
	float ship_response = 2.0f;										// respons factor on the movement
	float ship_avr_throttle = 0.0f;
	float ship_idle_throttle = 0.01f;
	float ship_max_thrust = 4000.0f;								// grams
	float ship_max_angle = 0.7853981634f; 							// M_PI_4 pi/4 = 45 deg
	float pi_2 = 1.57079632679f;									// 
	bool ship_crashed = false;
	int ship_docket_at_cargoType = 0;
	bool ship_tilt_key_held = false;
	bool ship_throttle_key_held = false;
	float auto_alt_hold = 0;

	bool game_toggle_intro = false;

	// engine quad layout
	//   1     4
	//      X
	//   2     3

	float ship_angle_x = 0.0;
	float ship_angle_y = 0.0;
	olc::vf2d ship_pos;

	// world
	float gravity = 0.4f;
	float altitude = 0.0f;									// 0m  sea level
	float max_altitude = 140.0f;
	olc::vf2d world_max = { 500.0f,500.0f };

	olc::vi2d ship_on_screen_pos;

	// minimap
	olc::vi2d minimap_size = { 80, 80 };
	olc::vi2d minimap_position;
	struct cargo {
		olc::vf2d pos;			// position in world space
		int cargoType;			// type of cargo: 
	};

	olc::vf2d startpos;			// doh
	olc::vf2d dropzone;  		// dropzone
	std::vector<cargo> cargos;	// yea you guessed it
	// std::vector<cargo>::iterator cargo_it;

	bool cargo_no_pickup = false;


	int ship_inventory_capasity = 3;
	std::vector<cargo> inventory;
	olc::vi2d inventory_pos = { 10, 50 };

	std::string tmpstr;
	std::stringstream ss;

	// timers
	bool timer_descent_vel_alert_active = false;
	bool timer_toggle_on_state = false;

	// do the blinking/alert stick shaker etc...
	bool isToggled = false;
	std::chrono::steady_clock::time_point lastToggleTime;


	struct Ship {
		float x;
		float y;
		float z;  // Altitude
		float vel_x;
		float vel_y;
		float vel_z;
		float max_z;   // where to cap the altitude, wouldn't want to hurt the player uhm?
		float angle;
		float thrust;		// max thrust the ship can generate
		float maxSpeed;
		//olc::vf2d thrust1;
		//olc::vf2d thrust2;
		//olc::vf2d thrust3;
		//olc::vf2d thrust4;
		float throttle1;
		float throttle2;
		float throttle3;
		float throttle4;
		olc::vi2d screen_pos;
	};

	Ship inferiourBattleCruiser;
	Ship oldRustyBucket;

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

		boostScale = 1.0f;

		// *: starting position
		// u: unloading pad
		// 0-9: cargo type, 
		//	  0:stationary (usually just bomb it)
		//  1-5: moveable  (pic up and drop these different cargo types)
		//  6-9: running   (now we are talking, pick up or bomb those running suckers)
		game_map = "9              9";
		game_map += "     0   0      ";
		game_map += "  5         3   " ;
		game_map += "       2        " ;
		game_map += "   d  2*2  1    ";
		game_map += "       2        ";
		game_map += "    0    6      ";
		game_map += "                ";
		game_map += "     2      8   ";
		game_map += "9      3       9";


		ship_on_screen_pos = { ScreenWidth() / 2,ScreenHeight() / 2 };
		minimap_position = { ScreenWidth() - minimap_size.x - 2, 10 };

		InitGameMap();
		ship_pos = startpos;

		wave_engine.InitialiseAudio();

		// std::string sound_file_path;
		wave_bgm.LoadAudioWaveform("./res/wav/bgm.wav");
		wave_engine.PlayWaveform(&wave_bgm, true);

		wave_altitude_alert.LoadAudioWaveform("./res/wav/altitude_alert.wav");
		wave_pickup.LoadAudioWaveform( "./res/wav/pickup.wav");
		wave_drop.LoadAudioWaveform( "./res/wav/drop.wav");
		wave_crash.LoadAudioWaveform( "./res/wav/crash.wav");
		wave_purge.LoadAudioWaveform("./res/wav/purge.wav");

		wave_engine_sound.LoadAudioWaveform("./res/wav/engine2.wav");
		it_engine_sound = wave_engine.PlayWaveform(&wave_engine_sound, true, 1.0);
		wave_engine.SetOutputVolume(0.0f);
		// it_engine_sound->bFlagForStop = true;

		spr_orb = new olc::Sprite("./res/img/orb.png");	dec_orb = new olc::Decal(spr_orb);
		spr_chest = new olc::Sprite("./res/img/chest.png");	dec_chest = new olc::Decal(spr_chest);
		spr_startpad = new olc::Sprite("./res/img/startpad.png");	dec_startpad = new olc::Decal(spr_startpad);
		spr_bg_tile = new olc::Sprite("./res/img/bg_tile.png");	dec_bg_tile = new olc::Decal(spr_bg_tile);
		spr_ship = new olc::Sprite("./res/img/ship.png");	dec_ship = new olc::Decal(spr_ship);


		// cargo_it = cargos.begin();

		// Ships initial values
		InitializeShip(inferiourBattleCruiser, startpos);
		inferiourBattleCruiser.x = startpos.x;
		inferiourBattleCruiser.x = startpos.y;
		inferiourBattleCruiser.thrust = ship_max_thrust;
		
		oldRustyBucket = inferiourBattleCruiser;
		oldRustyBucket.x += 10.0f;
		oldRustyBucket.y -= 10.0f;


		return true;
	}

	void TimerUpdateTrigger(float fElapsedTime, float time_until_trigger) {
		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lastToggleTime).count() >= time_until_trigger) {
			timer_toggle_on_state = !timer_toggle_on_state;
			lastToggleTime = std::chrono::steady_clock::now();
		}
	}


	// Calculate the ship movements
	void updateShip(Ship& ship, float fElapsedTime) {
		// Calculate thrust for each engine
		ship.angle = atan2(ship.z, sqrt(pow(ship.x, 2) + pow(ship.y, 2)));

		// individual engine thrust
		float thrustX1 = ship.thrust * ship.throttle1 * cos(ship.angle + M_PI / 4);
		float thrustY1 = ship.thrust * ship.throttle1 * sin(ship.angle + M_PI / 4);
		float thrustX2 = ship.thrust * ship.throttle2 * cos(ship.angle - M_PI / 4);
		float thrustY2 = ship.thrust * ship.throttle2 * sin(ship.angle - M_PI / 4);
		float thrustX3 = ship.thrust * ship.throttle3 * cos(ship.angle - 3 * M_PI / 4);
		float thrustY3 = ship.thrust * ship.throttle3 * sin(ship.angle - 3 * M_PI / 4);
		float thrustX4 = ship.thrust * ship.throttle4 * cos(ship.angle + 3 * M_PI / 4);
		float thrustY4 = ship.thrust * ship.throttle4 * sin(ship.angle + 3 * M_PI / 4);

		// todo: do I need this later?
		//ship.thrust1 = { thrustX1, thrustY1 };
		//ship.thrust2 = { thrustX2, thrustY2 };
		//ship.thrust3 = { thrustX3, thrustY3 };
		//ship.thrust4 = { thrustX4, thrustY4 };

		// Calculate total thrust
		float thrustX = thrustX1 + thrustX2 + thrustX3 + thrustX4;
		float thrustY = thrustY1 + thrustY2 + thrustY3 + thrustY4;
		float thrustZ = thrustX1 + thrustX2 + thrustX3 + thrustX4;

		
		// Update velocity, this time around it should be frame rate independend, we'll see :)
		ship.vel_x += thrustX * fElapsedTime;
		ship.vel_y += thrustY * fElapsedTime;
		ship.vel_z += thrustZ * fElapsedTime;

		// Limit velocity
		float velocityMagnitude = sqrt(pow(ship.vel_x, 2) + pow(ship.vel_y, 2) + pow(ship.vel_z, 2));
		if (velocityMagnitude > ship.maxSpeed) {
			float scaleFactor = ship.maxSpeed / velocityMagnitude;
			ship.vel_x *= scaleFactor;
			ship.vel_y *= scaleFactor;
			ship.vel_z *= scaleFactor;
		}
		ship.x += ship.vel_x;
		ship.y += ship.vel_y;
		ship.z += ship.vel_z;
	}



	bool OnUserUpdate(float fElapsedTime) override
	{
		// called once per frame
		mouse_pos_old = mouse_pos;
		mouse_pos = GetMousePos();


		TimerUpdateTrigger(fElapsedTime, 200);
		/*
				if (timer_toggle_on_state) {
					timer_descent_vel_alert_active = true;
				} else {
					timer_descent_vel_alert_active = false;
				}
		*/

		Clear(olc::VERY_DARK_BLUE);


		// show mouse cursor + position
		DrawLine(mouse_pos - olc::vi2d{ 5,5 }, mouse_pos + olc::vi2d{ 5,5 }, olc::RED);
		DrawLine(mouse_pos - olc::vi2d{ -5,5 }, mouse_pos + olc::vi2d{ -5,5 }, olc::RED);
		//ss.str(""); ss << "(" << mouse_pos.x << "," << mouse_pos.y << ")";
		// DrawString(mouse_pos, ss.str(), olc::RED);


		if (game_state == state::GAMEON) {


			// calculate angle from thrust differentials (simple version) max angle = 45 deg
			// todo: wrong way, must improve...
			//       Should be able to have max throttle at any angle without the ship rights itself
			ship_angle_x = pi_2 * (((throttle1 + throttle2) - (throttle3 + throttle4)) / 2);
			ship_angle_y = pi_2 * ((throttle1 + throttle4) - (throttle2 + throttle3)) / 2;

			// boost throttle if shift is held
			if (GetKey(olc::Key::SHIFT).bHeld)
				boostScale = 4.0f;
			else 
				boostScale = 1.0f;

			ship_throttle_key_held = false;
			// power to all engines
			if (GetKey(olc::Key::UP).bHeld) {
				if (altitude < max_altitude) {
					throttle1 += fElapsedTime * ship_response * boostScale; if (throttle1 > 1.0 * boostScale) throttle1 = 1.0 * boostScale;
					throttle2 += fElapsedTime * ship_response * boostScale; if (throttle2 > 1.0 * boostScale) throttle2 = 1.0 * boostScale;
					throttle3 += fElapsedTime * ship_response * boostScale; if (throttle3 > 1.0 * boostScale) throttle3 = 1.0 * boostScale;
					throttle4 += fElapsedTime * ship_response * boostScale; if (throttle4 > 1.0 * boostScale) throttle4 = 1.0 * boostScale;
					ship_throttle_key_held = true;
					auto_alt_hold = altitude;
				}

			}

			if (GetKey(olc::Key::DOWN).bHeld) {
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

				// autothrottle if no throttle key input
				if (ship_autothrottle_toggle && !ship_throttle_key_held) {
					/*
					float autothrottle_response = 60.0f;
					float trueVel = ship_velocity_z; // *ship_velocity_to_player_scale;
					float altitudeDifference = auto_alt_hold - altitude;

					if (altitudeDifference > 0) {
						throttle1 += fElapsedTime * autothrottle_response * altitudeDifference/auto_alt_hold;
						throttle2 += fElapsedTime * autothrottle_response * altitudeDifference / auto_alt_hold;
						throttle3 += fElapsedTime * autothrottle_response * altitudeDifference / auto_alt_hold;
						throttle4 += fElapsedTime * autothrottle_response * altitudeDifference / auto_alt_hold;

						if (throttle1 > 1.0f) throttle1 = 1.0f;
						if (throttle2 > 1.0f) throttle2 = 1.0f;
						if (throttle3 > 1.0f) throttle3 = 1.0f;
						if (throttle4 > 1.0f) throttle4 = 1.0f;
					}
					else if(altitudeDifference < 0) {
						throttle1 -= fElapsedTime * autothrottle_response / 2;
						throttle2 -= fElapsedTime * autothrottle_response / 2;
						throttle3 -= fElapsedTime * autothrottle_response / 2;
						throttle4 -= fElapsedTime * autothrottle_response / 2;
						if (throttle1 < 0.0f) throttle1 = 0.0f;
						if (throttle2 < 0.0f) throttle2 = 0.0f;
						if (throttle3 < 0.0f) throttle3 = 0.0f;
						if (throttle4 < 0.0f) throttle4 = 0.0f;
					}
					*/
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

				// velocity = distance * time
				// position = velocity * time
				// acceleration += gravity * time
				// cap acceleration to gravity

				// ss.str(""); ss << "Vel " << std::setw(6) << ship_velocity_z;
				// DrawString({ 20,200 }, ss.str(), olc::YELLOW);

				float gameSpeed = 20.0f;
				// to hight, auto throtteling down
				if (altitude >= max_altitude)
					ship_velocity_z = 0.0;
				else
					// thrust
					ship_velocity_z += fElapsedTime * gameSpeed * cos(ship_angle_x) * cos(ship_angle_y) * ship_avr_throttle;
				// ship_velocity_z += 200.0f *  fElapsedTime;
				// ship_velocity_z += fElapsedTime * 0.005f * (ship_max_thrust)*cos(ship_angle_x) * cos(ship_angle_y) * ship_avr_throttle;

			// Gravity
			// ship_velocity_z -= fElapsedTime  * 0.001f * ship_weight * gravity;

				ship_velocity_z -= fElapsedTime * gameSpeed * gravity;
				ship_velocity_z -= fElapsedTime * ship_weight * 0.005; // normalize weight
				ship_velocity_x += fElapsedTime * gameSpeed * sin(ship_angle_x);
				ship_velocity_y += fElapsedTime * gameSpeed * sin(ship_angle_y);


				if (game_state == state::GAMEON)
					last_velocity_before_crashlanding = ship_velocity_z; // *velocity_scale;    // for checking if you crashed hard into ground


				altitude += fElapsedTime * gameSpeed * ship_velocity_z;

				// cap ship velocity in xy
				if ((ship_velocity_x /* * velocity_scale */) > ship_cap_vel_xy.x) 	ship_velocity_x = ship_cap_vel_xy.x /* / velocity_scale */;
				if ((ship_velocity_x /* * velocity_scale */) < -ship_cap_vel_xy.x)	ship_velocity_x = -(ship_cap_vel_xy.x /* / velocity_scale */);
				if ((ship_velocity_y /* * velocity_scale */) > ship_cap_vel_xy.y) 	ship_velocity_y = ship_cap_vel_xy.y /* / velocity_scale */;
				if ((ship_velocity_y /* * velocity_scale */) < -ship_cap_vel_xy.y)	ship_velocity_y = -(ship_cap_vel_xy.y /* / velocity_scale */);

				ship_pos.x += fElapsedTime * gameSpeed / 4 * ship_velocity_x;
				ship_pos.y += fElapsedTime * gameSpeed / 4 * ship_velocity_y;

				// limit the ship inside the map area , bounch back
				if (ship_pos.x < 0.0f) { ship_pos.x = 0.0f; ship_velocity_x *= -1.0f; }
				if (ship_pos.x > 500.0f) { ship_pos.x = 500.0f; ship_velocity_x *= -1.0f; }
				if (ship_pos.y < 0.0f) { ship_pos.y = 0.0f; ship_velocity_y *= -1.0f; }
				if (ship_pos.y > 500.0f) { ship_pos.y = 500.0f; ship_velocity_y *= -1.0f; }

#ifdef DEBUG_PRINT
				// Show the position to the ship under minimap
				ss.str(""); ss << ship_pos.x; tmpstr = ss.str();
				DrawString({ minimap_position.x + 20,minimap_position.y + minimap_size.y + 2 }, tmpstr, olc::YELLOW);
				ss.str(""); ss << ship_pos.y; tmpstr = ss.str();
				DrawString({ minimap_position.x + 20,minimap_position.y + minimap_size.y + 12 }, tmpstr, olc::YELLOW);
#endif

				// Altitude check limits
				//if (altitude > max_altitude) {
					// altitude = max_altitude;
					// ship_velocity_z = 0.0f;
				//}

				if (altitude < 0.0f) {
					altitude = 0.0f;
					ship_velocity_x = 0.0f; // x
					ship_velocity_y = 0.0f; // y
					ship_velocity_z = 0.0f; // z
				}

				// engine speed sound reletive to the avg throttle
				engine_sound_speed = 1.0 + 2.0 * ship_avr_throttle;
				it_engine_sound->dSpeedModifier = engine_sound_speed * it_engine_sound->pWave->file.samplerate() / 44100.0;
				it_engine_sound->dDuration = it_engine_sound->pWave->file.duration() / engine_sound_speed;


				// Moved to DrawGameMapOnScreen() doe to the z-order of drawing, ship is on top, ie last
				// DrawShipOnScreen(ship_on_screen_pos, 10, 20, altitude); // x,y,engine minsize, maxsize, altitude

#ifdef DEBUG_PRINT
			// show ship real position
				ss.str("");
				ss << "(" << ship_on_screen_pos.x << "," << ship_on_screen_pos.y << ")";
				DrawString({ 0,0 }, ss.str(), olc::YELLOW);
#endif

			} // physics update toggled off if game_toggle_intro

			// toggle the intro/instruction page
			if (GetKey(olc::Key::F1).bPressed) {
				game_toggle_intro = !game_toggle_intro;
			}

			// Dont draw anything while showing the intro screen
			if (!game_toggle_intro) {
				DrawAltitude(500, 200);
				DrawZVelocity(530, 200, ship_velocity_z); // *velocity_scale);
				DrawThrottle(560, 200);
				//	DrawShipAngle( ship_on_screen_pos, ship_angle_x, ship_angle_y);
				DrawMinimap(minimap_position, ship_pos);
				DrawGameMapOnScreen(ship_pos);

				// old working ship
				// keep him around just for comparizon
				DrawShip();
			} else
				Instructions(instructions_pos + olc::vi2d{ 30,50 });


			// DrawShipNew(oldRustyBucket);	// new version, todo: get it working!

			// TODO: Must make it frame rate independent!
			// give the update function it's trottle parameters which is normalized
			// gotta calc send in our own throttle
			inferiourBattleCruiser.throttle1 = throttle1;
			inferiourBattleCruiser.throttle2 = throttle2;
			inferiourBattleCruiser.throttle3 = throttle3;
			inferiourBattleCruiser.throttle4 = throttle4;

			// updateShip(inferiourBattleCruiser, fElapsedTime);
			// DrawShipNew(inferiourBattleCruiser);	// new version, todo: get it working!

#ifdef DEBUG_PRINT
			// Draw new ship velocity and angle
			ss.str(""); ss << "Vel: " << inferiourBattleCruiser.vel_x << "," << inferiourBattleCruiser.vel_y << " Ang " << inferiourBattleCruiser.angle;
			DrawString({ 100,100 }, ss.str());
#endif



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
			if (cargos.size() == 2) {  // only startpad and dropzone left
				if (inventory.size() == 0)   // and inventory delivered
					game_state = state::GAMEWON;  // goal!!!
			}

			// Show inventory and calculate weight
			int cargo_weight = ShowInventory(inventory_pos);
			ship_weight = ship_net_weight + cargo_weight;
			tmpstr = "Cargo Weight";
			DrawString({ 100,10 }, tmpstr, olc::GREEN);
			ss.str(""); ss << std::setw(4) << cargo_weight;
			DrawString({ 100+8*8, 18 }, ss.str(), olc::RED);


			tmpstr = "Score";
			DrawString({ 10,10 }, tmpstr, olc::GREEN);
			ss.str(""); ss << std::setw(5) << int(player_points);
			DrawString({ 10,18 }, ss.str(), olc::RED);

			timer_descent_vel_alert_active = false;
			// show alert if decending dangerously fast
			if (int(altitude) != 0 && ship_velocity_z /* * velocity_scale */ < (game_critical_landing_velocity + 40.0f) ) {
				timer_descent_vel_alert_active = true;
			}

			// check z velocity on "landing"
			if (int(altitude) == 0 && last_velocity_before_crashlanding*15.0f < game_critical_landing_velocity) {
				game_state = state::THEEND;
				ship_crashed = true;
				sound_crash_play = true;
			}

			// debug: check if debug ship_crash is set
			if (ship_crashed)
				game_state = state::THEEND;

#ifdef DEBUG_PRINT
			// Set ship to next object
			if (GetKey(olc::Key::N).bReleased) {
				cargo_no_pickup = true;
				cargo_it++;
				if (cargo_it != cargos.end())
					ship_pos = cargo_it->pos;
				else
					cargo_it = cargos.begin();
			}
#endif


			// play the crash sound
			if (sound_crash_play) {
				sound_crash_play = false;
				wave_engine.PlayWaveform(&wave_crash);
			}

			// play the altitude alert sound
			if (sound_altitude_alert_play) {
				sound_altitude_alert_play = false;
				// this triggers every 500ms,  it frikes out the wave_engine...  so silence it is for now
				// it_altitude_alert = wave_engine.PlayWaveform(&wave_altitude_alert);
				// it_altitude_alert->bFlagForStop = true;
			}

			// play the cargo pickup sound
			if (sound_pickup_play) {
				sound_pickup_play = false;
				wave_engine.PlayWaveform(&wave_pickup);
			}

			// play the drop sound
			if (sound_drop_play) {
				sound_drop_play = false;
				wave_engine.PlayWaveform(&wave_drop);
			}

			// play the purge sound
			if (sound_purge_play) {
				sound_purge_play = false;
				wave_engine.PlayWaveform(&wave_purge);
			}

			// Purge the last item from inventory
			if (GetKey(olc::Key::P).bPressed) {
				if (inventory.size() > 0) {
					inventory.erase(inventory.end() - 1);
					sound_purge_play = true;
				}
			}

		} // endif: state_GAMEON ---


		// GameWon state
		if (game_state == state::GAMEWON) {
			GameWon();

			if (GetKey(olc::Key::ENTER).bReleased || GetKey(olc::Key::SPACE).bReleased) {
				RestartGame();
			}
		}

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

				// set sound on
				wave_engine.SetOutputVolume(0.8f);
			}
		}

		// Game ended, or user aborted
		if (game_state == state::THEEND) {
			EndGame();

			// SPACE: continue if not crashed, restarts if crashed
			if (GetKey(olc::Key::SPACE).bReleased) {
				if (!ship_crashed) {
					game_state = state::GAMEON;
					wave_engine.SetOutputVolume(0.8f);
				}
				else
					RestartGame();

			}

			// ENTER: restart
			if (GetKey(olc::Key::ENTER).bReleased) {
				RestartGame();
			}

		}

		// Debug: <SHIFT-C> toggle ship_crash
		if (GetKey(olc::Key::SHIFT).bHeld)
			if (GetKey(olc::Key::C).bReleased) 
				ship_crashed = !ship_crashed;


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
			wave_engine.SetOutputVolume(0.0f);
		}

#ifdef DEBUG_PRINT
		// show center of screen crossheir
		olc::vi2d screen_center{ ScreenWidth() / 2, ScreenHeight() / 2 };
		DrawLine(screen_center - olc::vi2d{ 5,5 }, screen_center + olc::vi2d{ 5,5 }, olc::RED);
		DrawLine(screen_center - olc::vi2d{ -5,5 }, screen_center + olc::vi2d{ -5,5 }, olc::RED);
		ss.str(""); ss << "(" << screen_center.x << "," << screen_center.y << ")";
		DrawString(screen_center, ss.str(), olc::RED);
#endif


		return true;
	} // end Update ---


	void InitializeShip(Ship& ship, olc::vf2d pos) {

		ship.angle = 0.0f;
		ship.maxSpeed = 100.0f;
		ship.throttle1 = 0.1f;
		ship.throttle2 = 0.1f;
		ship.throttle3 = 0.1f;
		ship.throttle4 = 0.1f;
		ship.thrust = 0.0f;
		ship.x = 0.0f;
		ship.y = 0.0f;
		ship.z = 0.0f;
		ship.vel_x = 0.0f;
		ship.vel_y = 0.0f;
		ship.vel_z = 0.0f;
		ship.max_z = 100.0f;
		ship.screen_pos = pos;
	}

	void RestartGame() {
		game_state = state::INTRO;
		InitGameMap();
		ship_pos = startpos;

		InitializeShip(inferiourBattleCruiser, startpos);
		inferiourBattleCruiser.thrust = ship_max_thrust;
		inferiourBattleCruiser.max_z = 100.0f;
		inferiourBattleCruiser.screen_pos = ship_on_screen_pos;

		oldRustyBucket = inferiourBattleCruiser;
		oldRustyBucket.x += -10;
		oldRustyBucket.y += 10;
	}

	void Instructions(olc::vi2d pos) {
		int offsy = 10; int yc = 0;
		FillRect({ pos.x - 1,  pos.y - 1 }, { 500 + 2, 220 + 2 }, olc::VERY_DARK_GREY);
		DrawRect({ pos.x, pos.y }, { 500, 220 }, olc::GREEN);

		ss << std::fixed << std::setprecision(2);
		DrawString({ ScreenWidth() / 2 - 10 * 16 / 2,24 }, "Hover run", olc::YELLOW, 2);

		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "     As a drone pilot I just had to make this game.        ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                                                           ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, " The game is played by watching the altitude carefully     ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, " while running the missions gathering cargo by landing     ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, " softly on top of them,you can pick up as many as you like ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "      after pickup you fly and land on the dropzone        ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                                                           ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, " Be aware that when you roll and pitch you loose altitude. ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "    You will have to increse throttle to stay airborn.     ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                                                           ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                If you land hard you crash!                ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                                                           ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                    A,D,W,S Roll/Pitch                     ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                     UP,DOWN Throttle                      ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                 SPACE Toggle autoleveling                 ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                  P Purge from inventory                   ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                                                           ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                     F1 This info page                     ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                                                           ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * ++yc }, "                     Author: DragonEye                     ", olc::DARK_GREEN);
	}



	void EndGame() {
		int offsy = 10;
		int offsx = ScreenWidth() / 4;
		int asdf = ScreenHeight() / 4;
		FillRect({ offsx - 1, asdf - 1 }, { 300 + 2, 220 + 2 }, olc::VERY_DARK_GREY);
		DrawRect({ offsx, asdf }, { 300, 220 }, olc::RED);

		//		if (last_velocity_before_crashlanding <= game_critical_landing_velocity) {
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
			DrawRect({ pos.x, pos.y + 8 }, { 80,offs * j + 3 });
		}
		else {
			DrawString({ pos.x + 10 + 8 * 8, pos.y }, "Empty", olc::RED);
		}

		DrawString({ pos.x, pos.y }, "Inventory", olc::GREEN);

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



#define NEWFUNC_DrawGameMapOnScreen
#ifdef NEWFUNC_DrawGameMapOnScreen
	void DrawGameMapOnScreen(olc::vf2d ship_pos) {
		float cx;
		float cy;
		olc::Pixel col = olc::GREEN;

		// draw background first
		for (int i = 0; i < cargos.size(); ++i) {
			cx = (cargos[i].pos.x - ship_pos.x + ship_on_screen_pos.x);
			cy = (cargos[i].pos.y - ship_pos.y + ship_on_screen_pos.y);

			float dec_scale;
			float dec_scaley;

			float center_y;
			float center_x;
			// don't draw the object if it is outside the clip radius
			if (sqrt((ship_pos.x - cargos[i].pos.x) * (ship_pos.x - cargos[i].pos.x) + (ship_pos.y - cargos[i].pos.y) * (ship_pos.y - cargos[i].pos.y)) < game_clip_objects_radius) {
				//				DrawDecal(olc::vf2d{ cx - 12, cy - 13 }, dec_bg_tile, olc::vf2d{ 0.01f,0.01f });

				dec_scale = 0.25f;
				dec_scaley = 0.50;
				center_x = cx - (spr_bg_tile->width * dec_scale) / 2;
				center_y = cy - (spr_bg_tile->height * dec_scaley) / 2;
				DrawDecal(olc::vf2d{ center_x, center_y }, dec_bg_tile, olc::vf2d{ dec_scale,dec_scaley });
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
			float dec_scaley;

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
					// DrawCircle({ int(cx),int(cy) }, 10, col);
					ss.str(""); ss << std::setw(1) << static_cast<char>(cargos[i].cargoType);
					DrawString({ int(cx) - 3,int(cy) - 3 }, ss.str());
				}
			}

/*			
			dec_scale = 0.2f + 0.4f * (altitude/max_altitude);
			center_x = ship_on_screen_pos.x - (spr_ship->width * dec_scale) / 2;
			center_y = ship_on_screen_pos.y - (spr_ship->height * dec_scale) / 2;
			// draw the ship
			// DrawDecal(olc::vf2d{ center_x, center_y }, dec_ship, olc::vf2d{ dec_scale,dec_scale });
			// draw the propps


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

			// test warped decal drawing so we can tilt the ship.
			// warped decal thingy
			points[0] = { tl_x, tl_y };		// top left
			points[1] = { bl_x, bl_y };		// bottom left
			points[2] = { br_x, br_y };		// bottom right
			points[3] = { tr_x, tr_y };		// top right
			DrawWarpedDecal(dec_ship, points);
*/
		}
	}


	void DrawShipNew(Ship& ship) {

		float angle_x = atan2(ship.y, ship.x);
		float angle_y = atan2(ship.z, sqrt(pow(ship.x, 2) + pow(ship.y, 2)));

		float dec_scale = 0.2f + 0.4f * (ship.z / ship.max_z);

		float center_x = ship.screen_pos.x - (spr_ship->width * dec_scale) / 2;
		float center_y = ship.screen_pos.y - (spr_ship->height * dec_scale) / 2;


		// now try tilting it using ships_angle_x
		// when tilting forward (north), tl and tr should get narrower and bl and br should get wider
		float tl_x = ship.screen_pos.x + (spr_ship->width * dec_scale) / 2;
		float tl_y = ship.screen_pos.y + (spr_ship->height * dec_scale) / 2;

		float bl_x = ship.screen_pos.x + (spr_ship->width * dec_scale) / 2;
		float bl_y = ship.screen_pos.y - (spr_ship->height * dec_scale) + (spr_ship->height * dec_scale) / 2;

		float br_x = ship.screen_pos.x - (spr_ship->width * dec_scale) + (spr_ship->width * dec_scale) / 2;
		float br_y = ship.screen_pos.y - (spr_ship->height * dec_scale) + (spr_ship->height * dec_scale) / 2;

		float tr_x = ship.screen_pos.x - (spr_ship->width * dec_scale) + (spr_ship->width * dec_scale) / 2;
		float tr_y = ship.screen_pos.y + (spr_ship->height * dec_scale) / 2;

		// ships_angle_y: tl and tr narrow
		//                bl and br wider
		/*
		tl_x = tl_x - tl_x * dec_scale * 0.1f * sin(ship_angle_y);
		tl_y = tl_y - tl_y * dec_scale * 0.1f * sin(ship_angle_y);
		tr_x = tr_x + tr_x * dec_scale * 0.1f * sin(ship_angle_y);
		tr_y = tr_y - tr_y * dec_scale * 0.1f * sin(ship_angle_y);

		bl_x = bl_x + bl_x * dec_scale * 0.1f * sin(ship_angle_y);
		br_x = br_x - br_x * dec_scale * 0.1f * sin(ship_angle_y);
		*/
		// tilt in y -156 -> 0
		if (angle_y <= 0) {
			tl_x = tl_x - tl_x * sin(angle_y) * dec_scale * 0.1f;
			tl_y = tl_y + tl_y * sin(angle_y) * dec_scale * 0.1f;
			tr_x = tr_x + tr_x * sin(angle_y) * dec_scale * 0.1f;
			tr_y = tr_y + tr_y * sin(angle_y) * dec_scale * 0.1f;

			bl_x = bl_x + bl_x * sin(angle_y) * dec_scale * 0.1f;
			bl_y = bl_y - bl_y * sin(angle_y) * dec_scale * 0.1f;
			br_x = br_x - br_x * sin(angle_y) * dec_scale * 0.1f;
			br_y = br_y - br_y * sin(angle_y) * dec_scale * 0.1f;
		}


		// tilt in y 0 ->  1.56
		if (angle_y > 0) {
			tl_x = tl_x - tl_x * sin(angle_y) * dec_scale * 0.1f;
			tl_y = tl_y - tl_y * sin(angle_y) * dec_scale * 0.1f;
			tr_x = tr_x + tr_x * sin(angle_y) * dec_scale * 0.1f;
			tr_y = tr_y - tr_y * sin(angle_y) * dec_scale * 0.1f;

			bl_x = bl_x + bl_x * sin(angle_y) * dec_scale * 0.1f;
			bl_y = bl_y + bl_y * sin(angle_y) * dec_scale * 0.1f;
			br_x = br_x - br_x * sin(angle_y) * dec_scale * 0.1f;
			br_y = br_y + br_y * sin(angle_y) * dec_scale * 0.1f;
		}

		// tilt in x angle -1.56  -> 0
		if (angle_x <= 0) {
			tl_x = tl_x + tl_x * sin(angle_x) * dec_scale * 0.1f;
			tl_y = tl_y - tl_y * sin(angle_x) * dec_scale * 0.1f;
			tr_x = tr_x - tr_x * sin(angle_x) * dec_scale * 0.1f;
			tr_y = tr_y + tr_y * sin(angle_x) * dec_scale * 0.1f;

			bl_x = bl_x + bl_x * sin(angle_x) * dec_scale * 0.1f;
			bl_y = bl_y + bl_y * sin(angle_x) * dec_scale * 0.1f;
			br_x = br_x - br_x * sin(angle_x) * dec_scale * 0.1f;
			br_y = br_y - br_y * sin(angle_x) * dec_scale * 0.1f;
		}
		// tilt in x angle 0  -> 1.56
		if (angle_x > 0) {
			tl_x = tl_x - tl_x * sin(angle_x) * dec_scale * 0.1f;
			tl_y = tl_y - tl_y * sin(angle_x) * dec_scale * 0.1f;
			tr_x = tr_x + tr_x * sin(angle_x) * dec_scale * 0.1f;
			tr_y = tr_y + tr_y * sin(angle_x) * dec_scale * 0.1f;

			bl_x = bl_x - bl_x * sin(angle_x) * dec_scale * 0.1f;
			bl_y = bl_y + bl_y * sin(angle_x) * dec_scale * 0.1f;
			br_x = br_x + br_x * sin(angle_x) * dec_scale * 0.1f;
			br_y = br_y - br_y * sin(angle_x) * dec_scale * 0.1f;
		}

		// test warped decal drawing so we can tilt the ship.
		// warped decal thingy
		points[0] = { tl_x, tl_y };		// top left
		points[1] = { bl_x, bl_y };		// bottom left
		points[2] = { br_x, br_y };		// bottom right
		points[3] = { tr_x, tr_y };		// top right
		DrawWarpedDecal(dec_ship, points);
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
		/*
		tl_x = tl_x - tl_x * dec_scale * 0.1f * sin(ship_angle_y);
		tl_y = tl_y - tl_y * dec_scale * 0.1f * sin(ship_angle_y);
		tr_x = tr_x + tr_x * dec_scale * 0.1f * sin(ship_angle_y);
		tr_y = tr_y - tr_y * dec_scale * 0.1f * sin(ship_angle_y);

		bl_x = bl_x + bl_x * dec_scale * 0.1f * sin(ship_angle_y);
		br_x = br_x - br_x * dec_scale * 0.1f * sin(ship_angle_y);
		*/
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

		// test warped decal drawing so we can tilt the ship.
		// warped decal thingy
		points[0] = { tl_x, tl_y };		// top left
		points[1] = { bl_x, bl_y };		// bottom left
		points[2] = { br_x, br_y };		// bottom right
		points[3] = { tr_x, tr_y };		// top right
		DrawWarpedDecal(dec_ship, points);
	}

	void DrawMinimap(olc::vi2d mm_pos, olc::vf2d ship_pos) {
		float scale_x = float(minimap_size.x) / world_max.x;
		float scale_y = float(minimap_size.y) / world_max.y;

		// draw cargos
		for (int i = 0; i < cargos.size(); ++i) {
			if ( cargos[i].cargoType != ' ')
				FillCircle({ mm_pos.x + int(cargos[i].pos.x * scale_x), mm_pos.y + int(cargos[i].pos.y * scale_y) }, 1, olc::GREEN);
		}

		// draw dropzone
		FillCircle({ mm_pos.x + int(dropzone.x * scale_x), mm_pos.y + int(dropzone.y * scale_y) }, 1, olc::RED);

		// draw ship
		DrawCircle({ mm_pos.x + int((ship_pos.x) * scale_x), mm_pos.y + int((ship_pos.y) * scale_y) }, 2, olc::RED);

		// minimap boundary
		DrawRect({ mm_pos.x - 4,mm_pos.y - 4 }, { minimap_size.x + 3,minimap_size.y + 1 }, olc::YELLOW);
	}

#else
	void DrawGameMapOnScreen(olc::vf2d ship_pos) {
		float cx;
		float cy;
		// float sx,sy;
		// float scale_alt;
		// float scale_factor = 1.5*(max_altitude/(altitude+10));

		int offsett = 10;

		olc::Pixel col = olc::GREEN;

		for (int i = 0; i < cargos.size(); ++i) {
			cx = (cargos[i].pos.x - ship_pos.x + ship_on_screen_pos.x + 40);
			cy = (cargos[i].pos.y - ship_pos.y + ship_on_screen_pos.y + 40);

			switch (cargos[i].cargoType)
			{
			case 'd':
				col = olc::RED;
				break;
			default:
				col = olc::GREEN;
				break;
			}

			// don't draw the object if it is outside the clip radius
			if (sqrt((ship_pos.x - cargos[i].pos.x) * (ship_pos.x - cargos[i].pos.x) + (ship_pos.y - cargos[i].pos.y) * (ship_pos.y - cargos[i].pos.y)) < game_clip_objects_radius) {
				DrawCircle({ int(cx),int(cy) }, 10, col);
				ss.str(""); ss << std::setw(1) << static_cast<char>(cargos[i].cargoType);
				DrawString({ int(cx) - 3,int(cy) - 3 }, ss.str());
			}
		}
	}

	void DrawMinimap(olc::vi2d mm_pos, olc::vf2d ship_pos) {
		float scale_x = float(minimap_size.x) / world_max.x;
		float scale_y = float(minimap_size.y) / world_max.y;

		// draw cargos
		for (int i = 0; i < cargos.size(); ++i) {
			FillCircle({ mm_pos.x + 6 + int(cargos[i].pos.x * scale_x), mm_pos.y + 7 + int(cargos[i].pos.y * scale_y) }, 3, olc::GREEN);
		}

		// draw dropzone
		FillCircle({ mm_pos.x + 6 + int(dropzone.x * scale_x), mm_pos.y + 7 + int(dropzone.y * scale_y) }, 3, olc::RED);

		// draw ship
		DrawCircle({ mm_pos.x + int((ship_pos.x + 20) * scale_x), mm_pos.y + int((ship_pos.y + 20) * scale_y) }, 5, olc::RED);

		// minimap boundary
		DrawRect({ mm_pos.x,mm_pos.y }, { minimap_size.x,minimap_size.y }, olc::YELLOW);
	}

#endif



	void DrawAltitude(int x, int y) {
		int BarHeight = 100;
		int BarWidth = 20;
		float scale = max_altitude / BarHeight;
		int AltBarHeight = int(altitude / scale);

		DrawRect({ x,y }, { BarWidth, BarHeight });
		FillRect({ x + 1,(y + 100) - (AltBarHeight - 2) }, { BarWidth - 2, (AltBarHeight - 4) }, olc::RED);

		tmpstr = "Alt";
		DrawString({ 500,190 }, tmpstr, olc::GREY);
		ss.str(""); ss << std::setw(3) << int(altitude);
		tmpstr = ss.str();
		DrawString({ 500,305 }, tmpstr, olc::YELLOW);

	}

	void DrawThrottle(int x, int y) {
		// ship_avr_throttle
		
		int BarHeight = 100;
		int BarWidth = 20;

		// float scale =  BarHeight*ship_avr_throttle;
		int AltBarHeight; // = int(ship_avr_throttle / scale);

#ifdef DEBUG_PRINT
		// draw throttle
		ss.str("");
		ss << throttle1 << " " << throttle2 << " " << throttle3 << " " << throttle4;
		DrawString({ 10,240 }, ss.str());
#endif

		DrawRect({ x,y }, { BarWidth, BarHeight });
		FillRect({ x + 1,(y + 100) - int((BarHeight*ship_avr_throttle - 2)) }, { BarWidth - 2, int((BarHeight*ship_avr_throttle - 4)) }, olc::RED);

		tmpstr = "Thr";
		DrawString({ x,y-10 }, tmpstr, olc::GREY);
		ss.str(""); ss << std::setw(3) << int(ship_avr_throttle);
		tmpstr = ss.str();
		DrawString({ x,y-105 }, tmpstr, olc::YELLOW);

	}

	void DrawZVelocity(int x, int y, float ship_vel) {
		int BarHeight = 100;
		int BarWidth = 20;
		float scale = game_critical_landing_velocity / float(BarHeight);
		float trueVel = ship_velocity_z * ship_velocity_to_player_scale; // *velocity_scale;
		int AltBarHeight = int(fabs(trueVel / scale));
		olc::Pixel col = olc::GREEN;

		if (AltBarHeight > BarHeight)
			AltBarHeight = BarHeight;

		if (trueVel < game_critical_landing_velocity * velocity_alert_warning_threshhold) {
			col = olc::RED;
		}

		DrawRect({ x,y }, { BarWidth, BarHeight });
		FillRect({ x + 1,(y + 100) - (AltBarHeight - 2) }, { BarWidth - 2, (AltBarHeight - 4) }, col);

		// Velocity descent warning light
		if (timer_descent_vel_alert_active) {
			if (timer_toggle_on_state) {
				FillRect({ x - 1 * 8,y - 10 }, { 5 * 8, 10 }, olc::RED);
				sound_altitude_alert_play = true;
			}
		}

		DrawString({ x,y - 10 }, "Vel", olc::GREY);
		ss.str(""); ss << std::setw(3) << int(trueVel);
		if ( trueVel < 0)
			DrawString({ x,y + 105 }, ss.str(), olc::RED);
		else
			DrawString({ x,y + 105 }, ss.str(), olc::YELLOW);

	}

	void DrawShipAngle(olc::vi2d sos_pos, float anglx, float angly) {
		float y1 = (sos_pos.y + 37) + (angly * 60);
		float x2 = (sos_pos.x + 37) + (anglx * 60);
		DrawLine({ sos_pos.x - 35,int(y1) }, { sos_pos.x + 110,int(y1) }, olc::GREEN);
		DrawLine({ int(x2),sos_pos.y - 35 }, { int(x2),sos_pos.y + 110 }, olc::GREEN);

	}

#define NEWFUNC_DrawShipOnScreen
#ifdef NEWFUNC_DrawShipOnScreen


	// proppellar size is the named size of the craft
	void DrawShipOnScreen(olc::vi2d sos_pos, int ship_min_size, int ship_max_size, float altitude) {
		float altscale = altitude / max_altitude;
		float ship_size = ship_min_size + ship_max_size * altscale; // scale this between min and max relative to altitude ...
		float engineOffset = ship_size * 2.5f;
		float offset = engineOffset / 2;

		olc::vf2d center_offset = sos_pos - olc::vf2d{ offset, offset };

		DrawCircle(center_offset, int(ship_size)); // engine 1
		FillCircle(center_offset, int(throttle1 * ship_size), olc::RED);  // power level

		DrawCircle({ int(center_offset.x),int(center_offset.y + engineOffset) }, int(ship_size)); // engine 2
		FillCircle({ int(center_offset.x),int(center_offset.y + engineOffset) }, int(throttle2 * ship_size), olc::RED); // engine 2

		DrawCircle({ int(center_offset.x + engineOffset),int(center_offset.y + engineOffset) }, int(ship_size)); // engine 3
		FillCircle({ int(center_offset.x + engineOffset),int(center_offset.y + engineOffset) }, int(throttle3 * ship_size), olc::RED); // engine 3

		DrawCircle({ int(center_offset.x + engineOffset),int(center_offset.y) }, int(ship_size)); // engine 4
		FillCircle({ int(center_offset.x + engineOffset),int(center_offset.y) }, int(throttle4 * ship_size), olc::RED); // engine 4

		// cargo bay
		float bay_min_size = 10;
		float bay_max_size = 20;
		int bay_size = int(bay_min_size + bay_max_size * altscale);
		// DrawRect(sos_pos-olc::vf2d{5,5}, {10,10}, olc::RED);
		DrawRect(sos_pos - olc::vf2d{ float(bay_size / 2),float(bay_size / 2) }, { bay_size,bay_size }, olc::RED);

	}
#else

	void DrawShipOnScreen(olc::vi2d sos_pos, int engineSize) {
		float engineOffset = engineSize * 2.5;
		DrawCircle(sos_pos, engineSize); // engine 1
		FillCircle(sos_pos, int(throttle[0] * engineSize), olc::RED);  // power level

		DrawCircle({ sos_pos.x,int(sos_pos.y + engineOffset) }, engineSize); // engine 2
		FillCircle({ sos_pos.x,int(sos_pos.y + engineOffset) }, int(throttle[1] * engineSize), olc::RED); // engine 2

		DrawCircle({ int(sos_pos.x + engineOffset),int(sos_pos.y + engineOffset) }, engineSize); // engine 3
		FillCircle({ int(sos_pos.x + engineOffset),int(sos_pos.y + engineOffset) }, int(throttle[2] * engineSize), olc::RED); // engine 3

		DrawCircle({ int(sos_pos.x + engineOffset),sos_pos.y }, engineSize); // engine 4
		FillCircle({ int(sos_pos.x + engineOffset),sos_pos.y }, int(throttle[3] * engineSize), olc::RED); // engine 4
	}
#endif



};


int main()
{
	Game hover;

	// 640x360 with no full screen and sync to monitor refresh rate
	if (hover.Construct(640, 360, 2, 2, false,true)) 
		hover.Start();

	return 0;
}
