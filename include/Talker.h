#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "qtts.h"
#include "msp_cmn.h"
#include "msp_errors.h"

#define ALSA_MAX_BUF_SIZE 65535
/* wav音频头部格式 */
typedef struct _wave_pcm_hdr
{
        char            riff[4];                // = "RIFF"
        int		size_8;                 // = FileSize - 8
        char            wave[4];                // = "WAVE"
        char            fmt[4];                 // = "fmt "
        int		fmt_size;		// = 下一个结构体的大小 : 16
        short int       format_tag;             // = PCM : 1
        short int       channels;               // = 通道数 : 1
        int		samples_per_sec;        // = 采样率 : 8000 | 6000 | 11025 | 16000
        int		avg_bytes_per_sec;      // = 每秒字节数 : samples_per_sec * bits_per_sample / 8
        short int       block_align;            // = 每采样点字节数 : wBitsPerSample / 8
        short int       bits_per_sample;        // = 量化比特数: 8 | 16
        char            data[4];                // = "data";
        int		data_size;              // = 纯数据长度 : FileSize - 44
} wave_pcm_hdr;


///* 默认wav音频头部数据 */
static wave_pcm_hdr default_wav_hdr =
{
        { 'R', 'I', 'F', 'F' },
        0,
        {'W', 'A', 'V', 'E'},
        {'f', 'm', 't', ' '},
        16,
        1,
        1,
        16000,
        32000,
        2,
        16,
        {'d', 'a', 't', 'a'},
        0
};
static char  login_params[50]  = "appid = 5a52e95f, work_dir = .";//登录参数,appid与msc库绑定
	/*
	* rdn:           合成音频数字发音方式
	* volume:        合成音频的音量
	* pitch:         合成音频的音调
	* speed:         合成音频对应的语速
	* voice_name:    合成发音人
	* sample_rate:   合成音频采样率
	* text_encoding: 合成文本编码格式
	*/
static char session_tts_begin_params[200]  = "voice_name = xiaoyan, text_encoding = utf8, sample_rate = 16000, speed = 50, volume = 50, pitch = 50, rdn = 2";
static char filename[20]             = "audio.wav"; //合成的语音文件名称

static char session_record_begin_params[200] =
                "sub = iat, domain = iat, language = zh_cn, "
                "accent = mandarin, sample_rate = 16000, "
                "result_type = plain, result_encoding = utf8";
typedef void  (*on_play_finished)();

class Talker {
public: 
	Talker();
	~Talker();
	void init();
        void talk(char*  text,on_play_finished  callback);
private:
        int textToAudioFile(char* text);
        int play(on_play_finished  callback);
	
};

