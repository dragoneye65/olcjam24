#include "pge.h"

// #define DEBUG_PRINT

class Game : public olc::PixelGameEngine
{
public:
	Game()
	{
		sAppName = "Hover";
	}

	// first char based game map, don't need it, can generate it
	std::string game_map;
	olc::vi2d charmap_dim = { 16, 10 };

	enum state { INTRO, GAMEON, THEEND, GAMEWON };
	enum state game_state = state::INTRO;
	float game_critical_landing_velocity = -100.0f;
	float game_object_proximity_limit = 10.0f;
	float game_clip_objects_radius = 150.0f;

	olc::vi2d mouse_pos, mouse_pos_old;

	olc::vi2d instructions_pos = { 50, 50 };
	int player_points = 0;
	int player_deliveries = 0;

	// ship, 4 engines 
	float throttle[4] = { 0.0f, 0.0f, 0.0f, 0.0f };				// 0-100% [0.0 - 1.0]
	float ship_net_weight = 900.0f;
	float ship_weight = 900.0f;								// grams
	float ship_velocity[3] = { 0.0f, 0.0f, 0.0f };				// x,y,z
	olc::vf2d ship_cap_vel_xy = { 100.0f, 100.0f };
	float ship_cap_vel_z = 300.0f;
	float last_velocity_before_crashlanding = 0.0f;			// checking for pancake 
	float ship_response = 1.6f;								// respons factor on throttle
	float ship_avr_throttle = 0.0f;
	float ship_idle_throttle = 0.01f;
	float ship_max_thrust = 4000.0f;							// grams
	float ship_max_angle = 0.7853981634f; // M_PI_4;							// pi/4 = 45 deg
	float pi_2 = 1.57079632679f;
	bool ship_crashed = false;
	int ship_docket_at_cargoType = 0;

	// engine quad layout
	//   1     4
	//      X
	//   2     3

	float ship_angle_x = 0.0;
	float ship_angle_y = 0.0;
	olc::vf2d ship_pos;
	float velocity_scale = 5000.0;

	// world
	float gravity = 9.81f;									//  9.81 m/s^2
	float altitude = 0.0f;									// 0m  sea level
	float max_altitude = 100.0f;
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

		// *: starting position
		// u: unloading pad
		// 0-9: cargo type, 
		//	  0:stationary (usually just bomb it)
		//  1-5: moveable  (pic up and drop these different cargo types)
		//  6-9: running   (now we are talking, pick up or bomb those running suckers)
		game_map  = "9              9";
		game_map += "     0   0      ";
		game_map += "  5         3   ";
		game_map += "       2        ";
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


		// cargo_it = cargos.begin();

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


		TimerUpdateTrigger(fElapsedTime, 200);
		/*
				if (timer_toggle_on_state) {
					timer_descent_vel_alert_active = true;
				} else {
					timer_descent_vel_alert_active = false;
				}
		*/

		Clear(olc::VERY_DARK_BLUE);


#ifdef DEBUG_PRINT
		// show mouse cursor + position
		DrawLine(mouse_pos - olc::vi2d{ 5,5 }, mouse_pos + olc::vi2d{ 5,5 }, olc::RED);
		DrawLine(mouse_pos - olc::vi2d{ -5,5 }, mouse_pos + olc::vi2d{ -5,5 }, olc::RED);
		ss.str(""); ss << "(" << mouse_pos.x << "," << mouse_pos.y << ")";
		DrawString(mouse_pos, ss.str(), olc::RED);
#endif


