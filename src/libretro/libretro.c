#include <stdint.h>

#include <libretro.h>

#include "mame.h"
#include "driver.h"
#include "state.h"
#include "log.h"

#if (HAS_DRZ80 || HAS_CYCLONE)
#include "frontend_list.h"
#endif

// Wrapper to build MAME on 3DS. It doesn't have stricmp.
#ifdef _3DS
int stricmp(const char *string1, const char *string2)
{
    return strcasecmp(string1, string2);
}
#endif

void mame_frame(void);
void mame_done(void);
static void retro_set_audio_buff_status_cb(void);
void mame2003_video_get_geometry(struct retro_game_geometry *geom);
static void configure_cyclone_mode (int driverIndex);

#if defined(__CELLOS_LV2__) || defined(GEKKO) || defined(_XBOX)
unsigned activate_dcs_speedhack = 1;
#else
unsigned activate_dcs_speedhack = 0;
#endif

struct retro_perf_callback perf_cb;

retro_log_printf_t log_cb = NULL;
retro_video_refresh_t video_cb = NULL;
static retro_input_poll_t poll_cb = NULL;
static retro_input_state_t input_cb = NULL;
static retro_audio_sample_batch_t audio_batch_cb = NULL;
retro_environment_t environ_cb = NULL;

unsigned long lastled = 0;
retro_set_led_state_t led_state_cb = NULL;

struct retro_audio_buffer_status_callback buf_status_cb;

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_cb = cb; }

bool retro_audio_buff_active        = false;
unsigned retro_audio_buff_occupancy = 0;
bool retro_audio_buff_underrun      = false;
int frameskip;

static struct retro_message frontend_message;

void frontend_message_cb(const char *message_string, unsigned frames_to_display)
{
	frontend_message.msg = message_string;
	frontend_message.frames = frames_to_display;
	environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &frontend_message);
}

static void retro_audio_buff_status_cb(bool active, unsigned occupancy, bool underrun_likely)
{
   retro_audio_buff_active    = active;
   retro_audio_buff_occupancy = occupancy;
   retro_audio_buff_underrun  = underrun_likely;
}

void retro_set_audio_buff_status_cb(void)
{
  if (frameskip >0 && frameskip >= 12)
  {
      if (!environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK,
            &buf_status_cb))
      {
         if (log_cb)
            log_cb(RETRO_LOG_WARN, "Frameskip disabled - frontend does not support audio buffer status monitoring.\n");
			frontend_message_cb("Frameskip disabled - frontend does not support audio buffer status monitoring.\n", 240);
         retro_audio_buff_active    = false;
         retro_audio_buff_occupancy = 0;
         retro_audio_buff_underrun  = false;
      }
      else
      log_cb(RETRO_LOG_INFO, "Frameskip Enabled\n");
  }
   else
      environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK,NULL);

}

