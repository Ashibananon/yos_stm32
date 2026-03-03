/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <string.h>
#include <libopencm3/cm3/cortex.h>
#include "../yos/yos.h"
#include "../yos/ymutex.h"
#include "../../lib/yrenga/yringbuffer.h"
#include "../../lib/yrenga/yqueue.h"
#include "../../lib/yfs/yfs.h"
#include "../../lib/yfs/yfs_data.h"
#include "yaudio.h"
#include "yiis.h"

#if (HAS_AUDIO_MODULE == 1)

static struct yaudio_player _yaudio_player;
static int volatile yaudio_player_run_flag = 0;
static int volatile yaudio_player_is_running = 0;

static struct yaudio_player_command _yaudio_player_cmd_list[YAUDIO_PLAYER_CMD_QUEUE_LENGTH];
static struct yqueue _yaudio_cmd_queue;

static struct yfs_file _yaudio_file;
static int volatile _yaudio_file_is_open = 0;
static struct yiis_ctrl *yaudio_iis_ctrl = NULL;

/*
 * Buffer size must be the number divisible by both the channel num
 * and byte num of bit depth
 */
#define _PCM_FRAME_BUFFER_SIZE			(4096)
#define _PCM_FRAME_BUFFER_COUNT			4
static char _pcm_frames_buffer[_PCM_FRAME_BUFFER_COUNT][_PCM_FRAME_BUFFER_SIZE];
static struct YRingBuffer _pcm_frame_rb;
static char pcm_frames[_PCM_FRAME_BUFFER_SIZE];

static void _pcm_frame_rb_enter_critical(void)
{
	yiis_dma_disable_interrupts(yaudio_iis_ctrl, YIIS_DMA_DIRECTION_RX);
	yiis_dma_disable_interrupts(yaudio_iis_ctrl, YIIS_DMA_DIRECTION_TX);
}

static void _pcm_frame_rb_leave_critical(void)
{
	yiis_dma_enable_interrupts(yaudio_iis_ctrl, YIIS_DMA_DIRECTION_RX);
	yiis_dma_enable_interrupts(yaudio_iis_ctrl, YIIS_DMA_DIRECTION_TX);
}

static void _clear_audio_player_status(struct yaudio_player *player)
{
	if (player == NULL) {
		return;
	}

	player->sampling_rate = 0;
	player->audio_file_duration_ms = 0;
	player->audio_file_played_ms = 0;
	player->volume_l = YAUDIO_PLAYER_DEFAULT_VOLUME;
	player->volume_r = YAUDIO_PLAYER_DEFAULT_VOLUME;
	player->channel = 0;
	player->audio_bit_depth = 0;
	player->transfer_bit_depth = 0;
	player->sample_num = 0;
	player->sample_played = 0;
}


#if (YAUDIO_PLAYER_WITH_LIBOPUS == 1)
static OggOpusFile *_ogg_file = NULL;

static int _yfs_op_read(void *_stream, unsigned char *_ptr, int _nbytes)
{
	YAUDIO_DBG("enter _yfs_op_read(stream[0x%08X], ptr[0x%08X], nbytes[%d])\n",
				_stream, _ptr, _nbytes);
	int ret = -1;
	if (_stream == NULL || _ptr == NULL || _nbytes < 0) {
		goto para_err;
	}
	struct yfs_file *fp = (struct yfs_file *)(_stream);
	ret = yfs_fread(YFS_Data, fp, _ptr, _nbytes);
	YAUDIO_DBG("yfs_fread returns [%d]\n", ret);

para_err:
	YAUDIO_DBG("leave _yfs_op_read, ret[%d]\n", ret);
	return ret;
}

static int _yfs_op_seek(void *_stream, opus_int64 _offset, int _whence)
{
	YAUDIO_DBG("enter _yfs_op_seek(stream[0x%08X], offset[%ld], whence[%d])\n",
				_stream, _offset, _whence);
	int ret = -1;
	if (_stream == NULL) {
		goto para_err;
	}

	struct yfs_file *fp = (struct yfs_file *)(_stream);
	enum yfs_whence_flags wf;
	if (_whence == SEEK_SET) {
		wf = YFS_SEEK_SET;
	} else if (_whence == SEEK_CUR) {
		wf = YFS_SEEK_CUR;
	} else if (_whence == SEEK_END) {
		wf = YFS_SEEK_END;
	} else {
		goto para_err;
	}

	ret = yfs_fseek(YFS_Data, fp, _offset, wf);
	YAUDIO_DBG("yfs_fseek return[%d]\n", ret);

para_err:
	YAUDIO_DBG("leave _yfs_op_seek, ret[%d]\n", ret);
	return ret;
}