		if (game_state == state::GAMEON) {

			// #ifdef DEBUG_PRINT
						// draw velocity
			int vel_pos_x = ship_on_screen_pos.x - 50;
			int vel_pos_y = ship_on_screen_pos.y + 110;
			tmpstr = ""; int offx = 0;
			for (int i = 0; i < 3; i++) {
				ss.str("");
				ss << ship_velocity[i] * velocity_scale; 	// todo: get rid of this velocity_scale
				if (i == 0) { tmpstr += " { " + ss.str(); offx = 0; }
				else { tmpstr = ss.str(); offx = 20; }
				if (i != 2) tmpstr += ", "; else tmpstr += " }";
				DrawString({ vel_pos_x + i * 55 + offx,vel_pos_y }, tmpstr, olc::YELLOW);
			}
			// #endif

			// calculate angle from thrust differentials (simple version) max angle = 45 deg
			ship_angle_x = pi_2 * (((throttle[0] + throttle[1]) - (throttle[2] + throttle[3])) / 2);
			ship_angle_y = pi_2 * ((throttle[0] + throttle[3]) - (throttle[1] + throttle[2])) / 2;

			// power to all engines
			if (GetKey(olc::Key::UP).bHeld) {
				for (int i = 0; i < 4; i++) {
					throttle[i] += fElapsedTime * ship_response;
					if (throttle[i] > 1.0) throttle[i] = 1.0;
				}
			}

			if (GetKey(olc::Key::DOWN).bHeld) {
				for (int i = 0; i < 4; i++) {
					throttle[i] -= fElapsedTime * ship_response;
					if (throttle[i] < ship_idle_throttle) throttle[i] = ship_idle_throttle;
				}
			}

			// roll left
			// Increse 3 and 4
			// decrese 1 and 2
			int mouse_deadzone = 50;
			if (GetKey(olc::Key::A).bHeld) {
				throttle[0] -= fElapsedTime * ship_response;
				throttle[1] -= fElapsedTime * ship_response;
				throttle[2] += fElapsedTime * ship_response;
				throttle[3] += fElapsedTime * ship_response;
				for (int i = 0; i < 4; i++) {
					if (throttle[i] < ship_idle_throttle) throttle[i] = ship_idle_throttle;
					if (throttle[i] > 1.0) throttle[i] = 1.0;
				}
			}
			// roll right
			// Increse 1 and 2
			// decrese 3 and 4
			if (GetKey(olc::Key::D).bHeld) {
				throttle[0] += fElapsedTime * ship_response;
				throttle[1] += fElapsedTime * ship_response;
				throttle[2] -= fElapsedTime * ship_response;
				throttle[3] -= fElapsedTime * ship_response;
				for (int i = 0; i < 4; i++) {
					if (throttle[i] < ship_idle_throttle) throttle[i] = ship_idle_throttle;
					if (throttle[i] > 1.0) throttle[i] = 1.0;
				}
			}
			// pitch forward
			// Increse 2 and 3
			// decrese 1 and 4
			if (GetKey(olc::Key::W).bHeld) {
				throttle[0] -= fElapsedTime * ship_response;
				throttle[1] += fElapsedTime * ship_response;
				throttle[2] += fElapsedTime * ship_response;
				throttle[3] -= fElapsedTime * ship_response;
				for (int i = 0; i < 4; i++) {
					if (throttle[i] < ship_idle_throttle) throttle[i] = ship_idle_throttle;
					if (throttle[i] > 1.0) throttle[i] = 1.0;
				}
			}
			// pitch back
			// Increse 1 and 4
			// decrese 2 and 3
			if (GetKey(olc::Key::S).bHeld) {
				throttle[0] += fElapsedTime * ship_response;
				throttle[1] -= fElapsedTime * ship_response;
				throttle[2] -= fElapsedTime * ship_response;
				throttle[3] += fElapsedTime * ship_response;
				for (int i = 0; i < 4; i++) {
					if (throttle[i] < ship_idle_throttle) throttle[i] = ship_idle_throttle;
					if (throttle[i] > 1.0) throttle[i] = 1.0;
				}
			}

			if (GetKey(olc::Key::SPACE).bPressed) {
				float avr = 0.0;
				for (int i = 0; i < 4; i++)
					avr += throttle[i];
				for (int i = 0; i < 4; i++)
					throttle[i] = avr / 4;
			}



			// calculate throttle average from engines
			ship_avr_throttle = (throttle[0] + throttle[1] + throttle[2] + throttle[3]) / 4;
			// thrust
			ship_velocity[2] += fElapsedTime * 0.000005f * (ship_max_thrust)*cos(ship_angle_x) * cos(ship_angle_y) * ship_avr_throttle;
			// Gravity
			ship_velocity[2] -= fElapsedTime * 0.000001f * ship_weight * gravity;

			if (game_state == state::GAMEON)
				last_velocity_before_crashlanding = ship_velocity[2] * velocity_scale;    // for checking if you crashed hard into ground

			ship_velocity[0] += fElapsedTime * 0.006f * sin(ship_angle_x);
			ship_velocity[1] += fElapsedTime * 0.006f * sin(ship_angle_y);

#ifdef DEBUG_PRINT
			ss.str("");	ss << ship_velocity[0] * velocity_scale << ", " << ship_angle_x;
			DrawString({ 100,100 }, ss.str());
			ss.str("");	ss << ship_velocity[1] * velocity_scale << ", " << ship_angle_y;
			DrawString({ 100,110 }, ss.str());
#endif

			altitude += ship_velocity[2];

			// cap ship velocity in xy
			if ((ship_velocity[0] * velocity_scale) > ship_cap_vel_xy.x) 	ship_velocity[0] = ship_cap_vel_xy.x / velocity_scale;
			if ((ship_velocity[0] * velocity_scale) < -ship_cap_vel_xy.x)	ship_velocity[0] = -(ship_cap_vel_xy.x / velocity_scale);
			if ((ship_velocity[1] * velocity_scale) > ship_cap_vel_xy.y) 	ship_velocity[1] = ship_cap_vel_xy.y / velocity_scale;
			if ((ship_velocity[1] * velocity_scale) < -ship_cap_vel_xy.y)	ship_velocity[1] = -ship_cap_vel_xy.y / velocity_scale;
			
			ship_pos.x += ship_velocity[0];
			ship_pos.y += ship_velocity[1];

			// limit the ship inside the map area , bounch back
			if (ship_pos.x < 0) { ship_pos.x = 0.0; ship_velocity[0] *= -1.0; }
			if (ship_pos.x > 500) { ship_pos.x = 500.0; ship_velocity[0] *= -1.0; }
			if (ship_pos.y < 0) { ship_pos.y = 0.0; ship_velocity[1] *= -1.0; }
			if (ship_pos.y > 500) { ship_pos.y = 500.0; ship_velocity[1] *= -1.0; }

#ifdef DEBUG_PRINT
			// Show the position to the ship under minimap
			ss.str(""); ss << ship_pos.x; tmpstr = ss.str();
			DrawString({ minimap_position.x + 20,minimap_position.y + minimap_size.y + 2 }, tmpstr, olc::YELLOW);
			ss.str(""); ss << ship_pos.y; tmpstr = ss.str();
			DrawString({ minimap_position.x + 20,minimap_position.y + minimap_size.y + 12 }, tmpstr, olc::YELLOW);
#endif

			// Altitude check limits
			if (altitude > max_altitude) {
				altitude = max_altitude;
				ship_velocity[2] = 0.0; // z
			}
			if (altitude < 0.0) {
				altitude = 0.0;
				ship_velocity[0] = 0.0; // x
				ship_velocity[1] = 0.0; // y
				ship_velocity[2] = 0.0; // z
			}

			// Scale the ship size relative to altitude
			// Draw ship on screen and scale it relative to altitude
			DrawShipOnScreen(ship_on_screen_pos, 10, 20, altitude); // x,y,engine minsize, maxsize, altitude

#ifdef DEBUG_PRINT
			// show ship real position
			ss.str("");
			ss << "(" << ship_on_screen_pos.x << "," << ship_on_screen_pos.y << ")";
			DrawString({ 0,0 }, ss.str(), olc::YELLOW);
#endif

			DrawAltitude(500, 200);
			DrawZVelocity(530, 200, ship_velocity[2] * velocity_scale);
			//	DrawShipAngle( ship_on_screen_pos, ship_angle_x, ship_angle_y);
			DrawMinimap(minimap_position, ship_pos);
			DrawGameMapOnScreen(ship_pos);

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
			tmpstr = "Cargo Weight: ";
			DrawString({ 100,20 }, tmpstr, olc::GREEN);
			ss.str(""); ss << std::setw(5) << cargo_weight;
			DrawString({ 100 + 15 * 8,20 }, ss.str(), olc::RED);


			tmpstr = "Score";
			DrawString({ 10,10 }, tmpstr, olc::GREEN);
			ss.str(""); ss << std::setw(5) << int(player_points);
			DrawString({ 10,18 }, ss.str(), olc::RED);

			timer_descent_vel_alert_active = false;
			// show alert if decending dangerously fast
			if (int(altitude) != 0 && ship_velocity[2] * velocity_scale < (game_critical_landing_velocity + 40.0f)) {
				timer_descent_vel_alert_active = true;
			}

			// check z velocity on "landing"
			if (int(altitude) == 0 && last_velocity_before_crashlanding < game_critical_landing_velocity) {
				game_state = state::THEEND;
				ship_crashed = true;
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
			last_velocity_before_crashlanding = 0.0;
			player_points = 0;
			for (int i = 0; i < 3; i++) {
				ship_velocity[i] = 0.0;
				throttle[i] = 0.0;
			}

			Instructions(instructions_pos);
			if (GetKey(olc::Key::ENTER).bPressed || GetKey(olc::Key::SPACE).bPressed) {
				game_state = state::GAMEON;
			}
		}

		// Game ended, or user aborted
		if (game_state == state::THEEND) {
			EndGame();

			// SPACE: continue if not crashed, restarts if crashed
			if (GetKey(olc::Key::SPACE).bReleased) {
				if (!ship_crashed)
					game_state = state::GAMEON;
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
			if (GetKey(olc::Key::C).bReleased) ship_crashed = !ship_crashed;


		// Escape to THEEND, or quit if pressed while in THEEND state
		if (GetKey(olc::Key::ESCAPE).bPressed) {
			if (game_state == state::THEEND) {
				return false;
			}
			else {
				game_state = state::THEEND;
			}
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




	void RestartGame() {
		game_state = state::INTRO;
		InitGameMap();
		ship_pos = startpos;
	}

	void Instructions(olc::vi2d pos) {
		int offsy = 10;
		FillRect({ pos.x - 1,  pos.y - 1 }, { 500 + 2, 200 + 2 }, olc::VERY_DARK_GREY);
		DrawRect({ pos.x, pos.y }, { 500, 200 }, olc::GREEN);

		ss << std::fixed << std::setprecision(2);
		DrawString({ ScreenWidth() / 2 - 10 * 16 / 2,24 }, "Hover run", olc::YELLOW, 2);

		DrawString({ pos.x + 10, pos.y + offsy * 1 }, "     As a drone pilot I just had to make this game.        ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * 2 }, "                If you land hard you crash!                ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * 3 }, " The game is played by watching the altitude carefully     ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * 4 }, " while running your missions gathering cargo by landing    ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * 5 }, " softly on top of them,you can pick up as many as you like ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * 6 }, "    when you feel like it just land on the red dropzone    ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * 7 }, "          and it will offload automagically.               ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * 8 }, " Be aware that when you roll and pitch you loose altitude. ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * 9 }, "    You will have to increse throttle to stay airborn.     ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * 10 }, "The four engines power-level shows with red filled circles ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * 12 }, "          Use the ADWS, SPACE, UP, DOWN.  keys to play     ", olc::DARK_GREEN);
		DrawString({ pos.x + 10, pos.y + offsy * 14 }, "               Have fun!  Regards DragonEye.               ", olc::DARK_GREEN);
	}



	void EndGame() {
		int offsy = 10;
		int offsx = ScreenWidth() / 4;
		int asdf = ScreenHeight() / 4;
		FillRect({ offsx - 1, asdf - 1 }, { 300 + 2, 200 + 2 }, olc::VERY_DARK_GREY);
		DrawRect({ offsx, asdf }, { 300, 200 }, olc::RED);

		//		if (last_velocity_before_crashlanding <= game_critical_landing_velocity) {
		if (ship_crashed) {
			DrawString({ offsx + 10, asdf + offsy * 1 }, "Oh holy pancake...", olc::GREY);
			DrawString({ offsx + 10, asdf + offsy * 5 }, "What a spectacular crash!", olc::GREY);
			DrawString({ offsx + 10, asdf + offsy * 6 }, "Groundbreaking velocity:", olc::GREY);
			ss.str(""); ss << last_velocity_before_crashlanding;
			DrawString({ offsx + 10 + 28 * 8, asdf + offsy * 6 }, ss.str(), olc::RED);
			DrawString({ offsx + 10, asdf + offsy * 18 }, "SPACE/ENTER to restart, ESC to quit", olc::RED);
		}
		else {
			DrawString({ offsx + 10, asdf + offsy * 1 }, "        User aborted!", olc::GREY);
			DrawString({ offsx + 10, asdf + offsy * 7 }, "I am sorry to see you go...", olc::GREY);
			DrawString({ offsx + 10, asdf + offsy * 8 }, "  Hope you had fun! L8r o7 ", olc::GREY);
			DrawString({ offsx + 10, asdf + offsy * 16 }, " SPACE (Continue)", olc::GREEN);
			DrawString({ offsx + 10, asdf + offsy * 17 }, " ENTER (Restart)", olc::YELLOW);
			DrawString({ offsx + 10, asdf + offsy * 18 }, " ESC   (quit)", olc::RED);
		}
		DrawString({ offsx + 10, asdf + offsy * 3 }, "Score: ", olc::GREY);
		ss.str(""); ss << std::setw(0) << player_points;
		DrawString({ offsx + 10 + 9 * 8, asdf + offsy * 3 }, ss.str(), olc::GREEN);

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
		DrawString({ offsx + 10, asdf + offsy * 15 }, "           Esc to quit", olc::RED);

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
							cargos.erase(cargos.begin() + i);  // off the map with it
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
			FillCircle({ mm_pos.x + int(cargos[i].pos.x * scale_x), mm_pos.y + int(cargos[i].pos.y * scale_y) }, 2, olc::GREEN);
		}

		// draw dropzone
		FillCircle({ mm_pos.x + int(dropzone.x * scale_x), mm_pos.y + int(dropzone.y * scale_y) }, 2, olc::RED);

		// draw ship
		DrawCircle({ mm_pos.x + int((ship_pos.x) * scale_x), mm_pos.y + int((ship_pos.y) * scale_y) }, 3, olc::RED);

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

	void DrawZVelocity(int x, int y, float ship_vel) {
		int BarHeight = 100;
		int BarWidth = 20;
		float scale = game_critical_landing_velocity / float(BarHeight);
		float trueVel = ship_velocity[2] * velocity_scale;
		int AltBarHeight = int(fabs(trueVel / scale));
		olc::Pixel col = olc::GREEN;

		if (AltBarHeight > BarHeight)
			AltBarHeight = BarHeight;

		if (trueVel < game_critical_landing_velocity * 0.6f)
			col = olc::RED;

		DrawRect({ x,y }, { BarWidth, BarHeight });
		FillRect({ x + 1,(y + 100) - (AltBarHeight - 2) }, { BarWidth - 2, (AltBarHeight - 4) }, col);

		// Velocity descent warning light
		if (timer_descent_vel_alert_active) {
			if (timer_toggle_on_state) {
				FillRect({ x - 1 * 8,y - 10 }, { 5 * 8, 10 }, olc::RED);
			}
		}
		tmpstr = "Vel";
		DrawString({ x,y - 10 }, tmpstr, olc::GREY);
		ss.str(""); ss << std::setw(3) << int(trueVel);
		tmpstr = ss.str();
		DrawString({ x,y + 105 }, tmpstr, olc::YELLOW);

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
		FillCircle(center_offset, int(throttle[0] * ship_size), olc::RED);  // power level

		DrawCircle({ int(center_offset.x),int(center_offset.y + engineOffset) }, int(ship_size)); // engine 2
		FillCircle({ int(center_offset.x),int(center_offset.y + engineOffset) }, int(throttle[1] * ship_size), olc::RED); // engine 2

		DrawCircle({ int(center_offset.x + engineOffset),int(center_offset.y + engineOffset) }, int(ship_size)); // engine 3
		FillCircle({ int(center_offset.x + engineOffset),int(center_offset.y + engineOffset) }, int(throttle[2] * ship_size), olc::RED); // engine 3

		DrawCircle({ int(center_offset.x + engineOffset),int(center_offset.y) }, int(ship_size)); // engine 4
		FillCircle({ int(center_offset.x + engineOffset),int(center_offset.y) }, int(throttle[3] * ship_size), olc::RED); // engine 4

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
	if (hover.Construct(640, 360, 2, 2, false)) // 640x360
		hover.Start();

	return 0;
}
