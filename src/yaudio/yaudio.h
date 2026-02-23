/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef __Y_AUDIO_H__
#define __Y_AUDIO_H__

#define YAUDIO_PLAYER_WITH_LIBOPUS		0
#define YAUDIO_PLAYER_WITH_DR_LIBS		1

#if (YAUDIO_PLAYER_WITH_LIBOPUS == 1 && YAUDIO_PLAYER_WITH_DR_LIBS == 1)
#error "Only one audio lib can be specified, libopus or dr_libs"
#endif

#include "../main_config.h"

#if (YAUDIO_PLAYER_WITH_LIBOPUS == 1)
#error "This device does NOT support OPUS files"
#include "../../lib/yaudio_libs/opusfile-0.12/include/opus/opusfile.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
#if (HAS_AUDIO_MODULE == 1)

#define YAUDIO_PLAYER_OUTPUT_DBG_MSG	1

#define YAUDIO_PLAYER_TASK_NAME			"yaudio"
#define YAUDIO_PLAYER_TASK_STACK_SIZE	(16384)
#define YAUDIO_PLAYER_CMD_QUEUE_LENGTH	(3)

#define YAUDIO_FILE_NAME_MAX_LENGTH		128
#define YAUDIO_PLAYER_DEFAULT_VOLUME	50

enum YAUDIO_PLAYER_STATUS {
	YAUDIO_PLAYER_STATUS_IDLE,
	YAUDIO_PLAYER_STATUS_PLAYING,
	YAUDIO_PLAYER_STATUS_PAUSED,
	YAUDIO_PLAYER_STATUS_MAX
};

struct yaudio_player {
	int is_running;
	enum YAUDIO_PLAYER_STATUS status;
	uint32_t sampling_rate;
	uint32_t audio_file_duration_ms;
	uint32_t audio_file_played_ms;
	uint8_t volume_l;
	uint8_t volume_r;
	uint8_t channel;
	uint8_t audio_bit_depth;
	uint8_t transfer_bit_depth;
	uint64_t sample_num;
	uint64_t sample_played;
	char audio_file[YAUDIO_FILE_NAME_MAX_LENGTH];
};

int yaudio_player_start(void);
/* Never call this right now, the task never ends */
int yaudio_player_end(void);


enum YAUDIO_PLAYER_CMD {
	YAUDIO_PLAYER_CMD_PLAY,
	YAUDIO_PLAYER_CMD_PAUSE,
	YAUDIO_PLAYER_CMD_STOP,
	YAUDIO_PLAYER_CMD_CHANGE_VOLUME,
	YAUDIO_PLAYER_CMD_SET_AUDIO_FILE,
	YAUDIO_PLAYER_CMD_MAX
};

struct yaudio_player_command {
	enum YAUDIO_PLAYER_CMD cmd;
	union yaudio_player_cmd_para {
		struct yaudio_player_volume {
			uint8_t left;
			uint8_t right;
		} volume;
		char audio_file[YAUDIO_FILE_NAME_MAX_LENGTH];
	} para;
};

int yaudio_play(void);
int yaudio_pause(void);
int yaudio_stop(void);
int yaudio_change_volume(uint8_t volume_l, uint8_t volume_r);
int yaudio_set_audio_file(char *audio_file_path);

int yaudio_get_status(struct yaudio_player *player);

#if (YAUDIO_PLAYER_OUTPUT_DBG_MSG == 1)
#include "../../lib/cmdline/basic_io.h"

#define YAUDIO_DBG(...)			basic_io_printf("[YAUDIO]" __VA_ARGS__);
#else
#define YAUDIO_DBG(...)
#endif

#endif
#ifdef __cplusplus
}
#endif
#endif
