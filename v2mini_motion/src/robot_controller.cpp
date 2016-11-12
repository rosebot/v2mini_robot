#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdexcept>
#include <stdio.h>
#include <cmath>

#include "v2mini_motion/robot_controller.h"

namespace v2mini_motion
{

const Uint8 *keys = SDL_GetKeyboardState(NULL);

RobotController::RobotController()
{

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) == 0)
	{
		//Set texture filtering to linear
		SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1");

		window = SDL_CreateWindow("V2Mini Controller", SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

		if (window == NULL)
		{
			throw std::runtime_error("SDL failed to create window\n");

		}
		else
		{
			controller = SDL_GameControllerOpen(0);

			if (controller == NULL)
			{
				printf("open failed: %s\n", SDL_GetError());
			}

			// Setup SDL window and load robot icon
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

			if (renderer == NULL)
			{
				throw std::runtime_error("SDL failed to render\n");
			}

			SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

			// todo remove .png stuff vv
			IMG_Init(IMG_INIT_PNG);
//			std::string ros_path = ros::package::getPath("v2mini_robot")
			SDL_Surface* loadedSurface = IMG_Load("/home/v2-mini/Pictures/robo_icon.jpg"); //todo make rel path
			texture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
			SDL_FreeSurface(loadedSurface);

			reRenderImage();
		}
	}
	else
	{
		throw std::runtime_error("SDL failed to initialize\n");
	}
}

RobotController::~RobotController()
{
	//Deallocate SDL resources
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	if (controller != NULL)
	{
		SDL_GameControllerClose(controller);
		controller = NULL;
	}

	window = NULL;
	renderer = NULL;
	texture = NULL;

	IMG_Quit();
	SDL_Quit();
}

float* RobotController::getKeyCmds()
{

	static float key_cmds[] = {0, 0, 0, 0};
	int max_value = 100;

	if(keys[SDL_SCANCODE_ESCAPE])
	{
		quit = true;
	}

	// Base Left & Right
	if (keys[SDL_SCANCODE_LEFT] != keys[SDL_SCANCODE_RIGHT])
	{
		if (keys[SDL_SCANCODE_RIGHT] == 1) {
			icon_vel[BASE_VELX] = max_value;
		}
		else
		{
			icon_vel[BASE_VELX] = -max_value;
		}
	}
	else
	{
		icon_vel[BASE_VELX] = 0;
	}

	// Base Forward & Back
	if (keys[SDL_SCANCODE_UP] != keys[SDL_SCANCODE_DOWN])
	{
		if (keys[SDL_SCANCODE_UP] == 1)
		{
			icon_vel[BASE_VELY] = max_value;
		}
		else
		{
			icon_vel[BASE_VELY] = -max_value;
		}
	}
	else
	{
		icon_vel[BASE_VELY] = 0;
	}

	// Angular CW & CCW
	if (keys[SDL_SCANCODE_Q] != keys[SDL_SCANCODE_W])
	{
		if (keys[SDL_SCANCODE_Q] == 1)
		{
			icon_vel[BASE_VELZ] = max_value;
		}
		else
		{
			icon_vel[BASE_VELZ] = -max_value;
		}
	}
	else
	{
		icon_vel[BASE_VELZ] = 0;
	}

	int torso_vel = 0;

	// Torso Height Up & Down
	if (keys[SDL_SCANCODE_R] != keys[SDL_SCANCODE_F])
	{

		if (keys[SDL_SCANCODE_R] == 1)
		{
			torso_vel = max_value;
		}
		else
		{
			torso_vel = -max_value;
		}
	}

	key_cmds[BASE_VELX] = icon_vel[BASE_VELX];
	key_cmds[BASE_VELY] = icon_vel[BASE_VELY];
	key_cmds[BASE_VELZ] = icon_vel[BASE_VELZ];
	key_cmds[TORSO_VELZ] = torso_vel;

	mapVelocity(key_cmds, max_value);

	return key_cmds;
}

float* RobotController::getGamepadCmds()
{
	static float cmds[] = {0, 0, 0, 0};
	int max_value = 32769;
	const int deadzone = 300;

	// LEFT JOY STICK X
	int axis_leftx = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);

	if (std::abs(axis_leftx) > deadzone)
	{
		cmds[BASE_VELX] = axis_leftx;
	}
	else
	{
		cmds[BASE_VELX] = 0;
	}

	// LEFT JOY STICK Y
	int axis_lefty = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);

	if (std::abs(axis_lefty) > deadzone)
	{
		cmds[BASE_VELY] = -axis_lefty;
	}
	else
	{
		cmds[BASE_VELY] = 0;
	}

	// LEFT & RIGHT BOTTOM TRIGGERS
	int cww_rot = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
	int cw_rot = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

	cmds[BASE_VELZ] = cww_rot - cw_rot;

	// A & Y BUTTONS
	int torso_up = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y);
	int torso_down = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A);

	cmds[TORSO_VELZ] = torso_up - torso_down;

	mapVelocity(cmds, max_value);

	return cmds;
}

void RobotController::mapVelocity(float *val_array, int ceiling_value)
{
	float velx = val_array[BASE_VELX];
	float vely = val_array[BASE_VELY];

	float velr = std::sqrt(velx * velx + vely * vely);

	if (velr > ceiling_value)
	{
		ceiling_value = velr;
	}

	// Map value range for x and y
	val_array[BASE_VELX] = velx * MAX_BASE_RADIAL_VEL / ceiling_value;
	val_array[BASE_VELY] = vely * MAX_BASE_RADIAL_VEL / ceiling_value;

	// Cap angular speed
	if (std::abs(float(val_array[BASE_VELZ])) > MAX_BASE_ANGULAR_VEL)
	{
		if (val_array[BASE_VELZ] < 0)
		{
			val_array[BASE_VELZ] = -MAX_BASE_ANGULAR_VEL;
		}
		else
		{
			val_array[BASE_VELZ] = MAX_BASE_ANGULAR_VEL;
		}
	}

	// Cap torso speed
	if (std::abs(float(val_array[TORSO_VELZ])) > MAX_TORSO_VEL)
	{
		if (val_array[TORSO_VELZ] < 0)
		{
			val_array[TORSO_VELZ] = -MAX_TORSO_VEL;
		}
		else
		{
			val_array[TORSO_VELZ] = MAX_TORSO_VEL;
		}
	}
}

void RobotController::reRenderImage()
{
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

bool RobotController::checkQuitStatus()
{
	return quit;
}

} // namespace v2mini_motion
