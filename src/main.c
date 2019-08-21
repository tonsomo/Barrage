/***************************************************************************
    copyright            : (C) 2003 by Michael Speck
    email                : http://lgames.sf.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "defs.h"
#include "shots.h"
#include "units.h"
#include "data.h"
#include "chart.h"
#include "bfield.h"
#include "menu.h"
#include "gp2x_joystick.h"

SDL_Surface *screen;
#define BITDEPTH 16

int delay = 0;
int fullscreen = 1;
int audio_on = 1;
int audio_freq = 22050;
int audio_format = AUDIO_S16LSB;
int audio_channels = 2;
int audio_mix_channel_count = 0;

extern SDL_Surface 
	*img_ammo, *background, *img_ground, *img_logo, *img_icons, *img_black, *img_cursors; 
extern SDL_Font *ft_main, *ft_chart, *ft_chart_highlight, *ft_help, *ft_copyright;
extern SDL_Cursor *cr_empty;
extern int ammo_w, ammo_h;
extern int cursor_w, cursor_x_offset;
extern char *optarg;

Menu main_menu;
enum {
	ID_GAME = 101,
	ID_HELP,
	ID_CHART,
	ID_QUIT
};

Menu quit_menu;
enum {
     ID_YES = 101,
     ID_NO
};

Vector gun_base = { 20, 240, 0 };
Vector gun_img_offset = { -9, -20, 0 };

int player_score = 0;
int player_ammo = 0;
char player_name[20] = "Michael"; /* is updated when entering highscore */
int game_start = 0, game_end = 0; /* time in global secs when game started and has to end */

int timer_blink_delay = 0; /* timer starts to blink when time runs out */
int timer_visible = 1; /* used to blink timer */

enum {
	STATE_MENU,
	STATE_HELP,
	STATE_CHART,
	STATE_GAME,
	STATE_QUIT
};
int state = STATE_MENU; /* state of game */

static void fade_screen( int type, int time )
{
	SDL_Surface *buffer = 0;
	float alpha;
	float alpha_change; /* per ms */
	int leave = 0;
	int ms;
	int cur_ticks, prev_ticks;

	/* get screen contents */
	buffer = SDL_CreateRGBSurface( 
			SDL_SWSURFACE, screen->w, screen->h, 
			screen->format->BitsPerPixel,
			screen->format->Rmask, screen->format->Bmask,
			screen->format->Gmask, screen->format->Amask );
	if ( buffer == 0 ) return;
	SDL_BlitSurface( screen, 0, buffer, 0 );

	/* compute alpha and alpha change */
	if ( type == FADE_OUT ) {
		alpha = 255;
		alpha_change = -255.0 / time;
	}
	else {
		alpha = 0;
		alpha_change = 255.0 / time;
	}

	/* fade */
	cur_ticks = prev_ticks = SDL_GetTicks();
	while ( !leave ) {
		prev_ticks = cur_ticks;
		cur_ticks = SDL_GetTicks();
		ms = cur_ticks - prev_ticks;
		
		alpha += alpha_change * ms;
		if ( type == FADE_IN && alpha >= 255 ) break;
		if ( type == FADE_OUT && alpha <= 0 ) break;
		
		/* update */
		SDL_FillRect( screen, 0, 0x0 );
		SDL_SetAlpha( buffer, SDL_SRCALPHA | SDL_RLEACCEL, (int)alpha );
		SDL_BlitSurface( buffer, 0, screen, 0 );
		SDL_Flip(screen);
		if (delay>0) SDL_Delay(10);
	}

	/* update screen */
	SDL_SetAlpha( buffer, 0, 0 );
	if ( type == FADE_IN )
		SDL_BlitSurface( buffer, 0, screen, 0 );
	else
		SDL_FillRect( screen, 0, 0x0 );
	SDL_Flip(screen);
	SDL_FreeSurface( buffer );
}

static int all_filter( const SDL_Event *event ) { return 0; }

static void wait_for_input( void )
{
	SDL_EventFilter filter;
	SDL_Event event;
	
	filter = SDL_GetEventFilter();
	SDL_SetEventFilter(all_filter);
	while ( SDL_PollEvent(&event) ); /* clear queue */
	SDL_SetEventFilter(filter);
	
	while ( 1 ) {
		SDL_WaitEvent(&event);
		if (event.type == SDL_QUIT) {
			return;
		}
		if (event.type == SDL_KEYDOWN || event.type == SDL_JOYBUTTONDOWN)
			return;
	}
}