static opus_int64 _yfs_op_tell(void *_stream)
{
	YAUDIO_DBG("enter _yfs_op_tell(stream[0x%08X])\n", _stream);
	opus_int64 ret = -1;
	if (_stream == NULL) {
		goto para_err;
	}

	struct yfs_file *fp = (struct yfs_file *)(_stream);
	ret = yfs_ftell(YFS_Data, fp);
	YAUDIO_DBG("yfs_ftell return[%ld]\n", ret);

para_err:
	YAUDIO_DBG("leave _yfs_op_tell, ret[%ld]\n", ret);
	return ret;
}

static int _yfs_op_close(void *_stream)
{
	YAUDIO_DBG("enter _yfs_op_close(stream[0x%08X])\n", _stream);
	int ret = -1;
	if (_stream == NULL) {
		goto para_err;
	}

	struct yfs_file *fp = (struct yfs_file *)(_stream);
	ret = yfs_fclose(YFS_Data, fp);
	YAUDIO_DBG("yfs_fclose return[%d])\n", ret);

para_err:
	YAUDIO_DBG("leave _yfs_op_close, ret[%d])\n", ret);
	return ret;
}

static struct OpusFileCallbacks _opus_file_cbs_on_yfs = {
	.read = _yfs_op_read,
	.seek = _yfs_op_seek,
	.tell = _yfs_op_tell,
	.close = _yfs_op_close
};
#endif


#if (YAUDIO_PLAYER_WITH_DR_LIBS == 1)
#define DR_WAV_IMPLEMENTATION
//#define DR_WAV_NO_CONVERSION_API
#define DR_WAV_NO_STDIO
#define DR_WAV_NO_WCHAR
#include <dr_wav.h>

static drwav _drwav_obj;

static size_t _drwav_read_proc(void *pUserData, void *pBufferOut, size_t bytesToRead)
{
	//YAUDIO_DBG("enter _drwav_read_proc(pUserData[0x%08X], pBufferOut[0x%08X], bytesToRead[%d])\n",
	//			pUserData, pBufferOut, bytesToRead);
	size_t ret = 0;
	if (pUserData == NULL || pBufferOut == NULL) {
		goto para_err;
	}
	struct yfs_file *fp = (struct yfs_file *)(pUserData);
	ret = yfs_fread(YFS_Data, fp, pBufferOut, bytesToRead);
	//YAUDIO_DBG("yfs_fread returns [%d]\n", ret);

para_err:
	//YAUDIO_DBG("leave _drwav_read_proc, ret[%d]\n", ret);
	return ret;
}

static size_t _drwav_write_proc(void *pUserData, const void *pData, size_t bytesToWrite)
{
	//YAUDIO_DBG("enter _drwav_write_proc(pUserData[0x%08X], pData[0x%08X], bytesToWrite[%d])\n",
	//			pUserData, pData, bytesToWrite);
	size_t ret = 0;
	if (pUserData == NULL || pData == NULL) {
		goto para_err;
	}
	struct yfs_file *fp = (struct yfs_file *)(pUserData);
	ret = yfs_fwrite(YFS_Data, fp, pData, bytesToWrite);
	//YAUDIO_DBG("yfs_fread returns [%d]\n", ret);

para_err:
	//YAUDIO_DBG("leave _drwav_write_proc, ret[%d]\n", ret);
	return ret;
}

