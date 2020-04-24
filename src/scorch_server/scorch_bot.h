#pragma once

enum scorch_bot_state {
	idle,
	aiming,
	moving
};

struct scorch_bot {
	const int idle_time = 50;
	scorch_bot_state state;
	int counter;
	//float aim_target;
	//int move_target;
	scorch_player* tank;

	float launch_angle;
	float launch_power;
	weapon_type launch_weapon;

	scorch_bot();
	void update(scorch_engine& game);
	void post_update(scorch_engine& game);

	bool select_target(scorch_engine& game);
};
