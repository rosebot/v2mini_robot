#ifndef __ROBOT_CONTROLLER_H
#define __ROBOT_CONTROLLER_H

#include <SDL2/SDL.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const int MAX_BASE_RADIAL_VEL = 30;		// cm/s
const int MAX_BASE_ANGULAR_VEL = 30;	// deg/s
const float MAX_TORSO_VEL = 0.6;		// cm/s

namespace v2mini_motion {

enum ROBOT_VEL {BASE_VELX, BASE_VELY, BASE_VELZ, TORSO_VELZ};

class RobotController {

	private:

		bool quit = false;

		int icon_vel[3] = {0, 0, 0}; // todo change this...

		SDL_Window* window = NULL;
		SDL_Surface* surface = NULL;
		SDL_Renderer* renderer = NULL;
		SDL_Texture* texture = NULL;
		SDL_GameController *controller = NULL;

	public:

		RobotController();
		~RobotController();

		float* getKeyCmds();
		float* getGamepadCmds();
		bool checkQuitStatus();
		void reRenderImage();
		void mapVelocity(float*, int);

};

}

#endif
