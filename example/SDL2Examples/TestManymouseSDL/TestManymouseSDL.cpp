// TestManmouseSDL.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

//#include <iostream>
//
//int main()
//{
//    std::cout << "Hello World!\n";
//}
//
//// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
//// Debug program: F5 or Debug > Start Debugging menu
//
//// Tips for Getting Started: 
////   1. Use the Solution Explorer window to add/manage files
////   2. Use the Team Explorer window to connect to source control
////   3. Use the Output window to see build output and other messages
////   4. Use the Error List window to view errors
////   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
////   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

//Using SDL and standard IO
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>

#include "../../manymouse.h"
#include "../sdl-event-to-string-master/cpp_version/sdl_event_to_string.h"

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;


#define MAX_MICE 128
#define SCROLLWHEEL_DISPLAY_TICKS 100

static int available_mice = 0;

typedef struct
{
	int connected;
	int x;
	int y;
	SDL_Color color;
	char name[64];
	Uint32 buttons;
	Uint32 scrolluptick;
	Uint32 scrolldowntick;
	Uint32 scrolllefttick;
	Uint32 scrollrighttick;
} Mouse;

static Mouse mice[MAX_MICE];


static void initial_setup(int screen_w, int screen_h)
{
	int i;
	memset(mice, '\0', sizeof(mice));

	/* pick some random colors for each mouse. */
	for (i = 0; i < MAX_MICE; i++)
	{
		Mouse* mouse = &mice[i];
		mouse->x = screen_w / 2;
		mouse->y = screen_h / 2;
		mouse->color.r = (int)(255.0 * rand() / (RAND_MAX + 1.0));
		mouse->color.g = (int)(255.0 * rand() / (RAND_MAX + 1.0));
		mouse->color.b = (int)(255.0 * rand() / (RAND_MAX + 1.0));
	}
}

static void init_mice(void)
{
	int i;

	available_mice = ManyMouse_Init();

	if (available_mice < 0)
	{
		printf("Error initializing ManyMouse!\n");
		return;
	}

	printf("ManyMouse driver: %s\n", ManyMouse_DriverName());

	if (available_mice == 0)
	{
		printf("No mice detected!\n");
		return;
	}

	for (i = 0; i < available_mice; i++)
	{
		const char* name = ManyMouse_DeviceName(i);
		printf("#%d: %s\n", i, name);
	}

	if (available_mice > MAX_MICE)
	{
		printf("Clamping to first %d mice.\n", available_mice);
		available_mice = MAX_MICE;
	}

	for (i = 0; i < available_mice; i++)
	{
		const char* name = ManyMouse_DeviceName(i);
		strncpy_s(mice[i].name, name, sizeof(mice[i].name));
		mice[i].name[sizeof(mice[i].name) - 1] = '\0';
		mice[i].connected = 1;
	}
}

static void update_mice(int screen_w, int screen_h)
{
	ManyMouseEvent event;
	while (ManyMouse_PollEvent(&event))
	{
		Mouse* mouse;
		if (event.device >= (unsigned int)available_mice)
			continue;

		mouse = &mice[event.device];

		if (event.type == MANYMOUSE_EVENT_RELMOTION)
		{
			if (event.item == 0)
				mouse->x += event.value;
			else if (event.item == 1)
				mouse->y += event.value;
		}

		else if (event.type == MANYMOUSE_EVENT_ABSMOTION)
		{
			float val = (float)(event.value - event.minval);
			float maxval = (float)(event.maxval - event.minval);
			if (event.item == 0)
				mouse->x = (int)(val / maxval) * screen_w;
			else if (event.item == 1)
				mouse->y = (int)(val / maxval) * screen_h;
		}

		else if (event.type == MANYMOUSE_EVENT_BUTTON)
		{
			if (event.item < 32)
			{
				if (event.value)
					mouse->buttons |= (1 << event.item);
				else
					mouse->buttons &= ~(1 << event.item);
			}
		}

		else if (event.type == MANYMOUSE_EVENT_SCROLL)
		{
			if (event.item == 0)
			{
				if (event.value < 0)
					mouse->scrolldowntick = SDL_GetTicks();
				else
					mouse->scrolluptick = SDL_GetTicks();
			}
			else if (event.item == 1)
			{
				if (event.value < 0)
					mouse->scrolllefttick = SDL_GetTicks();
				else
					mouse->scrollrighttick = SDL_GetTicks();
			}
		}

		else if (event.type == MANYMOUSE_EVENT_DISCONNECT)
		{
			mice[event.device].connected = 0;
		}
	}
}