static drwav_bool32 _drwav_seek_proc(void *pUserData, int offset, drwav_seek_origin origin)
{
	//YAUDIO_DBG("enter _drwav_seek_proc(pUserData[0x%08X], offset[%ld], origin[%d])\n",
	//			pUserData, offset, origin);
	drwav_bool32 ret = DRWAV_FALSE;
	if (pUserData == NULL) {
		goto para_err;
	}

	struct yfs_file *fp = (struct yfs_file *)(pUserData);
	enum yfs_whence_flags wf;
	if (origin == DRWAV_SEEK_SET) {
		wf = YFS_SEEK_SET;
	} else if (origin == DRWAV_SEEK_CUR) {
		wf = YFS_SEEK_CUR;
	} else if (origin == DRWAV_SEEK_END) {
		wf = YFS_SEEK_END;
	} else {
		goto para_err;
	}

	int yfs_ret = yfs_fseek(YFS_Data, fp, offset, wf);
	//YAUDIO_DBG("yfs_fseek return[%d]\n", yfs_ret);
	if (yfs_ret == 0) {
		ret = DRWAV_TRUE;
	}

para_err:
	//YAUDIO_DBG("leave _drwav_seek_proc, ret[%d]\n", ret);
	return ret;
}

static drwav_bool32 _drwav_tell_proc(void *pUserData, drwav_int64 *pCursor)
{
	//YAUDIO_DBG("enter _drwav_tell_proc(pUserData[0x%08X], pCursor[0x%08X])\n",
	//			pUserData, pCursor);
	drwav_bool32 ret = DRWAV_FALSE;
	if (pUserData == NULL || pCursor == NULL) {
		goto para_err;
	}

	struct yfs_file *fp = (struct yfs_file *)(pUserData);
	int64_t yfs_ret = yfs_ftell(YFS_Data, fp);
	//YAUDIO_DBG("yfs_ftell return[%ld]\n", yfs_ret);
	if (yfs_ret >= 0) {
		ret = DRWAV_TRUE;
		*pCursor = yfs_ret;
	}

para_err:
	//YAUDIO_DBG("leave _drwav_tell_proc, ret[%ld]\n", ret);
	return ret;
}
#endif



static int _yaudio_check_file_format(char *filename)
{
	int ret = -1;
	if (filename == NULL) {
		goto para_err;
	}

#if (YAUDIO_PLAYER_WITH_LIBOPUS == 1)
	if (is_string_end_with(filename, ".opus")) {
		ret = 0;
	}
#elif (YAUDIO_PLAYER_WITH_DR_LIBS == 1)
	if (is_string_end_with(filename, ".wav")) {
		ret = 0;
	}
#endif

para_err:
	return ret;
}


