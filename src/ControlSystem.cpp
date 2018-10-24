#include "ControlSystem.h"
#include "extern.h"

//set initial state of input system
void ControlSystem::init() {
	//set initial context to 'playing game'
	context_ = 0;

	//set all keys and buttons to 0
	for (int i = 0; i < GLFW_KEY_LAST; i++) input[i] = 0;
}

//called from hardware input (via game)
void ControlSystem::key_mouse_callback(int key_button, int action, int mods) {
	if (action == GLFW_PRESS) input[key_button] = true;
	if (action == GLFW_RELEASE) input[key_button] = false;
}

//called from hardware input (via game)
void ControlSystem::updateMousePosition(int new_x, int new_y) {
	mouse.delta_x = new_x - mouse.x;
	mouse.delta_y = new_y - mouse.y;
	mouse.x = new_x;
	mouse.y = new_y;
}

//called once per frame
void ControlSystem::update(float dt) {
	if (context_ == 0) { //update player control
		
        //get reference to mesh components first
        auto& control_components = ECS.getAllComponents<Control>();
        
        //loop over mesh components by reference (&)
        for (auto &curr_comp : control_components) {
            if (curr_comp.input_type == ControlComponentTypeFree) {
                updateFree(curr_comp, dt);
            } else if (curr_comp.input_type == ControlComponentTypeFPS){
                updateFPS(curr_comp, dt);
            }
        }
	}
}


void ControlSystem::updateFPS(Control& input_comp, float dt) {
    Transform& transform = ECS.getComponentFromEntity<Transform>(input_comp.owner);
    Camera& camera = ECS.getComponentFromEntity<Camera>(input_comp.owner);
    
    //multiply speeds by delta time
    float move_speed = input_comp.move_speed * dt;
    float turn_speed = input_comp.turn_speed * dt;
    
    //rotate camera just like Free movement
	if (input[GLFW_MOUSE_BUTTON_LEFT]) {
        lm::mat4 R_yaw, R_pitch;
        R_yaw.makeRotationMatrix(mouse.delta_x * turn_speed, lm::vec3(0, 1, 0));
        camera.forward = R_yaw * camera.forward;
        transform.rotateLocal(mouse.delta_x * turn_speed, lm::vec3(0, 1, 0));
        lm::vec3 pitch_axis = camera.forward.normalize().cross(lm::vec3(0, 1, 0));
        R_pitch.makeRotationMatrix(mouse.delta_y * turn_speed, pitch_axis);
        camera.forward = R_pitch * camera.forward;
    }
    
    //fps control should have three colliders that are attached to child gameobjects of this one
    auto& colliders = ECS.getAllComponents<Collider>();
    
    Collider& collider_down = colliders[input_comp.FPS_collider_down];
    Collider& collider_forward= colliders[input_comp.FPS_collider_forward];
    Collider& collider_left = colliders[input_comp.FPS_collider_left];
    Collider& collider_right = colliders[input_comp.FPS_collider_right];
    Collider& collider_back = colliders[input_comp.FPS_collider_back];

	//collisions and gravity
	//player down ray is always colliding, we need to keep player at 'FPS_height' units above nearest collider
    float dist_above_ground = (transform.position()-collider_down.collision_point).length();
    //collision test # 1
    if (collider_down.colliding && dist_above_ground < input_comp.FPS_height + 0.01f) // if below or on ground
    {
        //say we can jump
        input_comp.FPS_can_jump = true;
        //force player to correct height above ground
        transform.position(transform.position().x, collider_down.collision_point.y+input_comp.FPS_height, transform.position().z);
    }
    else { // we are in the air
        if (input_comp.FPS_jump_force > 0.0) {// slow down jump with time
            input_comp.FPS_jump_force -= input_comp.FPS_jump_force_slowdown*dt;
        }
        else {// clamp force to 0 if it is already below
            input_comp.FPS_jump_force = 0;
        }
        
        //move player according to jump force and gravity
        transform.translate(0.0f, (input_comp.FPS_jump_force-input_comp.FPS_gravity)*dt, 0.0f);
        
        //Collision test #2, as we might have moved down since test #1
        dist_above_ground = (transform.position()-collider_down.collision_point).length();
        if (collider_down.colliding && dist_above_ground < input_comp.FPS_height + 0.01f) // if below or on ground
        {
            //force player to correct height
            transform.position(transform.position().x, collider_down.collision_point.y+input_comp.FPS_height, transform.position().z);
        }
    }
    
    //jump
    if (input_comp.FPS_can_jump && input[GLFW_KEY_SPACE] == true){
        
        //set jump state to false cos we don't want to double/multiple jump
        input_comp.FPS_can_jump = false;
        
        //add force to jump upwards
        input_comp.FPS_jump_force = input_comp.FPS_jump_initial_force;
        
        //start jump
        transform.translate(0.0f, input_comp.FPS_jump_force*dt, 0.0f);
    }
    
    //forward and strafe 
    lm::vec3 forward_dir = camera.forward.normalize() * move_speed;
    lm::vec3 strafe_dir = camera.forward.cross(lm::vec3(0, 1, 0)) * move_speed;
    //nerf y components because we can't fly in an FPS
    forward_dir.y = 0.0;
    strafe_dir.y = 0.0;
    //now move
    if (input[GLFW_KEY_W] == true && !collider_forward.colliding)
        transform.translate(forward_dir);
    if (input[GLFW_KEY_S] == true && !collider_back.colliding)
        transform.translate(forward_dir * -1.0f);
    if (input[GLFW_KEY_A] == true && !collider_left.colliding)
        transform.translate(strafe_dir * -1.0f);
    if (input[GLFW_KEY_D] == true && !collider_right.colliding)
        transform.translate(strafe_dir);

    //update camera position
    camera.position = transform.position();
    
    //check if switch to Debug cam
	if (input[GLFW_KEY_O] == true) ECS.main_camera = 0; //debug cam is 0
	if (input[GLFW_KEY_P] == true) ECS.main_camera = 1; 
}

