#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <cmath>
#include <iostream>
#include "../SDL_Stuff/timer.h"
#include "../SDL_Stuff/environment.h"

const int SCREEN_FPS = 30;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

/*
The direction keys point the head of snake NSEW
Coming accross food adds a unit to the tail of the snake
Movement of the snake consists of adding a unit to the head (becoming the new head) and deleting a unit from the tail
*/

enum Direction           { N, 	      S, 	E, 	    W, None = -1 };
const int dValues[][4] = { { 0, -1 }, { 0, 1 }, { 1, 0 }, { -1, 0 } };

struct Point 
{
	int x, y;
	Point() : x(0), y(0) {}
	Point( int ex, int why ) : x(ex), y(why) {}
};

//Generic unit, any object appearing on the screen
struct Unit
{
	Point pos;
	SDL_Rect vis;
	int r, g, b;
	int vw;

	Unit(int x, int y) : pos(x,y), vw(20), r(255), g(255), b(255)
	{
		vis = { pos.x, pos.y, vw, vw };
	}
	void draw( SDL_Renderer* rend );
	Unit move( Direction d ); 
	~Unit();
};

typedef std::vector<Unit> Snake;

Unit Unit::move( Direction d ) 
{
	//For reasons unknown, move is being called twice a frame. Compensating by dividing by 2
	int posX = (pos.x + dValues[d][0]*vw/2);
	int posY = (pos.y + dValues[d][1]*vw/2);
	return Unit( posX, posY );
}

Unit::~Unit() 
{
	pos.x = -1;
	pos.y = -1;
	vis = { 0,0,0,0};
}

void Unit::draw( SDL_Renderer* rend ) 
{
	/* Associate a point with a shape
	   draw shape at its location*/
	SDL_SetRenderDrawColor( rend, 255, r, g, b );
	SDL_RenderFillRect( rend, &vis );
}

void move( Snake & snake, Direction d )
{
	if( d != None ) {
		snake.push_back( snake.back().move( d ) );	
		snake.erase( snake.begin() );
	} else return;
}

Direction handleInputs( SDL_Event e, Direction prev) 
{
	//No double backing 
	auto f = [&]( Direction d ) {
		if( d == W && prev == E ) {
			return prev;
		} else 
		if( d == E && prev == W ) {
			return prev;
		} else 
		if( d == N && prev == S ) {
			return prev;
		} else 
		if( d == S && prev == N ) {
			return prev;
		} 
		 return d;
	};
	
	if( e.type == SDL_KEYDOWN && e.key.repeat == 0 ) {
		switch( e.key.keysym.sym ) {
			case SDLK_h: return f( W ); break;
			case SDLK_j: return f( S ); break;
			case SDLK_k: return f( N ); break;
			case SDLK_l: return f( E ); break;
			default: break;
		}
	}

	return prev;
}

bool collision( SDL_Rect a, SDL_Rect b ) 
{
	double leftA=a.x, rightA=a.x+a.w, topA=a.y, bottomA=a.y+a.h;
	double leftB=b.x, rightB=b.x+b.w, topB=b.y, bottomB=b.y+b.h;

	if( bottomA <= topB ) {
		return false;
	}
	if( topA >= bottomB ) {
		return false;
	}
	if( rightA <= leftB ) {
		return false;
	}
	if( leftA >= rightB ) {
		return false;
	}

	return true;
}

int main() 
{

	SDL_Window*   window   = NULL;  
	SDL_Renderer* renderer = NULL;

	Snake snake = { };

	SDL_Event e;

	Direction md=None;
	
	//The frames per second timer
	LTimer fpsTimer;

	//The frames per second cap timer
	LTimer capTimer;

	//Start counting frames per second
	int countedFrames = 0;
	fpsTimer.start();

	for( int i=0; i<1; ++i ) {
		snake.push_back( Unit(i + 1, i + 1) );
	}

	Unit* food = new Unit( 40 + rand() % 620, 40 + rand() % 440 );

	if( !init(& window, & renderer ) ) {
		return 1;	
	}
	bool running = true;
	while( running ) {
		capTimer.start();
		if( SDL_PollEvent( &e ) ) {
			if( e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_q ) {
				std::cout << "Quitting!" << '\n';
				running = false;
			}
			md = handleInputs( e, md );
		}

		move( snake, md );

		//Calculate and correct fps
		float avgFPS = countedFrames / ( fpsTimer.getTicks() / 1000.f );
		if( avgFPS > 2000000 ) {
			avgFPS = 0;
		}

		if(snake.size() != 1) {
			for( int i=0; i < snake.size() - 1; ++i ) {
				/*For some reason move() is being called twice a frame, this is a lazy fix to that problem*/
				SDL_Rect collider = { snake[i].pos.x, snake[i].pos.y, snake[i].vw/2, snake[i].vw/2 } ;
				SDL_Rect head = { snake.back().pos.x, snake.back().pos.y, snake[i].vw/2, snake[i].vw/2 } ;
				if( collision( collider, head ) ) {
					std::cout << "Game Over!" << '\n';
					running = false;
				}
			}
		}

		if( snake.back().pos.x + snake.back().vw > SCREEN_WIDTH 
				|| snake.back().pos.x < 0 ) {
			std::cout << "Game Over!" << '\n';
			running = false;
		}
		if( snake.back().pos.y + snake.back().vw >= SCREEN_HEIGHT 
				|| snake.back().pos.y <= 0 ) {
			std::cout << "Game Over!" << '\n';
			running = false;
		}
		
		if( collision( food->vis, snake.back().vis ) ) {
			snake.push_back( snake.back() );
			delete food;
			food = new Unit( 40 + rand() % 620, 40 + rand() % 440 );
		} 

		//Clear screen
		SDL_SetRenderDrawColor( renderer, 0, 0, 0, 0 );
		SDL_RenderClear( renderer );

		//Render Objects
		for( auto s : snake ) {
			s.draw( renderer );
		}

		food->draw( renderer );

		//Update screen
		SDL_RenderPresent( renderer );

		++countedFrames;
		//If frame finished early
		int frameTicks = capTimer.getTicks();
		if( frameTicks < SCREEN_TICKS_PER_FRAME ) {
			//Wait remaining time
			SDL_Delay( SCREEN_TICKS_PER_FRAME - frameTicks );
		}
	}

	//Free and close SDL
	close( &window, &renderer );
	delete food;

	return 0;
}