static int _yaudio_player_task(void *para)
{
	struct yaudio_player *yplayer = (struct yaudio_player *)para;
	struct yaudio_player_command cmd;

#if (YAUDIO_PLAYER_WITH_DR_LIBS == 1)
	drwav_uint64 pcm_frame_processed;
	uint32_t pcm_frame_2_process;
#elif (YAUDIO_PLAYER_WITH_LIBOPUS == 1)
	int pcm_frame_processed;
	int pcm_frame_2_process;
#endif

	uint32_t pcm_data_io_times_l;
	uint32_t pcm_data_io_times_h;
	uint32_t pcm_data_io_wait_times_l;
	uint32_t pcm_data_io_wait_times_h;

	_clear_audio_player_status(yplayer);

	yplayer->is_running = 1;
	while (yaudio_player_run_flag) {
		if (yqueue_try_receive_items(&_yaudio_cmd_queue, &cmd, 1) == 1) {
			switch (cmd.cmd) {
			case YAUDIO_PLAYER_CMD_PLAY:
				yplayer->status = YAUDIO_PLAYER_STATUS_PLAYING;
				YAUDIO_DBG("Receive cmd: play\n");
				break;
			case YAUDIO_PLAYER_CMD_RECORD:
				yplayer->status = YAUDIO_PLAYER_STATUS_RECORDING;
				YAUDIO_DBG("Receive cmd: record\n");
				break;
			case YAUDIO_PLAYER_CMD_PAUSE:
				yplayer->status = YAUDIO_PLAYER_STATUS_PAUSED;
				YAUDIO_DBG("Receive cmd: pause\n");
				break;
			case YAUDIO_PLAYER_CMD_STOP:
				yplayer->status = YAUDIO_PLAYER_STATUS_IDLE;
				YAUDIO_DBG("Receive cmd: stop\n");
				break;
			case YAUDIO_PLAYER_CMD_CHANGE_VOLUME:
				yplayer->volume_l = cmd.para.volume.left;
				yplayer->volume_r = cmd.para.volume.right;
				YAUDIO_DBG("Receive cmd: change volume to (%d, %d)\n",
							cmd.para.volume.left, cmd.para.volume.right);
				break;
			case YAUDIO_PLAYER_CMD_SET_AUDIO_FILE:
				if (yplayer->status == YAUDIO_PLAYER_STATUS_IDLE) {
					if (_yaudio_check_file_format(cmd.para.audio_file) != 0) {
						YAUDIO_DBG("File format not support\n");
					} else {
						strncpy(yplayer->audio_file, cmd.para.audio_file, sizeof(yplayer->audio_file));
						yplayer->audio_file[sizeof(yplayer->audio_file) - 1] = '\0';
						_clear_audio_player_status(yplayer);

						YAUDIO_DBG("Receive cmd: set audio file to [%s]\n", cmd.para.audio_file);
					}
				} else {
					YAUDIO_DBG("Set audio file to [%s] failed, please stop playing first\n",
								cmd.para.audio_file);
				}
				break;
			default:
				YAUDIO_DBG("Receive unknown cmd(%d)\n", cmd.cmd);
				break;
			}
		}

		if (yplayer->status == YAUDIO_PLAYER_STATUS_IDLE) {
			if (_yaudio_file_is_open) {
#if (YAUDIO_PLAYER_WITH_LIBOPUS == 1)
				op_free(_ogg_file);
				_ogg_file = NULL;
#elif (YAUDIO_PLAYER_WITH_DR_LIBS == 1)
				drwav_uninit(&_drwav_obj);
#endif
				yfs_fclose(YFS_Data, &_yaudio_file);
				_yaudio_file_is_open = 0;
				YAUDIO_DBG("File[%s] closed\n", yplayer->audio_file);

				yiis_dma_end(yaudio_iis_ctrl, YIIS_DMA_DIRECTION_RX);
				yiis_dma_end(yaudio_iis_ctrl, YIIS_DMA_DIRECTION_TX);
				yiis_deinit(yaudio_iis_ctrl);

				YAUDIO_DBG("pcm data io times: %d:%d\n",
							pcm_data_io_times_h, pcm_data_io_times_l);
				YAUDIO_DBG("pcm data io wait times: %d:%d\n",
							pcm_data_io_wait_times_h, pcm_data_io_wait_times_l);
#if (IIS_DMA_WAIT_STATICSTIC == 1)
				YAUDIO_DBG("dma data wait times: %d:%d\n",
							yaudio_iis_ctrl->dma_buffer_not_ready_h,
							yaudio_iis_ctrl->dma_buffer_not_ready_l);
#endif
			}
		} else if (yplayer->status == YAUDIO_PLAYER_STATUS_PLAYING
				|| yplayer->status == YAUDIO_PLAYER_STATUS_RECORDING) {
			if (!_yaudio_file_is_open) {
				int status;
				if (yplayer->status == YAUDIO_PLAYER_STATUS_PLAYING) {
					status = yfs_fopen(YFS_Data,
									&_yaudio_file,
									yplayer->audio_file,
									YFS_O_RDONLY);
				} else if (yplayer->status == YAUDIO_PLAYER_STATUS_RECORDING) {
					status = yfs_fopen(YFS_Data,
									&_yaudio_file,
									yplayer->audio_file,
									YFS_O_WRONLY | YFS_O_EXCL | YFS_O_CREAT);
				} else {
					status = -1;
				}

				if (status == 0) {
					YAUDIO_DBG("File[%s] open ok\n", yplayer->audio_file);
#if (YAUDIO_PLAYER_WITH_LIBOPUS == 1)
					if (yplayer->status == YAUDIO_PLAYER_STATUS_PLAYING) {
						_ogg_file = op_open_callbacks(&_yaudio_file,
													&_opus_file_cbs_on_yfs,
												NULL, 0, NULL);
						if (_ogg_file != NULL) {
							/* Open ogg file ok */
							_yaudio_file_is_open = 1;
							YAUDIO_DBG("Open ogg file [%s] OK\n", yplayer->audio_file);
						} else {
							YAUDIO_DBG("Open ogg file [%s] failed\n", yplayer->audio_file);
							yfs_fclose(YFS_Data, &_yaudio_file);
							yplayer->status = YAUDIO_PLAYER_STATUS_IDLE;
						}
					} else if (yplayer->status == YAUDIO_PLAYER_STATUS_RECORDING) {
						/* Not implemented */
					} else {
						/* Error */
					}
#elif (YAUDIO_PLAYER_WITH_DR_LIBS == 1)
					drwav_bool32 init_result;
					if (yplayer->status == YAUDIO_PLAYER_STATUS_PLAYING) {
						init_result = drwav_init(&_drwav_obj,
												_drwav_read_proc,
												_drwav_seek_proc,
												_drwav_tell_proc,
												&_yaudio_file,
												NULL);
					} else if (yplayer->status == YAUDIO_PLAYER_STATUS_RECORDING) {
						drwav_data_format fmt;
						fmt.container = drwav_container_riff;
						fmt.format = DR_WAVE_FORMAT_PCM;
						fmt.channels = YAUDIO_PLAYER_DEFAULT_RECORDING_CHANNEL;
						fmt.sampleRate = YAUDIO_PLAYER_DEFAULT_RECORDING_SAMPLING_RATE;
						fmt.bitsPerSample = YAUDIO_PLAYER_DEFAULT_RECORDING_BIT_DEPTH;
						init_result = drwav_init_write(&_drwav_obj,
													&fmt,
													_drwav_write_proc,
													_drwav_seek_proc,
													&_yaudio_file,
													NULL);
					} else {
						init_result = DRWAV_FALSE;
					}

					if (init_result == DRWAV_TRUE) {
						_yaudio_file_is_open = 1;
						YAUDIO_DBG("Open wav file [%s] OK\n", yplayer->audio_file);
					} else {
						YAUDIO_DBG("Open wav file [%s] failed\n", yplayer->audio_file);
						yfs_fclose(YFS_Data, &_yaudio_file);
						yplayer->status = YAUDIO_PLAYER_STATUS_IDLE;
					}
#endif

					if (_yaudio_file_is_open) {
						_clear_audio_player_status(yplayer);
						enum yiis_dma_direction dir = YIIS_DMA_DIRECTION_UNKNOWN;

#if (YAUDIO_PLAYER_WITH_LIBOPUS == 1)
						if (yplayer->status == YAUDIO_PLAYER_STATUS_PLAYING) {
							yplayer->sampling_rate = 48000;
							yplayer->channel = op_channel_count(_ogg_file, -1);
							yplayer->audio_bit_depth = 16;
							yplayer->sample_num = op_pcm_total(_ogg_file, -1);
						} else if (yplayer->status == YAUDIO_PLAYER_STATUS_RECORDING) {
							/* Not implemented */
						} else {
							/* Error */
						}
#elif (YAUDIO_PLAYER_WITH_DR_LIBS == 1)
						if (yplayer->status == YAUDIO_PLAYER_STATUS_PLAYING) {
							yplayer->sampling_rate = _drwav_obj.sampleRate;
							yplayer->channel = _drwav_obj.channels;
							yplayer->channel_length_bits = _drwav_obj.bitsPerSample;
							yplayer->audio_bit_depth = _drwav_obj.bitsPerSample;
							yplayer->sample_num = _drwav_obj.totalPCMFrameCount;
							yaudio_iis_ctrl = YIIS_2_CTRL;
							dir = YIIS_DMA_DIRECTION_TX;
						} else if (yplayer->status == YAUDIO_PLAYER_STATUS_RECORDING) {
							yplayer->sampling_rate = YAUDIO_PLAYER_DEFAULT_RECORDING_SAMPLING_RATE;
							yplayer->channel = YAUDIO_PLAYER_DEFAULT_RECORDING_CHANNEL;
							yplayer->channel_length_bits = YAUDIO_PLAYER_DEFAULT_RECORDING_CHANNEL_LENGTH_BIT;
							yplayer->audio_bit_depth = YAUDIO_PLAYER_DEFAULT_RECORDING_BIT_DEPTH;
							yplayer->sample_num = 0;
							yaudio_iis_ctrl = YIIS_3_CTRL;
							dir = YIIS_DMA_DIRECTION_RX;
						} else {
							/* Error */
						}
#endif

						YAUDIO_DBG("Audio info:\n");
						//YAUDIO_DBG("  format: [%d]\n", _drwav_obj.translatedFormatTag);
						YAUDIO_DBG("  channels: [%d]\n", yplayer->channel);
						YAUDIO_DBG("  channels length: [%d]\n", yplayer->channel_length_bits);
						YAUDIO_DBG("  sample rate: [%d]\n", yplayer->sampling_rate);
						YAUDIO_DBG("  bit depth: [%d]\n", yplayer->audio_bit_depth);
						YAUDIO_DBG("  sample num: [%u][%u]\n",
									(uint32_t)(yplayer->sample_num >> 32),
									(uint32_t)(yplayer->sample_num & 0xFFFFFFFF));

						pcm_data_io_times_h = 0;
						pcm_data_io_times_l = 0;
						pcm_data_io_wait_times_h = 0;
						pcm_data_io_wait_times_l = 0;

						yiis_init(yaudio_iis_ctrl);
						if (yiis_config(yaudio_iis_ctrl, dir,
									yplayer->sampling_rate,
									yplayer->channel, yplayer->audio_bit_depth,
									yplayer->channel_length_bits,
									IIS_AUDIO_STANDARD_PHILIPS_STANDARD) != 0) {
							YAUDIO_DBG("iis dma config failed\n");
							yplayer->status = YAUDIO_PLAYER_STATUS_IDLE;
						} else {
							if (yplayer->audio_bit_depth == 16) {
								yplayer->transfer_bit_depth = 16;
							} else if (yplayer->audio_bit_depth == 24) {
								yplayer->transfer_bit_depth = 32;
							} else if (yplayer->audio_bit_depth == 32) {
								yplayer->transfer_bit_depth = 32;
							}

							yiis_dma_start(yaudio_iis_ctrl, dir,
										&_pcm_frame_rb, YRingBufferGetItemSize(&_pcm_frame_rb));
						}
					}
				} else {
					YAUDIO_DBG("File[%s] open failed\n", yplayer->audio_file);
					yplayer->status = YAUDIO_PLAYER_STATUS_IDLE;
				}
			} else {
				/* Audio file is open, read and play */
				pcm_frame_2_process = sizeof(pcm_frames)
									/ (yplayer->transfer_bit_depth / 8)
									/ yplayer->channel;
#if (YAUDIO_PLAYER_WITH_LIBOPUS == 1)
				if (yplayer->status == YAUDIO_PLAYER_STATUS_PLAYING) {
					int frame_read_once;
					pcm_frame_processed = 0;
					while (pcm_frame_processed < pcm_frame_2_process) {
						frame_read_once = op_read_stereo(_ogg_file,
														pcm_frames + pcm_frame_processed * (yplayer->transfer_bit_depth / 8) * yplayer->channel,
														sizeof(pcm_frames) - pcm_frame_processed * (yplayer->transfer_bit_depth / 8) * yplayer->channel);
						if (frame_read_once > 0) {
							pcm_frame_processed += frame_read_once;
						} else if (frame_read_once == 0){
							/* EOF */
							YAUDIO_DBG("EOF reached\n");
							break;
						} else {
							/* Error */
							yplayer->status = YAUDIO_PLAYER_STATUS_IDLE;
							YAUDIO_DBG("File read error\n");
						}
					}
				}
#elif (YAUDIO_PLAYER_WITH_DR_LIBS == 1)
				if (yplayer->status == YAUDIO_PLAYER_STATUS_PLAYING) {
					if (yplayer->transfer_bit_depth == 16) {
						pcm_frame_processed = drwav_read_pcm_frames_s16(&_drwav_obj,
																pcm_frame_2_process,
																pcm_frames);
					} else if (yplayer->transfer_bit_depth == 24) {
						pcm_frame_processed = drwav_read_pcm_frames_s32(&_drwav_obj,
																pcm_frame_2_process,
																pcm_frames);
					} else if (yplayer->transfer_bit_depth == 32) {
						pcm_frame_processed = drwav_read_pcm_frames_s32(&_drwav_obj,
																pcm_frame_2_process,
																pcm_frames);
					}
					if (pcm_frame_processed < pcm_frame_2_process) {
						YAUDIO_DBG("EOF reached\n");
					}
					//YAUDIO_DBG("Read %ld pcm frames, expected %d\n", (uint32_t)pcm_frame_processed, _PCM_FRAME_COUNT_ONCE);
					if (pcm_frame_processed == pcm_frame_2_process) {
					} else if (pcm_frame_processed < pcm_frame_2_process) {
						/* EOF reached */
						memset(pcm_frames + pcm_frame_processed * (yplayer->transfer_bit_depth / 8) * yplayer->channel,
								0x00,
								(pcm_frame_2_process - pcm_frame_processed) * (yplayer->transfer_bit_depth / 8) * yplayer->channel);

						yplayer->status = YAUDIO_PLAYER_STATUS_IDLE;
					}
					yplayer->sample_played += pcm_frame_processed;

					pcm_data_io_times_l++;
					if (pcm_data_io_times_l == 0) {
						pcm_data_io_times_h++;
					}

					if (yplayer->transfer_bit_depth == 32) {
						uint32_t index;
						uint16_t *part1, *part2;
						uint16_t tmp_v;
						for (index = 0; index < sizeof(pcm_frames); index += yplayer->transfer_bit_depth / 8) {
							part1 = pcm_frames + index;
							part2 = pcm_frames + index + sizeof(*part1);
							tmp_v = *part1;
							*part1 = *part2;
							*part2 = tmp_v;
						}
					}
				}
#endif

				if (yplayer->status == YAUDIO_PLAYER_STATUS_PLAYING) {
					yiis_dma_restore(yaudio_iis_ctrl, YIIS_DMA_DIRECTION_TX);
					while (yiis_transfer_data(yaudio_iis_ctrl, (uint8_t *)pcm_frames, sizeof(pcm_frames)) < 0) {
						//YAUDIO_DBG("write audio data failed, wait to retry...\n");
						pcm_data_io_wait_times_l++;
						if (pcm_data_io_wait_times_l == 0) {
							pcm_data_io_wait_times_h++;
						}
						yos_task_delay(1);
					}
				} else if (yplayer->status == YAUDIO_PLAYER_STATUS_RECORDING) {
					yiis_dma_restore(yaudio_iis_ctrl, YIIS_DMA_DIRECTION_RX);
					while (yiis_receive_data(yaudio_iis_ctrl, (uint8_t *)pcm_frames, sizeof(pcm_frames)) < 0) {
						//YAUDIO_DBG("write audio data failed, wait to retry...\n");
						pcm_data_io_wait_times_l++;
						if (pcm_data_io_wait_times_l == 0) {
							pcm_data_io_wait_times_h++;
						}
						yos_task_delay(1);
					}
#if (YAUDIO_PLAYER_WITH_LIBOPUS == 1)
#elif (YAUDIO_PLAYER_WITH_DR_LIBS == 1)
#if 0
					if (yplayer->transfer_bit_depth == 32) {
						uint32_t index;
						uint16_t *part1, *part2;
						uint16_t tmp_v;
						for (index = 0; index < sizeof(pcm_frames); index += yplayer->transfer_bit_depth / 8) {
							part1 = pcm_frames + index;
							part2 = pcm_frames + index + sizeof(*part1);
							tmp_v = *part1;
							*part1 = *part2;
							*part2 = tmp_v;
						}
					}
#endif
					pcm_frame_processed = drwav_write_pcm_frames(&_drwav_obj,
																pcm_frame_2_process,
																pcm_frames);
					yplayer->sample_played += pcm_frame_processed;
					pcm_data_io_times_l++;
					if (pcm_data_io_times_l == 0) {
						pcm_data_io_times_h++;
					}
					if (pcm_frame_processed < pcm_frame_2_process) {
						YAUDIO_DBG("Writing pcm frames error: [%u] expected but [%u:%u] written\n",
									pcm_frame_2_process,
									(uint32_t)(pcm_frame_processed >> 32),
									(uint32_t)(pcm_frame_processed & 0xFFFFFFFF));
					}
#endif
				} else {
					/* Error */
				}
			}
		} else if (yplayer->status == YAUDIO_PLAYER_STATUS_PAUSED) {
			yiis_dma_pause(yaudio_iis_ctrl, YIIS_DMA_DIRECTION_TX);
			yiis_dma_pause(yaudio_iis_ctrl, YIIS_DMA_DIRECTION_RX);
		} else {

		}

		yos_task_delay(1);
	}
	yplayer->is_running = 0;

	return 0;
}

