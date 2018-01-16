#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include "qtts.h"
#include "msp_cmn.h"
#include "msp_errors.h"
#include "Talker.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/soundcard.h>
#include <alsa/asoundlib.h>
//TTS  : 用于将文字转换成声音文件(wav)，并进行播放

using namespace std;
Talker::Talker(){
}

Talker::~Talker(){
    MSPLogout(); //退出登录
}

void Talker::init(){

        int ret = MSPLogin(NULL, NULL, login_params);
        if (MSP_SUCCESS != ret){
            printf("MSPLogin failed, error code: %d.\n", ret);
            MSPLogout(); //退出登录
            exit(0);
        }
}

int Talker::textToAudioFile(char* text){
        int  ret  = -1;
        FILE*   fp  = NULL;
        const char*  sessionID = NULL;
        unsigned int audio_len  = 0;
        wave_pcm_hdr wav_hdr  = default_wav_hdr;
        int synth_status = MSP_TTS_FLAG_STILL_HAVE_DATA;
        if (NULL == text || NULL == filename){
            printf("params is error!\n");
            return ret;
        }
        fp = fopen(filename, "wb");
        if (NULL == fp){
            printf("open %s error.\n", text);
            return ret;
        }

        /* 开始合成 */
        sessionID = QTTSSessionBegin(session_tts_begin_params, &ret);

        if (MSP_SUCCESS != ret){
            printf("QTTSSessionBegin failed, error code: %d.\n", ret);
            fclose(fp);
            return ret;
        }
        ret = QTTSTextPut(sessionID, text, (unsigned int)strlen(text), NULL);
        if (MSP_SUCCESS != ret) {
            printf("QTTSTextPut failed, error code: %d.\n",ret);
            QTTSSessionEnd(sessionID, "TextPutError");
            fclose(fp);
            return ret;
        }
        printf("start composing ...\n");
        fwrite(&wav_hdr, sizeof(wav_hdr) ,1, fp); //添加wav音频头，使用采样率为16000
        while (1){
            /* 获取合成音频 */
            const void* data = QTTSAudioGet(sessionID, &audio_len, &synth_status, &ret);
            if (MSP_SUCCESS != ret)
                break;
            if (NULL != data) {
                fwrite(data, audio_len, 1, fp);
                wav_hdr.data_size += audio_len; //计算data_size大小
            }
            if (MSP_TTS_FLAG_DATA_END == synth_status)
                break;
            usleep(150*1000); //防止频繁占用CPU
        }

        if (MSP_SUCCESS != ret) {
            printf("QTTSAudioGet failed, error code: %d.\n",ret);
            QTTSSessionEnd(sessionID, "AudioGetError");
            fclose(fp);
            return ret;
        }

        /* 修正wav文件头数据的大小 */
        wav_hdr.size_8 += wav_hdr.data_size + (sizeof(wav_hdr) - 8);
        /* 将修正过的数据写回文件头部,音频文件为wav格式 */
        fseek(fp, 4, 0);
        fwrite(&wav_hdr.size_8,sizeof(wav_hdr.size_8), 1, fp); //写入size_8的值
        fseek(fp, 40, 0); //将文件指针偏移到存储data_size值的位置
        fwrite(&wav_hdr.data_size,sizeof(wav_hdr.data_size), 1, fp); //写入data_size的值
        fclose(fp);
        fp = NULL;
        /* 合成完毕 */
        ret = QTTSSessionEnd(sessionID, "Normal");
        if (MSP_SUCCESS != ret) {
            printf("QTTSSessionEnd failed, error code: %d.\n",ret);
        }
        return ret;
}

int Talker::play(on_play_finished  callback){
    cout<<"Start talking"<<endl;
    FILE*   fp  = NULL;

    int rc;
    int ret;
    int size;
    snd_pcm_t* handle; //PCI设备句柄
    snd_pcm_hw_params_t* params;//硬件信息和PCM流配置
    unsigned int val;
    int dir=0;
    snd_pcm_uframes_t frames;
    char *buffer;

    unsigned char ch[100]; //用来存储wav文件的头信息
    wave_pcm_hdr wave_header;
    fp=fopen(filename,"rb");
    if(!fp){
        perror("Cannot find audio file\n");
    }

     fread(&wave_header,1,sizeof(wave_header),fp);
     int channels=wave_header.channels;
     int frequency=wave_header.samples_per_sec;
     int bit=wave_header.bits_per_sample;
     int datablock=wave_header.block_align;

    rc=snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if(rc<0){
        perror("\nOpen PCM device failed:");
        exit(1);
    }

    snd_pcm_hw_params_alloca(&params); //分配params结构体
    if(rc<0)
    {
        perror("\nSnd_pcm_hw_params_alloca:");
        exit(1);
    }
    rc=snd_pcm_hw_params_any(handle, params);//初始化params
    if(rc<0) {
        perror("\nSnd_pcm_hw_params_any:");
        exit(1);
    }
    rc=snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED); //初始化访问权限
    if(rc<0){
        perror("\nSed_pcm_hw_set_access:");
        exit(1);

    }

    //采样位数
    switch(bit/8)
    {
    case 1:snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_U8);
        break ;
    case 2:snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
        break ;
    case 3:snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S24_LE);
        break ;

    }
    rc=snd_pcm_hw_params_set_channels(handle, params, channels); //设置声道,1表示单声>道，2表示立体声
    if(rc<0)
    {
        perror("\nSnd_pcm_hw_params_set_channels:");
        exit(1);
    }
    val = frequency;
    rc=snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir); //设置>频率
    if(rc<0)
    {
        perror("\nSnd_pcm_hw_params_set_rate_near:");
        exit(1);
    }

    rc = snd_pcm_hw_params(handle, params);
    if(rc<0)
    {
        perror("\nSnd_pcm_hw_params: ");
        exit(1);
    }

    rc=snd_pcm_hw_params_get_period_size(params, &frames, &dir); /*获取周期
    长度*/
    if(rc<0)
    {
        perror("\nSnd_pcm_hw_params_get_period_size:");
        exit(1);
    }

    size = frames * datablock; /*4 代表数据快长度*/

    buffer =(char*)malloc(size);
    fseek(fp,58,SEEK_SET); //定位歌曲到数据区

    while (1)
    {
        memset(buffer,0,sizeof(buffer));
        ret = fread(buffer, 1, size, fp);
        if(ret == 0){
            printf("talk finished\n");
            break;
        }else if (ret != size){
        }
        // 写音频数据到PCM设备
        while(ret = snd_pcm_writei(handle, buffer, frames)<0){
//            usleep(2000);
            if (ret == -EPIPE)
            {
                /* EPIPE means underrun */
                fprintf(stderr, "underrun occurred\n");
                //完成硬件参数设置，使设备准备好
                snd_pcm_prepare(handle);
            }
            else if (ret < 0)
            {
                fprintf(stderr,
                        "error from writei: %s\n",
                        snd_strerror(ret));
            }
        }

    }

    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);
    MSPLogout();
    callback();
    return 0;
}

void Talker::talk(char*  text,on_play_finished  callback){
    int ret;
    ret = textToAudioFile(text);
    if (MSP_SUCCESS != ret){
            printf("text_to_speech failed, error code: %d.\n", ret);
    }else{
    cout<<"Audio file composed successfully"<<endl;
    }
    play(callback);
}





