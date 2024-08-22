#include "pge.h"


// *: starting position
// u: unloading pad
// 0-9: cargo type, 
//	  0:stationary (usually just bomb it)
//  1-5: moveable  (pic up and drop these different cargo types)
//  6-9: running   (now we are talking, pick up or bomb those running suckers)
char *game_map[] = {
    "9              9",
    "     0   0      ",
    "  5         3   ",
    "       2        ",
    "   d  2*2  1    ",
    "       2        ",
    "    0    6      ",
    "                ",
	"     2      8   ",
    "9      3       9",
    nullptr
};

class Game : public olc::PixelGameEngine
{
public:
	Game()
	{
		sAppName = "Hover";
	}

	enum state { INTRO, GAMEON, THEEND };
	enum state game_state = state::INTRO;
	float game_critical_landing_velocity = -100;
	float game_object_proximity_limit = 10.0;
	float game_clip_objects_radius = 150;

	olc::vi2d mouse_pos,mouse_pos_old;
	olc::vi2d charmap_dim; 

	olc::vi2d instructions_pos = { 50, 50 };
	int player_points = 0;
	int player_deliveries = 0;

	// ship, 4 engines 
	float throttle[4] = {0.0, 0.0, 0.0, 0.0 };				// 0-100% [0.0 - 1.0]
	float ship_weight = 900.0;								// grams
	float ship_velocity[3] = { 0.0, 0.0, 0.0 };				// x,y,z
	float last_velocity_before_crashlanding = 0.0;			// checking for pancake 
	float ship_response = 0.6;
	float ship_avr_throttle = 0.0;
	float ship_idle_throttle = 0.01;
	float ship_max_thrust = 4000.0;							// grams
	float ship_max_angle = M_PI_4;							// pi/4 = 45 deg
	bool ship_crashed = false;

	// engine quad layout
	//   1     4
	//      X
	//   2     3

	float ship_angle_x = 0.0;
	float ship_angle_y = 0.0;
	olc::vf2d ship_cap_vel_xy =  { 60.0, 60.0};
	olc::vf2d ship_pos;
	float velocity_scale = 5000.0;

	// world
	float gravity = 9.81;									//  9.81 m/s^2
	float altitude = 0.0;									// 0m  sea level
	float max_altitude = 100.0;
	olc::vf2d world_max =  {500.0,500.0};

	olc::vi2d ship_on_screen_pos;

	// minimap
	olc::vi2d minimap_size = { 150, 150 };
	olc::vi2d minimap_position = { 0 , 0 };
	struct cargo {
		olc::vf2d pos;			// position in world space
		int cargoType;			// type of cargo: 
	};

	olc::vf2d startpos;			// doh
	olc::vf2d dropzone;  		// dropzone
	std::vector<cargo> cargos;	// yea you guessed it

	int ship_inventory_capasity = 3;
	std::vector<cargo> inventory;
	olc::vi2d inventory_pos = { 10, 50 };

	std::string tmpstr;
	std::stringstream ss;

public:

	olc::vi2d GetCharMapDimentions() {
		int r,c;

		for ( r = 0; game_map[r] != nullptr; r++) 
			for ( c = 0; game_map[r][c] != '\0'; c++);

		return {c,r};
	}

	// scan through the map and place the objects
	void InitGameMap() {
		float sx = world_max.x/charmap_dim.x;
		float sy = world_max.y/charmap_dim.y;

		// wipe inventory
		inventory.clear();
		cargos.clear();

		// scan the rows
		char ch;
		for ( int r = 0; game_map[r] != nullptr; r++) {
			for ( int c = 0; game_map[r][c] != '\0'; c++) {
				switch (ch = game_map[r][c])
				{
				case '*':	// startingpos
					startpos = {float(c*sx),float(r*sy)};
					cargos.push_back({startpos,ch});
					break;
				case 'd':	// dropzone
					dropzone = {float(c*sx),float(r*sy)};
					cargos.push_back({dropzone,ch});
					break;
				default:
					if(isdigit(ch)) {

						cargos.push_back({{float(c)*sx,float(r)*sy},ch});
					}
					break;
				}
			}
		}
	}

