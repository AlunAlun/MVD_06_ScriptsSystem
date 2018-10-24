#pragma once
#include "includes.h"
#include "Components.h"
#include <map>

//struct to store mouse state
struct Mouse {
	int x, y, delta_x, delta_y;
};

//System which manages all our controls
class ControlSystem {
public:
	void init();
	void update(float dt);

	//functions called directly from main.cpp, via game
	void updateMousePosition(int new_x, int new_y);
	void key_mouse_callback(int key, int action, int mods);

	//public functions to get key and mouse
	bool GetKey(int code) { return input[code]; }
	bool GetButton(int code) { return input[code]; }
	//mouse is public, it's just four ints
	Mouse mouse;

private:

	int context_; // 0 = in-game, 1 = menu...
    int cam_id_store = -1;

	bool input[GLFW_KEY_LAST];

	//function to update entity movement
    void updateFPS(Control& input_comp, float dt);
	void updateFree(Control& input_comp, float dt);
};