static void draw_score()
{
	char str[8];

	if ( player_score < 0 ) player_score = 0;
	snprintf( str, 8, "%06i", player_score ); str[7] = 0;
	SDL_WriteText( ft_main, screen, 10, 10, str );
}

static void draw_time( int ms )
{
	static int old_secs = 0;
	char str[8];
	int secs = game_end - time(0);
	
	/* blink timer when time is running out in such a way
	 * that it becomes visible everytime a new second started */
	if ( secs < 30 ) {
		if ( timer_blink_delay )
		if ( (timer_blink_delay-=ms) <= 0 )
			timer_visible = 0;
		if ( secs != old_secs ) {
			timer_visible = 1;
			timer_blink_delay = 500;
			old_secs = secs;
		}
	}
	
	if ( !timer_visible ) return;
	snprintf( str, 8, "%i:%02i", secs/60, secs%60 ); str[7] = 0;
	SDL_WriteTextRight( ft_main, screen, 630, 10, str );
}

static void draw_ammo()
{
	int i, full, rest;
	SDL_Rect drect, srect;
	
	full = player_ammo / AMMO_BOMB; /* one tile represents a bomb ammo which
					   contains of AMMO_BOMB single grenade shots */
	rest = player_ammo % AMMO_BOMB; /* first ammo may contain less than AMMO_BOMB ammo */
	
	drect.w = srect.w = ammo_w;
	drect.h = srect.h = ammo_h;
	srect.x = 0; srect.y = 0;
	drect.y = 480 - 10 - drect.h;
	drect.x = 10;

	for ( i = 0; i < full; i++ ) { 
		SDL_BlitSurface( img_ammo, &srect, screen, &drect );
		drect.y -= drect.h;
	}

	if ( rest > 0 ) {
		srect.y = (AMMO_BOMB - rest) * ammo_h;
		SDL_BlitSurface( img_ammo, &srect, screen, &drect );
	}
}

/* draw cursor centered at x,y */
static void draw_cursor( SDL_Surface *dest, int x, int y )
{
	SDL_Rect srect,drect;

	drect.w = srect.w = cursor_w;
	drect.h = srect.h = img_cursors->h;
	srect.x = cursor_x_offset; srect.y = 0;
	drect.x = x - (cursor_w>>1); drect.y = y - (img_cursors->h>>1);
	SDL_BlitSurface( img_cursors, &srect, dest, &drect );
}

/* add copyright */
static void draw_logo_centered( SDL_Surface *dest, int y )
{
	SDL_Rect drect;

	drect.x = (dest->w-img_logo->w)/2;
	drect.y = y;
	drect.w = dest->w; drect.h = dest->h;

	SDL_BlitSurface( img_logo, 0, dest, &drect );

	SDL_WriteText( ft_copyright, dest, 4, 454, "(C) 2003 Michael Speck" );
	SDL_WriteTextRight( ft_copyright, dest, 636, 454, "http://lgames.sf.net" );
}

#define LINE(y,text) SDL_WriteText(ft_help,dest,x,y,text)
static void draw_help( SDL_Surface *dest )
{
	int x = 70, target_y = 60, gun_y = 180, hint_y = 350;
	SDL_Rect srect, drect;

	SDL_BlitSurface( img_black, 0, dest, 0 );
	
	/* objective */
	LINE(10,"Your objective is to destroy as many targets as" );
	LINE(30,"possible within 3 minutes." );
	
	/* icons and points */
	SDL_WriteText( ft_help, dest, x,target_y, "Target Scores:" );
	srect.w = drect.w = 30;
	srect.h = drect.h = 30;
	srect.y = 0; drect.y = target_y+20;
	srect.x = 0; drect.x = x; 
	SDL_BlitSurface( img_icons, &srect, dest, &drect );
	SDL_WriteText( ft_help, dest, x + 34,target_y+24, "Soldier: 5 Pts" );
	srect.x = 30; drect.x = x+180;
	SDL_BlitSurface( img_icons, &srect, dest, &drect );
	SDL_WriteText( ft_help, dest, x+214,target_y+24, "Jeep: 20 Pts" );
	srect.x = 60; drect.x = x+350;
	SDL_BlitSurface( img_icons, &srect, dest, &drect );
	SDL_WriteText( ft_help, dest, x+384,target_y+24, "Tank: 50 Pts" );
	SDL_WriteText( ft_help, dest, x,target_y+50, 
			"If a jeep makes it safe through the screen you'll" );
	SDL_WriteText( ft_help, dest, x,target_y+70, 
			"loose 10 pts. For a tank you'll loose 25 pts. For" );
	SDL_WriteText( ft_help, dest, x,target_y+90,
			"soldiers there is no such penalty." );

	/* gun handling */
	LINE(gun_y,    "Gun Handling:");
	LINE(gun_y+20, "The gun is controlled by the stick. Move the");
	LINE(gun_y+40, "crosshair at the target and press X to fire a" );
	LINE(gun_y+60, "large grenade. (costs 6 rounds, 0.5 secs fire rate)");
	LINE(gun_y+80, "press A to fire a small grenade. (1 round)" );
	LINE(gun_y+100,"Use the Y button to reload your gun" );
	LINE(gun_y+120,"to the full 36 rounds. Reloading always costs" );
	LINE(gun_y+140,"36 points so do this as late as possible." );

	/* hint */
	LINE(hint_y,    "NOTE: A tank cannot be damaged by small" ); 
	LINE(hint_y+20, "grenades which you should only use at soldiers" );
	LINE(hint_y+40, "and jeeps when you are experienced enough in" );
	LINE(hint_y+60, "aiming. For rookies it is much better to only" );
	LINE(hint_y+80, "use the large grenades (X), count the" );
	LINE(hint_y+100,"shots and reload after six times. (Y)" );
}