	void DrawCargoPos(olc::vi2d txtpos, olc::vf2d p) {
		tmpstr = "CargoPos ";
		DrawString( txtpos, tmpstr);
		ss.str("");	ss << p.x; tmpstr = ss.str(); DrawString( {txtpos.x+100, txtpos.y}, tmpstr, olc::YELLOW);
		ss.str("");	ss << p.y; tmpstr = ss.str(); DrawString( {txtpos.x+160, txtpos.y}, tmpstr, olc::YELLOW);
	}

	bool OnUserCreate() override
	{
		// Called once at the start, so create things here
		charmap_dim = GetCharMapDimentions();

		ship_on_screen_pos = { ScreenWidth()/3,ScreenHeight()/3};
		minimap_position = {ScreenWidth()-minimap_size.x-1, 1};

		InitGameMap();
		ship_pos = startpos;
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		// called once per frame
		mouse_pos_old = mouse_pos;
		mouse_pos = GetMousePos();

		Clear(olc::VERY_DARK_BLUE);

		if ( game_state == state::INTRO) {
			ss << std::fixed << std::setprecision(2);
			DrawString({ ScreenWidth()/2-10*16/2,24},"Hover run",olc::GREY, 2);
		}
		
		if ( game_state == state::GAMEON) {
			// draw velocity
			int vel_pos_x = ship_on_screen_pos.x - 50;
			int vel_pos_y = ship_on_screen_pos.y + 110;
			tmpstr = ""; int offx = 0;
			for ( int i = 0; i < 3; i++) {
				ss.str("");
				ss << ship_velocity[ i]*velocity_scale; 	// todo: get rid of this velocity_scale
				if ( i == 0 ) { tmpstr += " { " + ss.str(); offx =  0;  } else { tmpstr = ss.str(); offx = 20;}
				if ( i != 2 ) tmpstr += ", "; else tmpstr += " }";
				DrawString( { vel_pos_x + i * 55 + offx,vel_pos_y}, tmpstr, olc::YELLOW); 
			}

			tmpstr = "Alt";
			DrawString({500,190},tmpstr,olc::GREY); 
			ss.str(""); ss << std::setw(3) << int(altitude);
			tmpstr = ss.str();
			DrawString({500,305},tmpstr,olc::YELLOW); 

			// calculate angle from thrust differentials (simple version) max angle = 45 deg
			ship_angle_x = M_PI_2 * (((throttle[0]+throttle[1] ) - (throttle[2]+throttle[3] ))/2); 
			ship_angle_y = M_PI_2 * ((throttle[0]+throttle[3] ) - (throttle[1]+throttle[2] ))/2;

			// power to all engines
			if ( GetKey(olc::Key::UP).bHeld) {
				for ( int i = 0; i < 4; i++) {
					throttle[i] += fElapsedTime * ship_response;
					if ( throttle[i] > 1.0) throttle[i] = 1.0;
				}
			}

			if ( GetKey(olc::Key::DOWN).bHeld) {
				for ( int i = 0; i < 4; i++) {
					throttle[i] -= fElapsedTime * ship_response;
					if ( throttle[i] < ship_idle_throttle) throttle[i] = ship_idle_throttle;
				}
			}

			// roll left
			// Increse 3 and 4
			// decrese 1 and 2
			int mouse_deadzone = 50;
			if ( GetKey(olc::Key::A).bHeld ) {
				throttle[0] -= fElapsedTime * ship_response;
				throttle[1] -= fElapsedTime * ship_response;
				throttle[2] += fElapsedTime * ship_response;
				throttle[3] += fElapsedTime * ship_response;
				for ( int i = 0; i < 4; i++) {
					if ( throttle[i] < ship_idle_throttle) throttle[i] = ship_idle_throttle;
					if ( throttle[i] > 1.0) throttle[i] = 1.0;
				}
			}
			// roll right
			// Increse 1 and 2
			// decrese 3 and 4
			if ( GetKey(olc::Key::D).bHeld ) {
				throttle[0] += fElapsedTime * ship_response;
				throttle[1] += fElapsedTime * ship_response;
				throttle[2] -= fElapsedTime * ship_response;
				throttle[3] -= fElapsedTime * ship_response;
				for ( int i = 0; i < 4; i++) {
					if ( throttle[i] < ship_idle_throttle) throttle[i] = ship_idle_throttle;
					if ( throttle[i] > 1.0) throttle[i] = 1.0;
				}
			}
			// pitch forward
			// Increse 2 and 3
			// decrese 1 and 4
			if ( GetKey(olc::Key::W).bHeld) {
				throttle[0] -= fElapsedTime * ship_response;
				throttle[1] += fElapsedTime * ship_response;
				throttle[2] += fElapsedTime * ship_response;
				throttle[3] -= fElapsedTime * ship_response;
				for ( int i = 0; i < 4; i++) {
					if ( throttle[i] < ship_idle_throttle) throttle[i] = ship_idle_throttle;
					if ( throttle[i] > 1.0) throttle[i] = 1.0;
				}
			}
			// pitch back
			// Increse 1 and 4
			// decrese 2 and 3
			if ( GetKey(olc::Key::S).bHeld) {
				throttle[0] += fElapsedTime * ship_response;
				throttle[1] -= fElapsedTime * ship_response;
				throttle[2] -= fElapsedTime * ship_response;
				throttle[3] += fElapsedTime * ship_response;
				for ( int i = 0; i < 4; i++) {
					if ( throttle[i] < ship_idle_throttle) throttle[i] = ship_idle_throttle;
					if ( throttle[i] > 1.0) throttle[i] = 1.0;
				}
			}

			if ( GetKey(olc::Key::SPACE).bPressed) {
				float avr = 0.0;
				for ( int i = 0; i < 4; i++) 
					avr += throttle[i];
				for ( int i = 0; i < 4; i++) 
					throttle[i] = avr/4;
			}



			// calculate throttle average from engines
			ship_avr_throttle = (throttle[0] + throttle[1] + throttle[2] + throttle[3]) / 4 ;
			// thrust
			ship_velocity[2] += fElapsedTime * 0.000005* (ship_max_thrust) * cos(ship_angle_x)*cos(ship_angle_y)*ship_avr_throttle;
			// Gravity
			ship_velocity[2] -= fElapsedTime * 0.000001 * ship_weight * gravity;

			if ( game_state == state::GAMEON)
				last_velocity_before_crashlanding = ship_velocity[2]*velocity_scale;    // for checking if you crashed hard into ground

			ship_velocity[0] += fElapsedTime * 0.006 * sin(ship_angle_x);
			ship_velocity[1] += fElapsedTime * 0.006 * sin(ship_angle_y);

			altitude += ship_velocity[2];

			// cap ship velocity in xy
			if ((ship_velocity[0]*velocity_scale) > ship_cap_vel_xy.x) 	ship_velocity[0] = ship_cap_vel_xy.x/velocity_scale;
			if ((ship_velocity[0]*velocity_scale) < -ship_cap_vel_xy.x)	ship_velocity[0] = -(ship_cap_vel_xy.x/velocity_scale);
			if ((ship_velocity[1]*velocity_scale) > ship_cap_vel_xy.y) 	ship_velocity[1] = ship_cap_vel_xy.y/velocity_scale;
			if ((ship_velocity[1]*velocity_scale) < -ship_cap_vel_xy.y)	ship_velocity[1] = -ship_cap_vel_xy.y/velocity_scale;

			ship_pos.x += ship_velocity[0];
			ship_pos.y += ship_velocity[1];


			// limit the ship inside the map area , bounch back
			if ( ship_pos.x < 0) { ship_pos.x = 0.0; ship_velocity[0] *= -1.0; }
			if ( ship_pos.x > 500-40) { ship_pos.x = 500.0-40; ship_velocity[0] *= -1.0;}
			if ( ship_pos.y < 0) { ship_pos.y = 0.0; ship_velocity[1] *= -1.0;}
			if ( ship_pos.y > 500-40) { ship_pos.y = 500.0-40; ship_velocity[1] *= -1.0;}

			// Show the position to the ship under minimap
			ss.str(""); ss << ship_pos.x; tmpstr = ss.str();
			DrawString({minimap_position.x+20,minimap_position.y+minimap_size.y+2},tmpstr,olc::YELLOW); 
			ss.str(""); ss << ship_pos.y; tmpstr = ss.str();
			DrawString({minimap_position.x+80,minimap_position.y+minimap_size.y+2},tmpstr,olc::YELLOW); 

			// Altitude check limits
			if ( altitude > max_altitude) {
				altitude = max_altitude; 
				ship_velocity[2] = 0.0; // z
				}
			if ( altitude < 0.0) {
				altitude = 0.0; 
				ship_velocity[0] = 0.0; // x
				ship_velocity[1] = 0.0; // y
				ship_velocity[2] = 0.0; // z
				}

			DrawShipOnScreen( ship_on_screen_pos, 30); // x,y,engine size

			DrawAltitude( 500, 200);
			DrawShipAngle( ship_on_screen_pos, ship_angle_x, ship_angle_y);
			DrawMinimap( minimap_position, ship_pos);
			DrawGameMapOnScreen( ship_pos );
			CheckDropPickupOnLanding();
			ShowInventory( inventory_pos);
			tmpstr = "Score: ";
			DrawString({ 10,10},tmpstr,olc::GREEN); 
			ss.str(""); ss << std::setw(4) << int(player_points);
			tmpstr = ss.str();
			DrawString({110,10},tmpstr,olc::RED);

			// check z velocity on "landing"
			if ( int(altitude) == 0 && last_velocity_before_crashlanding < game_critical_landing_velocity) {
				game_state = state::THEEND;
				ship_crashed = true;
			}

			if (ship_crashed) 
				game_state = state::THEEND;
		}

		if ( game_state == state::INTRO) {
			player_deliveries = 0;
			ship_crashed = false;
			last_velocity_before_crashlanding = 0.0;
			player_points = 0;
			for ( int i = 0; i < 4; i++) {
				ship_velocity[i] = 0.0;
				throttle[i] = 0.0;
			}

			Instructions( instructions_pos);
			if ( GetKey(olc::Key::ENTER).bReleased || GetKey(olc::Key::SPACE).bReleased)
				game_state = state::GAMEON;
		}

		if ( game_state == state::THEEND) {
			EndGame();

			if ( GetKey(olc::Key::SPACE).bReleased || GetKey(olc::Key::ENTER).bReleased) {
				game_state = state::INTRO;
				InitGameMap();
				ship_pos = startpos;
			}
		}

		if ( GetKey(olc::Key::ESCAPE).bPressed) {
			if ( game_state == state::THEEND) {
				return false;
			}
			else {
				game_state = state::THEEND;
			}
		}
		return true;
	}