//update an entity with a free movement control component 
void ControlSystem::updateFree(Control& input_comp, float dt) {
	
    Transform& transform = ECS.getComponentFromEntity<Transform>(input_comp.owner);
	Camera& camera = ECS.getComponentFromEntity<Camera>(input_comp.owner);

	//rotate camera if clicking the mouse - update camera.forward
	if (input[GLFW_MOUSE_BUTTON_LEFT]) {
		lm::mat4 R_yaw, R_pitch;
		
		//yaw - axis is up vector of world
		R_yaw.makeRotationMatrix(mouse.delta_x * 0.005f, lm::vec3(0, 1, 0));
		camera.forward = R_yaw * camera.forward;

		//pitch - axis is strafe vector of camera i.e cross product of cam_forward and up
		lm::vec3 pitch_axis = camera.forward.normalize().cross(lm::vec3(0, 1, 0));
		R_pitch.makeRotationMatrix(mouse.delta_y * 0.005f, pitch_axis);
		camera.forward = R_pitch * camera.forward;
	}

	//multiply speeds by delta time 
	float move_speed = input_comp.move_speed * dt;
	float turn_speed = input_comp.turn_speed * dt;

	lm::vec3 forward_dir = camera.forward.normalize() * move_speed;
	lm::vec3 strafe_dir = camera.forward.cross(lm::vec3(0, 1, 0)) * move_speed;

	if (input[GLFW_KEY_W] == true)	transform.translate(forward_dir);
	if (input[GLFW_KEY_S] == true)	transform.translate(forward_dir * -1);
	if (input[GLFW_KEY_A] == true) 	transform.translate(strafe_dir*-1);
	if (input[GLFW_KEY_D] == true) 	transform.translate(strafe_dir);

	//update camera position
	camera.position = transform.position();
    
}

