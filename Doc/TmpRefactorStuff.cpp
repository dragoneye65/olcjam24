// todo: this structure is not currently in use
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

			// Ships initial values
		InitializeShip(inferiourBattleCruiser, startpos);
		inferiourBattleCruiser.x = startpos.x;
		inferiourBattleCruiser.x = startpos.y;
		inferiourBattleCruiser.thrust = ship_max_thrust;

		oldRustyBucket = inferiourBattleCruiser;
		oldRustyBucket.x += 10.0f;
		oldRustyBucket.y -= 10.0f;



		// Calculate the ship movements
		void updateShip(Ship& ship, float fElapsedTime) {
			// Calculate thrust for each engine
			ship.angle = float(atan2(ship.z, sqrt(pow(ship.x, 2) + pow(ship.y, 2))));

			// individual engine thrust
			float thrustX1 = ship.thrust * ship.throttle1 * float(cos(ship.angle + M_PI / 4));
			float thrustY1 = ship.thrust * ship.throttle1 * float(sin(ship.angle + M_PI / 4));
			float thrustX2 = ship.thrust * ship.throttle2 * float(cos(ship.angle - M_PI / 4));
			float thrustY2 = ship.thrust * ship.throttle2 * float(sin(ship.angle - M_PI / 4));
			float thrustX3 = ship.thrust * ship.throttle3 * float(cos(ship.angle - 3 * M_PI / 4));
			float thrustY3 = ship.thrust * ship.throttle3 * float(sin(ship.angle - 3 * M_PI / 4));
			float thrustX4 = ship.thrust * ship.throttle4 * float(cos(ship.angle + 3 * M_PI / 4));
			float thrustY4 = ship.thrust * ship.throttle4 * float(sin(ship.angle + 3 * M_PI / 4));

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
			float velocityMagnitude = float(sqrt(pow(ship.vel_x, 2) + pow(ship.vel_y, 2) + pow(ship.vel_z, 2)));
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

		// autothrottle if no throttle key input
		if (ship_autothrottle_toggle && !ship_throttle_key_held) {
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
		}

		// DrawShipNew(oldRustyBucket);	// new version, todo: get it working!

		// TODO: Must make it frame rate independent!
		// give the update function it's trottle parameters which is normalized
		// gotta calc send in our own throttle
		//inferiourBattleCruiser.throttle1 = throttle1;
		//inferiourBattleCruiser.throttle2 = throttle2;
		//inferiourBattleCruiser.throttle3 = throttle3;
		//inferiourBattleCruiser.throttle4 = throttle4;

		// updateShip(inferiourBattleCruiser, fElapsedTime);
		// DrawShipNew(inferiourBattleCruiser);	// new version, todo: get it working!




		// velocity = distance * time
// position = velocity * time
// acceleration += gravity * time
// cap acceleration to gravity

// ss.str(""); ss << "Vel " << std::setw(6) << ship_velocity_z;
// DrawString({ 20,200 }, ss.str(), olc::YELLOW);


				/*
				it_engine_sound->dSpeedModifier = engine_sound_speed * it_engine_sound->pWave->file.samplerate() / 44100.0;
				it_engine_sound->dDuration = it_engine_sound->pWave->file.duration() / engine_sound_speed;
				*/




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


		// set sound on
		// wave_engine.SetOutputVolume(0.8f);
		// ma_engine.Toggle(sound_engine_id, false);
		// ma_engine.Pause(sound_ship_id);
		// ma_engine_listener_set_position(ma_engine.GetEngine(), 0, 0.0f, 0.0f, 0.0f);
		// ma_sound_set_looping(ma_engine.get .GetSound(), 1);

		// Debug: <SHIFT-C> toggle ship_crash
		if (GetKey(olc::Key::SHIFT).bHeld)
			if (GetKey(olc::Key::C).bReleased)
				ship_crashed = !ship_crashed;


		/*
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
		*/

		//InitializeShip(inferiourBattleCruiser, startpos);
		//inferiourBattleCruiser.thrust = ship_max_thrust;
		//inferiourBattleCruiser.max_z = 100.0f;
		//inferiourBattleCruiser.screen_pos = ship_on_screen_pos;

		//oldRustyBucket = inferiourBattleCruiser;
		//oldRustyBucket.x += -10;
		//oldRustyBucket.y += 10;

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


/*
void DrawShipNew(Ship& ship) {

	float angle_x = atan2(ship.y, ship.x);
	float angle_y = float(atan2(ship.z, sqrt(pow(ship.x, 2) + pow(ship.y, 2))));

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

	//tl_x = tl_x - tl_x * dec_scale * 0.1f * sin(ship_angle_y);
	//tl_y = tl_y - tl_y * dec_scale * 0.1f * sin(ship_angle_y);
	//tr_x = tr_x + tr_x * dec_scale * 0.1f * sin(ship_angle_y);
	//tr_y = tr_y - tr_y * dec_scale * 0.1f * sin(ship_angle_y);

	//bl_x = bl_x + bl_x * dec_scale * 0.1f * sin(ship_angle_y);
	//br_x = br_x - br_x * dec_scale * 0.1f * sin(ship_angle_y);

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
*/

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


// TODO: We dont need this anymore,  we draw a decal now
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
	DrawRect(sos_pos - olc::vf2d{ float(bay_size / 2),float(bay_size / 2) }, { bay_size,bay_size }, olc::RED);

}

//	DrawShipAngle( ship_on_screen_pos, ship_angle_x, ship_angle_y);


void DrawShipAngle(olc::vi2d sos_pos, float anglx, float angly) {
	float y1 = (sos_pos.y + 37) + (angly * 60);
	float x2 = (sos_pos.x + 37) + (anglx * 60);
	DrawLine({ sos_pos.x - 35,int(y1) }, { sos_pos.x + 110,int(y1) }, olc::GREEN);
	DrawLine({ int(x2),sos_pos.y - 35 }, { int(x2),sos_pos.y + 110 }, olc::GREEN);
}