static void take_screenshot()
{
	char buf[32];
	static int i;
	sprintf( buf, "screenshot%i.bmp", i++ );
	SDL_SaveBMP( screen, buf );

}

static void game_init()
{
	bfield_finalize(); /* clear battlefield from menu */
	bfield_init( BFIELD_GAME, &gun_base, &gun_img_offset );
	player_score = 0;
	player_ammo = MAX_AMMO;

	SET_CURSOR( CURSOR_CROSSHAIR );
	fade_screen( FADE_OUT, 500 );
	SDL_BlitSurface( background, 0, screen, 0 );
	fade_screen( FADE_IN, 500 );

	game_start = time(0); game_end = game_start + GAME_TIME; timer_visible = 1;
}

static void game_finalize( int check_chart )
{
	int rank;
	char buf[64];
	
	bfield_finalize();
	SET_CURSOR( CURSOR_ARROW );

	fade_screen( FADE_OUT, 500 );
	bfield_init( BFIELD_DEMO, &gun_base, &gun_img_offset ); /* new battlefield for menu */
	bfield_draw( screen, 0, 0 );
	fade_screen( FADE_IN, 500 );
	
	/* check wether highscore was entered */
	if ( check_chart ) {
		SDL_WriteTextCenter( ft_chart, screen, 320, 130, "The time is up!" );
		sprintf( buf, "Your result is %i points.", player_score );
		SDL_WriteTextCenter( ft_chart, screen, 320, 160, buf );
		if ( ( rank = chart_get_rank( player_score ) ) >= 0 ) {
			SDL_WriteTextCenter( 
					ft_chart, screen, 320, 200, "Good job! This makes you" );
			sprintf( buf, "Topgunner #%i", rank+1 );
			SDL_WriteTextCenter( ft_chart, screen, 320, 230, buf );
			SDL_WriteTextCenter( ft_chart, screen, 320, 290, "Sign here:" );
			SDL_Flip(screen);
			SDL_EnterTextCenter( ft_chart, screen, 320, 320, 18, player_name );
			chart_add_entry( player_name, player_score );
		}
		else {
			SDL_WriteTextCenter( 
					ft_chart, screen, 320, 290, "Not enough to be a topgunner!" );
			SDL_WriteTextCenter( 
					ft_chart, screen, 320, 320, "Go, try again! Dismissed." );
			SDL_Flip(screen);
			wait_for_input();
		}
		chart_save();
	}
}
	