int yaudio_player_start(void)
{
	int ret = -1;
	if (yaudio_player_is_running) {
		goto already_started;
	}

	if (yqueue_init(&_yaudio_cmd_queue,
					_yaudio_player_cmd_list,
					sizeof(_yaudio_player_cmd_list[0]),
					YAUDIO_PLAYER_CMD_QUEUE_LENGTH) != 0) {
		goto queue_init_err;
	}

	if (YRingBufferInit(&_pcm_frame_rb, _pcm_frames_buffer,
					sizeof(_pcm_frames_buffer[0]),
					sizeof(_pcm_frames_buffer) / sizeof(_pcm_frames_buffer[0]),
					_pcm_frame_rb_enter_critical,
					_pcm_frame_rb_leave_critical) != 0) {
		goto pcm_rb_err;
	}

#if 0
	if (yiis_init(YIIS_2_CTRL) != 0) {
		goto iis_init_err;
	}
#endif

	yaudio_player_run_flag = 1;

	if (yos_create_task(_yaudio_player_task,
						&_yaudio_player,
						YAUDIO_PLAYER_TASK_STACK_SIZE,
						YAUDIO_PLAYER_TASK_NAME) < 0 ) {
		goto task_err;
	}

	ret = 0;
	return ret;

task_err:
#if 0
	yiis_deinit(YIIS_2_CTRL);
#endif
iis_init_err:
	YRingBufferDestory(&_pcm_frame_rb);
pcm_rb_err:
	yqueue_deinit(&_yaudio_cmd_queue);
queue_init_err:
already_started:
	return ret;
}