	void Instructions( olc::vi2d pos) {
		int offsy = 10;
		FillRect({pos.x-1,  pos.y-1}, {500+2, 200+2}, olc::VERY_DARK_GREY);
		DrawRect({pos.x, pos.y}, {500, 200}, olc::GREEN);
		DrawString( {pos.x+10, pos.y+offsy*1},  "     As a drone pilot I just had to make this game.        ", olc::DARK_GREEN);
		DrawString( {pos.x+10, pos.y+offsy*2},  "                If you land hard you crash!                ", olc::DARK_GREEN);
		DrawString( {pos.x+10, pos.y+offsy*3},  " The game is played by watching the altitude carefully     ", olc::DARK_GREEN);
		DrawString( {pos.x+10, pos.y+offsy*4},  " while running your missions gathering cargo by landing    ", olc::DARK_GREEN);
		DrawString( {pos.x+10, pos.y+offsy*5},  " softly on top of them,you can pick up as many as you like ", olc::DARK_GREEN);
		DrawString( {pos.x+10, pos.y+offsy*6},  "    when you feel like it just land on the red dropzone    ", olc::DARK_GREEN);
		DrawString( {pos.x+10, pos.y+offsy*7},  "          and it will offload automagically.               ", olc::DARK_GREEN);
		DrawString( {pos.x+10, pos.y+offsy*8},  " Be aware that when you roll and pitch you loose altitude. ", olc::DARK_GREEN);
		DrawString( {pos.x+10, pos.y+offsy*9},  "    You will have to increse throttle to stay airborn.     ", olc::DARK_GREEN);
		DrawString( {pos.x+10, pos.y+offsy*10}, "The four engines power-level shows with red filled circles ", olc::DARK_GREEN);
		DrawString( {pos.x+10, pos.y+offsy*12}, "          Use the ADWS, SPACE, UP, DOWN.  keys to play     ", olc::DARK_GREEN);
		DrawString( {pos.x+10, pos.y+offsy*14}, "               Have fun!  Regards DragonEye.               ", olc::DARK_GREEN);
	}