static void main_loop()
{
	Uint8 *keystate, empty_keys[SDLK_LAST];
	int x = 0, y = 0;
	int cur_ticks, prev_ticks;
	int ms;
	int buttonstate;
	int reload_clicked = 0;
	int reload_key = SDLK_SPACE;
	int frame_time = 0, frames = 0;
	Vector dest;
	int quit = 0;
	int input_delay = 0; /* while >0 no clicks are accepted */
	SDLMod modstate = 0;
	int shot_type;
    int mx=0,my=0;
    unsigned long c;
	
	frame_time = SDL_GetTicks(); /* for frame rate */
	memset( empty_keys, 0, sizeof( empty_keys ) ); /* to block input */
		
	state = STATE_MENU;
	bfield_init( BFIELD_DEMO, &gun_base, &gun_img_offset );
	
	cur_ticks = prev_ticks = SDL_GetTicks();
	while ( !quit ) {
	  if (delay>0) SDL_Delay(delay);
	  
		prev_ticks = cur_ticks;
		cur_ticks = SDL_GetTicks();
		ms = cur_ticks - prev_ticks;

		/* gather events in queue and get mouse/keyboard states */
		SDL_PumpEvents();
		//buttonstate = SDL_GetMouseState( &x, &y );
		keystate = SDL_GetKeyState( 0 );
		modstate = SDL_GetModState();
	
		/* update the battefield (particles,units,new cannonfodder) */
		if(state!=STATE_QUIT) bfield_update( ms );

        c=gp2x_joystick_read();

        if(c&GP2X_LEFT) x-=5;
        if(c&GP2X_RIGHT) x+=5;
        if(c&GP2X_UP) y-=5;
        if(c&GP2X_DOWN) y+=5;

        SDL_WarpMouse(x,y);

		/* update input_delay */
		if ( input_delay > 0 ) 
		if ( (input_delay-=ms) < 0 ) input_delay = 0;
		
		/* draw ground, trees, units, particles */
		bfield_draw( screen, 0, 0 );
		
		/* according to state draw add-ons */
		switch ( state ) {
			case STATE_GAME:
				draw_score();
				draw_time( ms );
				draw_ammo();
				break;
			case STATE_MENU:
				draw_logo_centered( screen, 70 );
				menu_draw( &main_menu, screen );
				break;
			case STATE_CHART:
				chart_draw( screen );
				break;
			case STATE_HELP:
				draw_help( screen );
				break;
			case STATE_QUIT:
                menu_draw( &quit_menu, screen );
		}
		
		/* add cursor */
		draw_cursor( screen, x, y );
		
		/* udpate screen */
		SDL_Flip(screen); frames++;

		/* end game? */
		if ( state == STATE_GAME )
		if ( time(0) >= game_end ) {
			game_finalize( 1 /*check highscore*/ );
			state = STATE_CHART;
			input_delay = INPUT_DELAY;
			cur_ticks = prev_ticks = SDL_GetTicks(); /* reset timer */
		}
		
		/* after this point only input is handled */
		if ( input_delay > 0 ) continue;
		
		/* handle input */
		switch ( state ) {
		    case STATE_HELP:
		    case STATE_CHART:
			if ( keystate[SDLK_ESCAPE] ||
			     keystate[SDLK_SPACE] || 
			     keystate[SDLK_RETURN] || 
			     keystate[SDLK_SPACE] ||
			     (c&GP2X_X || c&GP2X_B )) {
				input_delay = INPUT_DELAY;
				state = STATE_MENU;
				/* clear the selection of the menu entry */
				menu_handle_motion( &main_menu, -1, -1 );
			}
			break;
		    case STATE_GAME:
			/* abort game */
			if ( keystate[SDLK_ESCAPE] ) {
				game_finalize( 0/*don't check highscore*/ );
				cur_ticks = prev_ticks = SDL_GetTicks(); /* reset timer */
				state = STATE_CHART;
				input_delay = INPUT_DELAY;
			}
			/* set direction of gun */
			bfield_update_gun_dir( x, y );
			/* fire gun */
			if ( bfield_gun_is_ready() ) {
				dest.x = x; dest.y = y; dest.z = 0;
				shot_type = ST_NONE;
				if ( c&GP2X_X )
						shot_type = ST_BOMB;
				if ( c&GP2X_A )
						shot_type = ST_GRENADE;
				if ( c&GP2X_B )
					shot_type = ST_GRENADE;
				if ( shot_type != ST_NONE )
					bfield_fire_gun( shot_type, &dest, 60, 0 );
			}
			/* reload */
			if ( c&GP2X_Y || keystate[reload_key] ) {
				if ( !reload_clicked ) {
					reload_clicked = 1;
					bfield_reload_gun();
				}
			}
			else
				reload_clicked = 0;
			if(c&GP2X_START)
			{
                 state=STATE_QUIT;
                 input_delay = INPUT_DELAY;
            }
			break;
			case STATE_QUIT:
                 if(c&GP2X_X) {
                      switch (menu_handle_click( &quit_menu,x,y)) {
                      case ID_YES:
				           game_finalize( 0/*don't check highscore*/ );
				           cur_ticks = prev_ticks = SDL_GetTicks(); /* reset timer */
				           state = STATE_CHART;
				           input_delay = INPUT_DELAY;
                           break;
                      case ID_NO:
                           state=STATE_GAME;
                           break;
                      }
                 }
			     else
				     menu_handle_motion( &quit_menu, x, y );
                 break;
		    case STATE_MENU:
			if ( keystate[SDLK_ESCAPE] )
				quit = 1;
			if ( c&GP2X_X ) {
				switch (menu_handle_click( &main_menu, x, y ) ) {
				    case ID_QUIT: 
					quit = 1; 
					break;
				    case ID_HELP:
					state = STATE_HELP;
					input_delay = INPUT_DELAY;
					break;
				    case ID_CHART: 
					state = STATE_CHART; 
					input_delay = INPUT_DELAY;
					break;
				    case ID_GAME:
					state = STATE_GAME; 
					game_init();
					break;
				}
			}
			else
				menu_handle_motion( &main_menu, x, y );
			break;
		}
		
		/* switch fullscreen/window anywhere */
		/* enabe/disable sound anywhere */
		if ( c&GP2X_SELECT ) {
			audio_on = !audio_on;
			input_delay = INPUT_DELAY;
		}
		/* take screenshots anywhere */
		if ( c&GP2X_R )
			take_screenshot();
	}

	frame_time = SDL_GetTicks() - frame_time;
	printf( "Time: %.2f, Frames: %i\n", (double)frame_time/1000, frames );
	printf( "FPS: %.2f\n", 1000.0 * frames / frame_time );

	fade_screen( FADE_OUT, 500 );
	
}