int yaudio_player_end(void)
{
	//yqueue_deinit(&_yaudio_cmd_queue);

	return -1;
}


int yaudio_play(void)
{
	int ret = -1;
	struct yaudio_player_command cmd;
	cmd.cmd = YAUDIO_PLAYER_CMD_PLAY;
	if (yqueue_try_send_items(&_yaudio_cmd_queue, &cmd, 1) == 1) {
		ret = 0;
	}

	return ret;
}

int yaudio_record(void)
{
	int ret = -1;
	struct yaudio_player_command cmd;
	cmd.cmd = YAUDIO_PLAYER_CMD_RECORD;
	if (yqueue_try_send_items(&_yaudio_cmd_queue, &cmd, 1) == 1) {
		ret = 0;
	}

	return ret;
}

int yaudio_pause(void)
{
	int ret = -1;
	struct yaudio_player_command cmd;
	cmd.cmd = YAUDIO_PLAYER_CMD_PAUSE;
	if (yqueue_try_send_items(&_yaudio_cmd_queue, &cmd, 1) == 1) {
		ret = 0;
	}

	return ret;
}

int yaudio_stop(void)
{
	int ret = -1;
	struct yaudio_player_command cmd;
	cmd.cmd = YAUDIO_PLAYER_CMD_STOP;
	if (yqueue_try_send_items(&_yaudio_cmd_queue, &cmd, 1) == 1) {
		ret = 0;
	}

	return ret;
}