void retro_set_environment(retro_environment_t cb)
{
   static const struct retro_variable vars[] = {
      { "mame2003-xtreme-amped-turboboost", "TurboBoost; X6|disabled|X1|X2|X3|X4|X5|X6|X7|X8|X9|XX|auto|auto_aggressive|auto_max" },
      { "mame2003-xtreme-amped-oc", "Reverse OverClock; 99|100|101|102|103|104|105|106|107|108|109|110|111|112|113|114|115|116|117|118|119|120|121|122|123|124|125|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|62|63|64|65|66|67|68|69|70|71|72|73|74|75|76|77|78|79|80|81|82|83|84|85|86|87|88|89|90|91|92|93|94|95|96|97|98" },
      { "mame2003-xtreme-amped-dcs-speedhack",
#if defined(__CELLOS_LV2__) || defined(GEKKO) || defined(_XBOX)
         "MK2/MK3 DCS Speedhack; enabled|disabled"
#else
         "MK2/MK3 DCS Speedhack; enabled|disabled"
#endif
      },
      { "mame2003-xtreme-amped-skip_disclaimer", "Skip Disclaimer; enabled|disabled" },
      { "mame2003-xtreme-amped-skip_warnings", "Skip Warnings; enabled|disabled" },
      { "mame2003-xtreme-amped-samples", "Samples; enabled|disabled" },
      { "mame2003-xtreme-amped-sample_rate", "Sample Rate (KHz); 48000|8000|11025|18500|22050|30000|44100" },
      { "mame2003-xtreme-amped-cheats", "Cheats; enabled|disabled" },
      { "mame2003-xtreme-amped-dialsharexy", "Share 2 player dial controls across one X/Y device; disabled|enabled" },
#if defined(__IOS__)
      { "mame2003-xtreme-amped-mouse_device", "Mouse Device; pointer|mouse|disabled" },
#else
      { "mame2003-xtreme-amped-mouse_device", "Mouse Device; mouse|pointer|disabled" },
#endif
      { "mame2003-xtreme-amped-rstick_to_btns", "Right Stick to Buttons; enabled|disabled" },
      { "mame2003-xtreme-amped-option_tate_mode", "TATE Mode; disabled|enabled" },
      { "mame2003-xtreme-amped-use_artwork", "Artwork(Restart); enabled|disabled" },
      #if defined(HAS_CYCLONE) && defined(HAS_DRZ80)
      { "mame2003-xtreme-amped-cyclone_mode", "Cyclone mode(Restart); default|disabled|Cyclone|DrZ80|Cyclone+DrZ80|DrZ80(snd)|Cyclone+DrZ80(snd)" },
      #elif defined(HAS_CYCLONE) && !defined(HAS_DRZ80)
      { "mame2003-xtreme-amped-cyclone_mode", "Cyclone mode(Restart); default|disabled|Cyclone },
      #elif !defined(HAS_CYCLONE) && defined(HAS_DRZ80)
      { "mame2003-xtreme-amped-cyclone_mode", "Cyclone mode(Restart); default|disabled|DrZ80|DrZ80(snd)" },
      #endif
      { NULL, NULL },
   };
   environ_cb = cb;

   cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)vars);
}

#ifndef PATH_SEPARATOR
# if defined(WINDOWS_PATH_STYLE) || defined(_WIN32)
#  define PATH_SEPARATOR '\\'
# else
#  define PATH_SEPARATOR '/'
# endif
#endif

static char* normalizePath(char* aPath)
{
   char *tok;
   static const char replaced = (PATH_SEPARATOR == '\\') ? '/' : '\\';

   for (tok = strchr(aPath, replaced); tok; tok = strchr(aPath, replaced))
      *tok = PATH_SEPARATOR;

   return aPath;
}

static int getDriverIndex(const char* aPath)
{
    char driverName[128];
    char *path, *last;
    char *firstDot;
    int i;

    // Get all chars after the last slash
    path = normalizePath(strdup(aPath ? aPath : "."));
    last = strrchr(path, PATH_SEPARATOR);
    memset(driverName, 0, sizeof(driverName));
    strncpy(driverName, last ? last + 1 : path, sizeof(driverName) - 1);
    free(path);

    // Remove extension
    firstDot = strchr(driverName, '.');

    if(firstDot)
       *firstDot = 0;

    // Search list
    for (i = 0; drivers[i]; i++)
    {
       if(strcmp(driverName, drivers[i]->name) == 0)
       {
          options.romset_filename_noext = strdup(driverName);
          if (log_cb)
             log_cb(RETRO_LOG_INFO, "Found game: %s [%s].\n", driverName, drivers[i]->name);
          return i;
       }
    }

    return -1;
}

static char* peelPathItem(char* aPath)
{
    char* last = strrchr(aPath, PATH_SEPARATOR);
    if(last)
       *last = 0;

    return aPath;
}

static int driverIndex; //< Index of mame game loaded

//

#define max_players 6
extern const struct KeyboardInfo retroKeys[];
extern int retroKeyState[];
extern int retroJsState[];
extern int16_t mouse_x[];
extern int16_t mouse_y[];
int16_t prev_pointer_x;
int16_t prev_pointer_y;
extern int16_t analogjoy[max_players][4];

extern struct osd_create_params videoConfig;

unsigned retroColorMode;
int16_t XsoundBuffer[2048];
char *fallbackDir;
char *systemDir;
char *romDir;
char *saveDir;




unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_get_system_info(struct retro_system_info *info)
{
   info->library_name = "MAME 2003-Xtreme";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version = "2K23 Amped" GIT_VERSION;
   info->valid_extensions = "zip";
   info->need_fullpath = true;
   info->block_extract = true;
}

int sample_rate;

int gotFrame;

unsigned dial_share_xy = 0;
unsigned mouse_device = 0;
unsigned rstick_to_btns = 0;
unsigned option_tate_mode = 0;

static void update_variables(void)
{
   struct retro_led_interface ledintf;
   struct retro_variable var;
   int prev_frameskip_type;

   /* ASM cores: 0=None,1=Cyclone,2=DrZ80,3=Cyclone+DrZ80,4=DrZ80(snd),5=Cyclone+DrZ80(snd) */
   var.value = NULL;
   var.key = "mame2003-xtreme-amped-cyclone_mode";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value)
   {
   #if (HAS_CYCLONE || HAS_DRZ80)
          if(strcmp(var.value, "default") == 0)
            options.cyclone_mode = 6;
          else if(strcmp(var.value, "Cyclone") == 0)
            options.cyclone_mode = 1;
          else if(strcmp(var.value, "DrZ80") == 0)
            options.cyclone_mode = 2;
          else if(strcmp(var.value, "Cyclone+DrZ80") == 0)
            options.cyclone_mode = 3;
          else if(strcmp(var.value, "DrZ80(snd)") == 0)
            options.cyclone_mode = 4;
          else if(strcmp(var.value, "Cyclone+DrZ80(snd)") == 0)
            options.cyclone_mode = 5;
          else /* disabled */
            options.cyclone_mode = 0;
       #endif
   }

   var.value = NULL;
   var.key = "mame2003-xtreme-amped-turboboost";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value)
   {
				prev_frameskip_type = frameskip;

   				if (strcmp(var.value, "X1") == 0)
					frameskip = 1;

				else if (strcmp(var.value, "X2") == 0)
					frameskip = 2;

				else if (strcmp(var.value, "X3") == 0)
					frameskip = 3;

				else if (strcmp(var.value, "X4") == 0)
					frameskip = 4;

				else if (strcmp(var.value, "X5") == 0)
					frameskip = 5;

				else if (strcmp(var.value, "X6") == 0)
					frameskip = 6;

				else if (strcmp(var.value, "X7") == 0)
					frameskip = 7;

				else if (strcmp(var.value, "X8") == 0)
					frameskip = 8;

				else if (strcmp(var.value, "X9") == 0)
					frameskip = 9;

				else if (strcmp(var.value, "XX") == 0)
					frameskip = 10;

				else if (strcmp(var.value, "auto") == 0)
					frameskip = 12;
				else if (strcmp(var.value, "auto_aggressive") == 0)
					frameskip = 13;
				else if(strcmp(var.value, "auto_max") == 0)
					frameskip = 14;
				else
					frameskip = 0;

			 if (frameskip != prev_frameskip_type)	retro_set_audio_buff_status_cb();
   }

   var.value = NULL;
   var.key = "mame2003-xtreme-amped-oc";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value)
   {
      options.oc = ((double) atoi(var.value) / 100);

   }

   var.value = NULL;
   var.key = "mame2003-xtreme-amped-dcs-speedhack";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value)
   {
      if(strcmp(var.value, "enabled") == 0)
         activate_dcs_speedhack = 1;
      else
         activate_dcs_speedhack = 0;
   }

   var.value = NULL;
   var.key = "mame2003-xtreme-amped-skip_disclaimer";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value)
   {
      if(strcmp(var.value, "enabled") == 0)
         options.skip_disclaimer = 1;
      else
         options.skip_disclaimer = 0;
   }

   var.value = NULL;
   var.key = "mame2003-xtreme-amped-skip_warnings";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value)
   {
      if(strcmp(var.value, "enabled") == 0)
         options.skip_warnings = 1;
      else
         options.skip_warnings = 0;
   }

   var.value = NULL;
   var.key = "mame2003-xtreme-amped-samples";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value)
   {
      if(strcmp(var.value, "enabled") == 0)
         options.use_samples = 1;
      else
         options.use_samples = 0;
   }

   var.value = NULL;
   var.key = "mame2003-xtreme-amped-sample_rate";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value)
   {
      options.samplerate = atoi(var.value);
   }
   else
      options.samplerate = 48000;

   var.value = NULL;
   var.key = "mame2003-xtreme-amped-cheats";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value)
   {
      if(strcmp(var.value, "enabled") == 0)
         options.cheat = 1;
      else
         options.cheat = 0;
   }

   var.value = NULL;
   var.key = "mame2003-xtreme-amped-dialsharexy";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value)
   {
      if(strcmp(var.value, "enabled") == 0)
         dial_share_xy = 1;
      else
         dial_share_xy = 0;
   }

   var.value = NULL;
   var.key = "mame2003-xtreme-amped-mouse_device";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value)
   {
      if(strcmp(var.value, "pointer") == 0)
         mouse_device = RETRO_DEVICE_POINTER;
      else if(strcmp(var.value, "mouse") == 0)
         mouse_device = RETRO_DEVICE_MOUSE;
      else
         mouse_device = 0;
   }

   var.value = NULL;
   var.key = "mame2003-xtreme-amped-rstick_to_btns";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value)
   {
      if(strcmp(var.value, "enabled") == 0)
         rstick_to_btns = 1;
      else
         rstick_to_btns = 0;
   }

   var.value = NULL;
   var.key = "mame2003-xtreme-amped-option_tate_mode";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value)
   {
      if(strcmp(var.value, "enabled") == 0)
         option_tate_mode = 1;
      else
         option_tate_mode = 0;
   }

   var.value = NULL;
   var.key = "mame2003-xtreme-amped-use_artwork";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value)
   {
      if(strcmp(var.value, "enabled") == 0)
         options.use_artwork = ~0;
      else
         options.use_artwork = 0;
   }

   ledintf.set_led_state = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LED_INTERFACE, &ledintf))
      led_state_cb = ledintf.set_led_state;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
  mame2003_video_get_geometry(&info->geometry);
  info->timing.fps = Machine->drv->frames_per_second;
  if ( Machine->drv->frames_per_second * 1000 < options.samplerate)
    info->timing.sample_rate = 22050;

  else
    info->timing.sample_rate = options.samplerate;
}