void Terminate(void)
{
    gp2x_joystick_deinit();
	SDL_Quit();
	chdir("/usr/gp2x");
	execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);
}

int main( int argc, char **argv )
{
  char c;
  
  printf( "BARRAGE v%s\n", VERSION );
  printf( "Copyright 2003-2005 Michael Speck (http://lgames.sf.net)\n" );
  printf( "Released under Gnu GPL\n---\n" );
  printf( "GP2X Port By Jonn Blanchard\n" );

  /*while ( ( c = getopt( argc, argv, "d:ws" ) ) != -1 )
    {
      switch (c)
	{
	case 'd': delay = atoi(optarg); break;
	case 'w': fullscreen=0; break;
	case 's': audio_on = 0; break;
	}
    }
  printf("main loop delay: %d ms\n",delay);
	*/
  srand( time(0) );
	
  if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK ) < 0 ) {
    printf( "%s\n", SDL_GetError() );
    exit(1);
  }
  if ( SDL_SetVideoMode( 640, 480, 16, SDL_HWSURFACE | SDL_DOUBLEBUF ) < 0 ) {
    printf( "%s\n", SDL_GetError() );
    exit(1);
  }
  screen = SDL_GetVideoSurface();
  atexit(SDL_Quit);
  SDL_WM_SetCaption( "LBarrage", 0 );

  SDL_JoystickOpen(0);
  
  gp2x_joystick_init();
	
#ifdef AUDIO_ENABLED	
  if ( Mix_OpenAudio( audio_freq, audio_format, audio_channels, 1024 ) < 0 ) {
    printf( "%s\n", SDL_GetError() );
    exit(1);
  }
  audio_mix_channel_count = Mix_AllocateChannels( 16 );
#endif
	
  atexit(Terminate);
  atexit(data_delete); data_load();
  chart_load();

  SDL_SetCursor( cr_empty );
  /* create simple menu */
  menu_init( &main_menu, 230, 40 );
  menu_add_entry( &main_menu, "Enter Shooting Range", ID_GAME );
  menu_add_entry( &main_menu, "Receive Briefing", ID_HELP );
  menu_add_entry( &main_menu, "Topgunners", ID_CHART );
  menu_add_entry( &main_menu, "", 0 );
  menu_add_entry( &main_menu, "Quit", ID_QUIT );

  menu_init( &quit_menu, 230, 40 );
  menu_add_entry( &quit_menu, "Resume Game", ID_NO );
  menu_add_entry( &quit_menu, "Quit Game", ID_YES );
	
  /* run game */
  main_loop();

#ifdef AUDIO_ENABLED
  Mix_CloseAudio();
#endif
	
  return (0);
}