int yaudio_change_volume(uint8_t volume_l, uint8_t volume_r)
{
	int ret = -1;
	struct yaudio_player_command cmd;
	cmd.cmd = YAUDIO_PLAYER_CMD_CHANGE_VOLUME;
	cmd.para.volume.left = volume_l;
	cmd.para.volume.right = volume_r;
	if (yqueue_try_send_items(&_yaudio_cmd_queue, &cmd, 1) == 1) {
		ret = 0;
	}

	return ret;
}

int yaudio_set_audio_file(char *audio_file_path)
{
	int ret = -1;
	if (audio_file_path == NULL) {
		goto para_err;
	}

	struct yaudio_player_command cmd;
	cmd.cmd = YAUDIO_PLAYER_CMD_SET_AUDIO_FILE;
	strncpy(cmd.para.audio_file, audio_file_path, sizeof(cmd.para.audio_file));
	cmd.para.audio_file[sizeof(cmd.para.audio_file) - 1] = '\0';
	if (yqueue_try_send_items(&_yaudio_cmd_queue, &cmd, 1) == 1) {
		ret = 0;
	}

para_err:
	return ret;
}

int yaudio_get_status(struct yaudio_player *player)
{
	int ret = -1;
	if (player == NULL) {
		goto para_err;
	}

	memcpy(player, &_yaudio_player, sizeof(_yaudio_player));
	ret = 0;

para_err:
	return ret;
}

#endif