	void EndGame() {
		int offsy = 10;
		int offsx = ScreenWidth()/4;
		int asdf = ScreenHeight()/4;
		FillRect({offsx-1, asdf-1 }, {300+2, 200+2}, olc::VERY_DARK_GREY);
		DrawRect({offsx, asdf}, {300, 200}, olc::RED);

		if (last_velocity_before_crashlanding <= game_critical_landing_velocity) {
			DrawString( {offsx+10, asdf+offsy*1}, "Oh holy pancake...", olc::GREY);
			DrawString( {offsx+10, asdf+offsy*2}, "What a spectacular crash!", olc::GREY);
			DrawString( {offsx+10, asdf+offsy*3}, "Groundbreaking velocity:", olc::GREY);
			ss.str(""); ss << last_velocity_before_crashlanding;
			DrawString( {offsx+10+28*8, asdf+offsy*3}, ss.str(), olc::RED);
		} else {
			DrawString( {offsx+10, asdf+offsy*1}, "I am sorry to see you go...", olc::GREY);
			DrawString( {offsx+10, asdf+offsy*2}, "  Hope you had fun! L8r o7 ", olc::GREY);
		}
		DrawString( {offsx+10, asdf+offsy*4}, "Score: ", olc::GREY);
		ss.str(""); ss << std::setw(0) << player_points;
		DrawString( {offsx+10+9*8, asdf+offsy*4}, ss.str(), olc::GREEN);

		DrawString( {offsx+10, asdf+offsy*5}, "Successfull deliveries: ", olc::GREY);
		ss.str(""); ss << std::setw(0) << player_deliveries;
		DrawString( {offsx+10+24*8, asdf+offsy*5}, ss.str(), olc::GREEN);
		
		DrawString( {offsx+10, asdf+offsy*18}, " SPACE/ENTER to restart, ESC to quit", olc::RED);
		
	}