static void check_system_specs(void)
{
   // TODO - set variably
   // Midway DCS - Mortal Kombat/NBA Jam etc. require level 9
   unsigned level = 10;
   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
}

void retro_init (void)
{
   struct retro_log_callback log;
   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

#ifdef LOG_PERFORMANCE
   environ_cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf_cb);
#endif

   update_variables();
   check_system_specs();
}

void retro_deinit(void)
{
#ifdef LOG_PERFORMANCE
   perf_cb.perf_log();
#endif
}

void retro_reset (void)
{
    machine_reset();
}

/* get pointer axis vector from coord */
int16_t get_pointer_delta(int16_t coord, int16_t *prev_coord)
{
   int16_t delta = 0;
   if (*prev_coord == 0 || coord == 0)
   {
      *prev_coord = coord;
   }
   else
   {
      if (coord != *prev_coord)
      {
         delta = coord - *prev_coord;
         *prev_coord = coord;
      }
   }

   return delta;
}

void retro_run (void)
{
   int i;
   bool pointer_pressed;
   const struct KeyboardInfo *thisInput;
   bool updated = false;

   poll_cb();

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      update_variables();

   /* Keyboard */
   thisInput = retroKeys;
   while(thisInput->name)
   {
      retroKeyState[thisInput->code] = input_cb(0, RETRO_DEVICE_KEYBOARD, 0, thisInput->code);
      thisInput ++;
   }

   for (i = 0; i < max_players; i ++)
   {
      unsigned int offset = (i * 18);

      /* Analog joystick */
      analogjoy[i][0] = input_cb(i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
      analogjoy[i][1] = input_cb(i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
      analogjoy[i][2] = input_cb(i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
      analogjoy[i][3] = input_cb(i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);

      /* Joystick */
      if (rstick_to_btns)
      {
         /* if less than 0.5 force, ignore and read buttons as usual */
         retroJsState[RETRO_DEVICE_ID_JOYPAD_B + offset] = analogjoy[i][3] >  0x4000 ? 1 : input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
         retroJsState[RETRO_DEVICE_ID_JOYPAD_Y + offset] = analogjoy[i][2] < -0x4000 ? 1 : input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);
      }
      else
      {
         retroJsState[ RETRO_DEVICE_ID_JOYPAD_B + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
         retroJsState[ RETRO_DEVICE_ID_JOYPAD_Y + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);
      }
      retroJsState[ RETRO_DEVICE_ID_JOYPAD_SELECT + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);
      retroJsState[ RETRO_DEVICE_ID_JOYPAD_START + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
      retroJsState[ RETRO_DEVICE_ID_JOYPAD_UP + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
      retroJsState[ RETRO_DEVICE_ID_JOYPAD_DOWN + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
      retroJsState[ RETRO_DEVICE_ID_JOYPAD_LEFT + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
      retroJsState[ RETRO_DEVICE_ID_JOYPAD_RIGHT + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);
      if (rstick_to_btns)
      {
         retroJsState[ RETRO_DEVICE_ID_JOYPAD_A + offset] = analogjoy[i][2] >  0x4000 ? 1 : input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
         retroJsState[ RETRO_DEVICE_ID_JOYPAD_X + offset] = analogjoy[i][3] < -0x4000 ? 1 : input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
      }
      else
      {
         retroJsState[ RETRO_DEVICE_ID_JOYPAD_A + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
         retroJsState[ RETRO_DEVICE_ID_JOYPAD_X + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
      }
      retroJsState[ RETRO_DEVICE_ID_JOYPAD_L + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L);
      retroJsState[ RETRO_DEVICE_ID_JOYPAD_R + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R);
      retroJsState[ RETRO_DEVICE_ID_JOYPAD_L2 + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2);
      retroJsState[ RETRO_DEVICE_ID_JOYPAD_R2 + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2);
      retroJsState[ RETRO_DEVICE_ID_JOYPAD_L3 + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3);
      retroJsState[ RETRO_DEVICE_ID_JOYPAD_R3 + offset] = input_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3);

      if (mouse_device)
      {
         if (mouse_device == RETRO_DEVICE_MOUSE)
         {
            retroJsState[16 + offset] = input_cb(i, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
            retroJsState[17 + offset] = input_cb(i, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
            mouse_x[i] = input_cb(i, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
            mouse_y[i] = input_cb(i, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
         }
         else // RETRO_DEVICE_POINTER
         {
            pointer_pressed = input_cb(i, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
            retroJsState[16 + offset] = pointer_pressed;
            retroJsState[17 + offset] = 0; // padding
            mouse_x[i] = pointer_pressed ? get_pointer_delta(input_cb(i, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X), &prev_pointer_x) : 0;
            mouse_y[i] = pointer_pressed ? get_pointer_delta(input_cb(i, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y), &prev_pointer_y) : 0;
         }
      }
      else
      {
         retroJsState[16 + offset] = 0;
         retroJsState[17 + offset] = 0;
      }
   }
 if (options.oc)
 {
	if (cpunum_get_clockscale(0) != options.oc)
	{	printf("changing cpu - clockscale from %lf to%lf\n",cpunum_get_clockscale(0),options.oc);
		cpunum_set_clockscale(0, options.oc);
	}
 }

   mame_frame();
}


bool retro_load_game(const struct retro_game_info *game)
{
   if (!game)
      return false;

    // Find game index
    driverIndex = getDriverIndex(game->path);

    if(driverIndex)
    {
        #define describe_buttons(INDEX) \
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Joystick Left" },\
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Joystick Right" },\
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Joystick Up" },\
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Joystick Down" },\
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Button 1" },\
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Button 2" },\
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Button 3" },\
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Button 4" },\
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Button 5" },\
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Button 6" },\
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Button 7" },\
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Button 8" },\
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "Button 9" },\
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "Button 10" },\
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,   "Insert Coin" },\
        { INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,    "Start" },

        struct retro_input_descriptor desc[] = {
            describe_buttons(0)
            describe_buttons(1)
            describe_buttons(2)
            describe_buttons(3)
            { 0, 0, 0, 0, NULL }
            };

        fallbackDir = strdup(game->path);

        /* Get system directory from frontend */
        environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY,&systemDir);
        if (systemDir == NULL || systemDir[0] == '\0')
        {
            /* if non set, use old method */
            systemDir = normalizePath(fallbackDir);
            systemDir = peelPathItem(systemDir);
        }

        /* Get save directory from frontend */
        environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY,&saveDir);
        if (saveDir == NULL || saveDir[0] == '\0')
        {
            /* if non set, use old method */
            saveDir = normalizePath(fallbackDir);
            saveDir = peelPathItem(saveDir);
        }

        // Get ROM directory
        romDir = normalizePath(fallbackDir);
        romDir = peelPathItem(romDir);




        // Set all options before starting the game
        options.vector_resolution_multiplier = 2;
        options.antialias = 1; // 1 or 0
        options.beam = 2; //use 2.f  if using a decimal point 1|1.2|1.4|1.6|1.8|2|2.5|3|4|5|6|7|8|9|10|11|12 only works with antialas on
        options.translucency = 1; //integer: 1 to enable translucency on vectors
        options.vector_intensity = 1.5f; // 0.5|1.5|1|2|2.5|3
        options.vector_flicker = (int)(2.55 * 1.5f); // |0.5|1|1.5|2|2.5|3

        environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

		if(!init_game(driverIndex))
		return false;
        update_variables();
        configure_cyclone_mode(driverIndex);
        // Boot the emulator
        return run_game(driverIndex) == 0;
    }
    else
    {
        return false;
    }
}

void retro_unload_game(void)
{
    mame_done();

    free(fallbackDir);
    systemDir = 0;
}

size_t retro_serialize_size(void)
{
    extern size_t state_get_dump_size(void);

    return state_get_dump_size();
}

bool retro_serialize(void *data, size_t size)
{
   int cpunum;
	if(retro_serialize_size() && data && size)
	{
		/* write the save state */
		state_save_save_begin(data);

		/* write tag 0 */
		state_save_set_current_tag(0);
		if(state_save_save_continue())
		{
		    return false;
		}

		/* loop over CPUs */
		for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		{
			cpuintrf_push_context(cpunum);

			/* make sure banking is set */
			activecpu_reset_banking();

			/* save the CPU data */
			state_save_set_current_tag(cpunum + 1);
			if(state_save_save_continue())
			    return false;

			cpuintrf_pop_context();
		}

		/* finish and close */
		state_save_save_finish();

		return true;
	}

	return false;
}

bool retro_unserialize(const void * data, size_t size)
{
    int cpunum;
	/* if successful, load it */
	if (retro_serialize_size() && data && size && !state_save_load_begin((void*)data, size))
	{
        /* read tag 0 */
        state_save_set_current_tag(0);
        if(state_save_load_continue())
            return false;

        /* loop over CPUs */
        for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
        {
            cpuintrf_push_context(cpunum);

            /* make sure banking is set */
            activecpu_reset_banking();

            /* load the CPU data */
            state_save_set_current_tag(cpunum + 1);
            if(state_save_load_continue())
                return false;

            cpuintrf_pop_context();
        }

        /* finish and close */
        state_save_load_finish();


        return true;
	}

	return false;
}

static float              delta_samples;
int                       samples_per_frame = 0;
int                       orig_samples_per_frame =0;
short*                    samples_buffer;
short*                    conversion_buffer;
int                       usestereo = 1;

int osd_start_audio_stream(int stereo)
{
    if ( Machine->drv->frames_per_second * 1000 < options.samplerate)
      Machine->sample_rate=22050;

    else
      Machine->sample_rate = options.samplerate;

  delta_samples = 0.0f;
  usestereo = stereo ? 1 : 0;

  /* determine the number of samples per frame */
  samples_per_frame = Machine->sample_rate / Machine->drv->frames_per_second;
  orig_samples_per_frame = samples_per_frame;

  if (Machine->sample_rate == 0) return 0;

  samples_buffer = (short *) calloc(samples_per_frame+16, 2 + usestereo * 2);
  if (!usestereo) conversion_buffer = (short *) calloc(samples_per_frame+16, 4);

  return samples_per_frame;
}


int osd_update_audio_stream(INT16 *buffer)
{
	int i,j;
	if ( Machine->sample_rate !=0 && buffer )
	{
   		memcpy(samples_buffer, buffer, samples_per_frame * (usestereo ? 4 : 2));
		if (usestereo)
			audio_batch_cb(samples_buffer, samples_per_frame);
		else
		{
			for (i = 0, j = 0; i < samples_per_frame; i++)
        		{
				conversion_buffer[j++] = samples_buffer[i];
				conversion_buffer[j++] = samples_buffer[i];
		        }
         		audio_batch_cb(conversion_buffer,samples_per_frame);
		}


		//process next frame

		if ( samples_per_frame  != orig_samples_per_frame ) samples_per_frame = orig_samples_per_frame;

		// dont drop any sample frames some games like mk will drift with time

		delta_samples += (Machine->sample_rate / Machine->drv->frames_per_second) - orig_samples_per_frame;
		if ( delta_samples >= 1.0f )
		{

			int integer_delta = (int)delta_samples;
			if (integer_delta <= 16 )
                        {
				log_cb(RETRO_LOG_DEBUG,"sound: Delta added value %d added to frame\n",integer_delta);
				samples_per_frame += integer_delta;
			}
			else if(integer_delta >= 16) log_cb(RETRO_LOG_INFO, "sound: Delta not added to samples_per_frame too large integer_delta:%d\n", integer_delta);
			else log_cb(RETRO_LOG_DEBUG,"sound(delta) no contitions met\n");
			delta_samples -= integer_delta;

		}
	}
        return samples_per_frame;
}


void osd_stop_audio_stream(void)
{
}


// Stubs
unsigned retro_get_region (void) {return RETRO_REGION_NTSC;}
void *retro_get_memory_data(unsigned type) {return 0;}
size_t retro_get_memory_size(unsigned type) {return 0;}
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info){return false;}
void retro_cheat_reset(void){}
void retro_cheat_set(unsigned unused, bool unused1, const char* unused2){}
void retro_set_controller_port_device(unsigned in_port, unsigned device){}

void mapper(void)
{
// this will map z x n m keys to  retropad lr mame uses this key for rotary joysticks and sometimes with buttons.
static char buffer[100];
struct InputPort *entry;
struct InputPort *in;

if (Machine->input_ports)
   in = Machine->input_ports;

   while (in->type != IPT_END)
   {
      if (input_port_name(in) != 0 && seq_get_1(&in->seq) != CODE_NONE && (in->type & ~IPF_MASK) != IPT_UNKNOWN && (in->type & ~IPF_MASK) != IPT_OSD_RESERVED)
      {
         if (seq_get_1(&in->seq) != CODE_DEFAULT)
         {
            seq_name(input_port_seq(in),       buffer,sizeof(buffer) );
            //map non default z n n m mappings in drivers to retropad l/r
            if (buffer[0] == 'z') seq_set_1(&in->seq, JOYCODE_1_BUTTON5);
            if (buffer[0] == 'x') seq_set_1(&in->seq, JOYCODE_1_BUTTON6);
            if (buffer[0] == 'n') seq_set_1(&in->seq, JOYCODE_2_BUTTON5);
            if (buffer[0] == 'm') seq_set_1(&in->seq, JOYCODE_2_BUTTON6);
             //map non default pedal mappings in drivers to retropad l/r
            if(strcmp(input_port_name(in), "P1 Pedal 1") == 0)  seq_set_1(&in->seq, JOYCODE_1_BUTTON6);
            if(strcmp(input_port_name(in), "P1 Pedal 2") == 0)  seq_set_1(&in->seq, JOYCODE_1_BUTTON5);
            if(strcmp(input_port_name(in), "P2 Pedal 1") == 0)  seq_set_1(&in->seq, JOYCODE_2_BUTTON6);
            if(strcmp(input_port_name(in), "P2 Pedal 2") == 0)  seq_set_1(&in->seq, JOYCODE_2_BUTTON5);
            if(strcmp(input_port_name(in), "P3 Pedal 1") == 0)  seq_set_1(&in->seq, JOYCODE_3_BUTTON6);
            if(strcmp(input_port_name(in), "P3 Pedal 2") == 0)  seq_set_1(&in->seq, JOYCODE_3_BUTTON5);
            //map polpos pos gear change to button1
            if(strcmp(input_port_name(in), "Gear Change") == 0)  seq_set_1(&in->seq, JOYCODE_1_BUTTON1);
         }
      }
      in++;
    }
}

#if (HAS_CYCLONE || HAS_DRZ80)
int check_list(char *name)
{

   int found=0;
   int counter=0;
   while (fe_drivers[counter].name[0])
   {
      if  (strcmp(name,fe_drivers[counter].name)==0)
      {
         log_cb(RETRO_LOG_INFO, "frontend_list match {\"%s\", %d },\n",fe_drivers[counter].name,fe_drivers[counter].cores, fe_drivers[counter].cores);
         return fe_drivers[counter].cores;
      }
    counter ++;
   }
   /* todo do a z80 and 68k check to inform its not on the list if matched*/

   for (counter=0;counter<MAX_CPU;counter++)
   {
      unsigned int *type=(unsigned int *)&(Machine->drv->cpu[counter].cpu_type);

      if (*type==CPU_Z80)  log_cb(RETRO_LOG_INFO, "game:%s has no frontend_list.h match and has a z80  %s\n",name);
      if (*type==CPU_M68000) log_cb(RETRO_LOG_INFO, "game:%s has no frontend_list.h match and has a M68000  %s\n",name);
   }
   return 0;
}
#endif

static void configure_cyclone_mode (int driverIndex)
{
  /* Determine how to use cyclone if available to the platform */

  #if (HAS_CYCLONE || HAS_DRZ80)
  int i;
  int use_cyclone = 0;
  int use_drz80 = 0;
  int use_drz80_snd = 0;

   if (options.cyclone_mode == 6)
     i=check_list(drivers[driverIndex]->name);
   else
     i=options.cyclone_mode;
  /* ASM cores: 0=None,1=Cyclone,2=DrZ80,3=Cyclone+DrZ80,4=DrZ80(snd),5=Cyclone+DrZ80(snd) */
  switch (i)
  {
    /* nothing needs done for case 0 */
    case 1:
      use_cyclone = 1;
      break;

    case 2:
      use_drz80 = 1;
      break;

    case 3:
      use_cyclone = 1;
      use_drz80=1;
      break;

    case 4:
      use_drz80_snd = 1;
      break;

    case 5:
      use_cyclone = 1;
      use_drz80_snd = 1;
      break;

    default:
      break;
  }

#if (HAS_CYCLONE)
  /* Replace M68000 by CYCLONE */
  if (use_cyclone)
  {
    for (i=0;i<MAX_CPU;i++)
    {
      unsigned int *type=(unsigned int *)&(Machine->drv->cpu[i].cpu_type);


      if (*type==CPU_M68000 || *type==CPU_M68010 )
      {
        *type=CPU_CYCLONE;
        log_cb(RETRO_LOG_INFO, LOGPRE "Replaced m68000 with CYCLONE\n");
      }
    }
  }
#endif
#define enable_z80 1

#if (HAS_DRZ80)
  /* Replace Z80 by DRZ80 */
  if (use_drz80 || use_drz80_snd)
  {
    for (i=0;i<MAX_CPU;i++)
    {
      unsigned int *type=(unsigned int *)&(Machine->drv->cpu[i].cpu_type);
      if (*type==CPU_Z80 && enable_z80)
      {
        if ( (use_drz80_snd) && (Machine->drv->cpu[i].cpu_flags&CPU_AUDIO_CPU) )
        {
          *type=CPU_DRZ80;
          log_cb(RETRO_LOG_INFO, LOGPRE "Replaced Z80 sound cpu\n");
        }
        else if (use_drz80)
        {
           *type=CPU_DRZ80;
           log_cb(RETRO_LOG_INFO, LOGPRE "Replaced Z80 cpu\n");
        }
      }
    }
   if (!enable_z80)
     log_cb(RETRO_LOG_INFO, LOGPRE "z80 replacement disabled due to interrupt issues for now\n");
  }
#endif

#endif
}