static void draw_mouse(SDL_Renderer* renderer, int idx)
{
	int i;
	Uint32 now = SDL_GetTicks();
	Mouse* mouse = &mice[idx];
	SDL_Rect r = { mouse->x, mouse->y, 10, 10 };
	//Uint32 color = SDL_MapRGB(renderer->format, mouse->color.r, mouse->color.g, mouse->color.b);

	if (mouse->x < 0) mouse->x = 0;
	if (mouse->x >= SCREEN_WIDTH) mouse->x = SCREEN_WIDTH - 1;
	if (mouse->y < 0) mouse->y = 0;
	if (mouse->y >= SCREEN_HEIGHT) mouse->y = SCREEN_HEIGHT  - 1;
	//SDL_FillRect(renderer, &r, color);  /* draw a square for mouse position. */
	SDL_SetRenderDrawColor(renderer, mouse->color.r, mouse->color.g, mouse->color.b, 255);
	SDL_RenderFillRect(renderer, &r);

	/* now draw some buttons... */
	//color = SDL_MapRGB(renderer->format, 0xFF, 0xFF, 0xFF);
	for (i = 0; i < 32; i++)
	{
		if (mouse->buttons & (1 << i))  /* pressed? */
		{
			r.w = 20;
			r.x = i * r.w;
			r.h = 20;
			r.y = SCREEN_HEIGHT - ((idx + 1) * r.h);
			//SDL_FillRect(renderer, &r, color);
			SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 255);
			SDL_RenderFillRect(renderer, &r);
		}
	}

	/* draw scroll wheels... */

#define DRAW_SCROLLWHEEL(var, item) \
        if (var > 0) \
        { \
            if ((now - var) > SCROLLWHEEL_DISPLAY_TICKS) \
                var = 0; \
            else \
            { \
                r.w = r.h = 20; \
                r.y = idx * r.h; \
                r.x = item * r.w; \
			    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 255);\
				SDL_RenderFillRect(renderer, &r);\
            } \
        }

	DRAW_SCROLLWHEEL(mouse->scrolluptick, 0);
	DRAW_SCROLLWHEEL(mouse->scrolldowntick, 1);
	DRAW_SCROLLWHEEL(mouse->scrolllefttick, 2);
	DRAW_SCROLLWHEEL(mouse->scrollrighttick, 3);

#undef DRAW_SCROLLWHEEL
}


static void draw_mice(SDL_Renderer* renderer)
{
	int i;
	//SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
	for (i = 0; i < available_mice; i++)
	{
		if (mice[i].connected)
			draw_mouse(renderer, i);
	}
	//SDL_Flip(screen);
}

int main(int argc, char* args[])
{
	//The window we'll be rendering to
	SDL_Window* window = NULL;

	//The surface contained by the window
	SDL_Surface* screenSurface = NULL;

	SDL_Renderer* renderer = NULL;

	SDL_bool cursor = SDL_TRUE;
	SDL_bool grabbed = SDL_FALSE;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}
	else
	{
		SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);
		//Create window
		window = SDL_CreateWindow("Move your mice, R to rescan, G to (un)grab, S to show/hide, ESC to quit", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (window == NULL)
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		}
		else
		{
			//Get window surface
			screenSurface = SDL_GetWindowSurface(window);

			//Fill the surface white
			SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));

			//Update the surface
			SDL_UpdateWindowSurface(window);

			//Wait two seconds
			//SDL_Delay(2000);
			//Main loop flag
			bool quit = false;

			//Event handler
			SDL_Event e;

			renderer = SDL_CreateRenderer(window, -1, 0);



			initial_setup(SCREEN_WIDTH, SCREEN_HEIGHT);
			init_mice();

			//While application is running
			while (!quit)
			{
				//Handle events on queue
				while (SDL_PollEvent(&e) != 0)
				{

					SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, sdlEventToString(e).c_str());

					//User requests quit
					if (e.type == SDL_QUIT)
					{
						quit = true;
					}
					else if (e.type == SDL_KEYDOWN)
					{
						if (e.key.keysym.sym == SDLK_ESCAPE)
						{
							quit = true;
						}
						else if (e.key.keysym.sym == SDLK_g)
						{
							//SDL_GrabMode grab = SDL_WM_GrabInput(SDL_GRAB_QUERY);
							//grab = (grab == SDL_GRAB_ON) ? SDL_GRAB_OFF : SDL_GRAB_ON;
							//SDL_WM_GrabInput(grab);
							grabbed = grabbed ? SDL_FALSE : SDL_TRUE;
							SDL_SetWindowGrab(window, grabbed);
						}
						else if (e.key.keysym.sym == SDLK_s)
						{
							cursor = cursor ? SDL_FALSE : SDL_TRUE;
							SDL_ShowCursor(cursor);
						}
						else if (e.key.keysym.sym == SDLK_r)
						{
							printf("\n\nRESCAN!\n\n");
							ManyMouse_Quit();
							init_mice();
						}

					}

				}

				//Apply the image
				//SDL_BlitSurface(gXOut, NULL, gScreenSurface, NULL);
				update_mice(SCREEN_WIDTH, SCREEN_HEIGHT);
				//SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x0, 0x0, 0x0));
				SDL_RenderClear(renderer);
				draw_mice(renderer);
				SDL_RenderPresent(renderer);
				


				//Update the surface
				//SDL_UpdateWindowSurface(window);
			}
		}
	}

	//Destroy window
	SDL_DestroyWindow(window);

	//Quit SDL subsystems
	ManyMouse_Quit();
	SDL_Quit();

	return 0;
}