	void ShowInventory( olc::vi2d pos) {
		int offs = 10;
		int j;
		if ( inventory.size() > 0) {
			for ( j = 0; j < inventory.size(); ++j) {
				ss.str(""); ss <<  static_cast<char>(inventory[j].cargoType); 
				DrawString( {13, 62+j*offs}, ss.str(), olc::GREEN); 
			}
			DrawRect( {pos.x, pos.y+8}, {80,offs*j+3});
		} else {
			DrawString( {pos.x+13, pos.y+10}, "Empty", olc::RED);
		}

		DrawString( { pos.x+5, pos.y} , "Inventory", olc::GREEN);

	}

	void CountTheChicken( int cargoType) {
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
			player_points += cargoType*23;
			player_deliveries++;
			break;
		
		default:
			break;
		}
	}

	void CheckDropPickupOnLanding() {
		if ( int(altitude) == 0) {
			// Anything here?
			for (int i = 0; i < cargos.size(); ++i) {
				if ( fabs(ship_pos.x-cargos[i].pos.x) < game_object_proximity_limit) {
					if ( fabs(ship_pos.y-cargos[i].pos.y) < game_object_proximity_limit) {
						// yay, something here.

						// is it some cargo? then move it into inventory
						if (isdigit( cargos[i].cargoType)) {
						   inventory.push_back(cargos[i]);
						   cargos.erase(cargos.begin() + i);  // off the map with it
						} else {
							// if it is the drop point, yay happy you, unload and be happy
							if (cargos[i].cargoType == 'd') {
								// drop it all
								if (inventory.size() > 0) {
									for (int j = 0; j < inventory.size(); ++j) {
										CountTheChicken( inventory[j].cargoType);  // offload 
									}
									inventory.clear(); 
								}
							}
						}
					}
				}
			}
		}
	}

	void DrawGameMapOnScreen( olc::vf2d ship_pos) {
		float cx;
		float cy;
		float sx,sy;
		float scale_alt;
		float scale_factor = 1.5*(max_altitude/(altitude+10));

		int offsett = 10;

		olc::Pixel col = olc::GREEN;

		for (int i = 0; i < cargos.size(); ++i) {
			cx = (cargos[i].pos.x - ship_pos.x + ship_on_screen_pos.x+40);
			cy = (cargos[i].pos.y - ship_pos.y + ship_on_screen_pos.y+40);

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
			if ( sqrt( (ship_pos.x-cargos[i].pos.x)*(ship_pos.x-cargos[i].pos.x) + (ship_pos.y-cargos[i].pos.y)*(ship_pos.y-cargos[i].pos.y)) < game_clip_objects_radius) {
				DrawCircle( {int (cx),int(cy)} ,10, col);
				ss.str(""); ss << std::setw(1) << static_cast<char>(cargos[i].cargoType);
				DrawString( {int(cx)-3,int(cy)-3},ss.str());
			}
	    }
	}

	void DrawMinimap( olc::vi2d mm_pos, olc::vf2d ship_pos) {
		float scale_x = float(minimap_size.x)/world_max.x;
		float scale_y = float(minimap_size.y)/world_max.y;

		// draw cargos
		for (int i = 0; i < cargos.size(); ++i) {
        	FillCircle({mm_pos.x+6+ int(cargos[i].pos.x*scale_x), mm_pos.y+7+ int(cargos[i].pos.y*scale_y)}, 3, olc::GREEN);
	    }

		// draw dropzone
		FillCircle( { mm_pos.x+6+ int( dropzone.x*scale_x), mm_pos.y+7+ int( dropzone.y*scale_y) }, 3, olc::RED);

		// draw ship
		DrawCircle( { mm_pos.x+ int( (ship_pos.x+20)*scale_x), mm_pos.y+ int( (ship_pos.y+20)*scale_y) }, 5, olc::RED);

		// minimap boundary
		DrawRect( {mm_pos.x,mm_pos.y}, {minimap_size.x,minimap_size.y}, olc::YELLOW);
	}

	void DrawAltitude( int x, int y) {
		int BarHeight = 100;
		int BarWidth = 20;
		float scale = max_altitude / BarHeight;
		int AltBarHeight = altitude / scale;

		DrawRect( {x,y}, { BarWidth, BarHeight});
		FillRect( {x+1,(y+100)-(AltBarHeight-2)},{BarWidth-2, (AltBarHeight-4)},olc::RED);

	}

	void DrawShipAngle( olc::vi2d sos_pos, float anglx, float angly) {
		float y1 = (sos_pos.y+37)+(angly*60);
		float x2 = (sos_pos.x+37)+(anglx*60);
		DrawLine({sos_pos.x-35,int(y1)},{sos_pos.x+110,int(y1)},olc::GREEN);
		DrawLine({int(x2),sos_pos.y-35},{int(x2),sos_pos.y+110},olc::GREEN);

	}

	void DrawShipOnScreen( olc::vi2d sos_pos,int engineSize) {
		float engineOffset = engineSize*2.5;
		DrawCircle(sos_pos,engineSize); // engine 1
		FillCircle(sos_pos,int(throttle[0]*engineSize),olc::RED);  // power level

		DrawCircle({sos_pos.x,int(sos_pos.y+engineOffset)},engineSize); // engine 2
		FillCircle({sos_pos.x,int(sos_pos.y+engineOffset)},int(throttle[1]*engineSize), olc::RED); // engine 2

		DrawCircle({int(sos_pos.x+engineOffset),int(sos_pos.y+engineOffset)},engineSize); // engine 3
		FillCircle({int(sos_pos.x+engineOffset),int(sos_pos.y+engineOffset)},int(throttle[2]*engineSize), olc::RED); // engine 3
		
		DrawCircle({int(sos_pos.x+engineOffset),sos_pos.y},engineSize); // engine 4
		FillCircle({int(sos_pos.x+engineOffset),sos_pos.y},int(throttle[3]*engineSize), olc::RED); // engine 4
	}

};


int main()
{
	Game hover;
	if (hover.Construct(640, 360, 2, 2,false)) // 640x360
		hover.Start();

	return 0;
}
