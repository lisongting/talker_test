#include <iostream>
#include <string.h>
#include <stdlib.h>
#include "AIUITest.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "Talker.h"
#include "speech_recognizer.h"
using namespace std;
using namespace aiui;
using namespace rapidjson;

#define FRAME_LEN	640
#define	BUFFER_SIZE	4096

void writeText(string s);
void stopAIUI();
void on_result(const char *result, char is_last);
void on_speech_begin();
void on_speech_end(int reason);
void start_recording(const char* session_begin_params);

Talker talker;
ISpeechUtility* speechUtility;
char *g_result = NULL;
unsigned int g_buffersize = BUFFER_SIZE;
bool isInteracting = false;

class MyListener : public IAIUIListener{
public:
    void onEvent(IAIUIEvent& event);
};

//回调函数:当回答语音播放完成时，开启录音进行下一轮对话
void onPlayFinished(){
    cout<<"TTS  Finished"<<endl;
    isInteracting = false;
}
MyListener listener;
IAIUIAgent *agent ;

int main(){
     while(true){
         if(!isInteracting){
             //callback  function  of start_recording :  on_result
             start_recording(session_record_begin_params);
             //cout<<"---------------logout()  while"<<endl;
             MSPLogout();
         }

     }

	return 0;
}

void MyListener::onEvent(IAIUIEvent& event){
    switch (event.getEventType()) {
    case AIUIConstant::EVENT_STATE:
        {
            switch (event.getArg1()) {
            case AIUIConstant::STATE_IDLE:
                {
                    cout << "EVENT_STATE:" << "IDLE" << endl;

                } break;

            case AIUIConstant::STATE_READY:
                {
                    cout << "EVENT_STATE:" << "READY" << endl;

                } break;

            case AIUIConstant::STATE_WORKING:
                {
                    cout << "EVENT_STATE:" << "WORKING" << endl;
                } break;
            }
        } break;

    case AIUIConstant::EVENT_WAKEUP:
        {

            cout << "EVENT_WAKEUP :" << event.getInfo() << endl;

        } break;

    case AIUIConstant::EVENT_SLEEP:
        {
            cout << "EVENT_SLEEP:arg1=" << event.getArg1() << endl;
        } break;

    case AIUIConstant::EVENT_VAD:
    {
            switch (event.getArg1()) {
            case AIUIConstant::VAD_BOS:
                {
                    cout << "EVENT_VAD:" << "BOS" << endl;
                } break;

            case AIUIConstant::VAD_EOS:
                {
                    cout << "EVENT_VAD:" << "EOS" << endl;
                } break;

            case AIUIConstant::VAD_VOL:
                {
                    //						cout << "EVENT_VAD:" << "VOL" << endl;
                } break;
            }
        } break;

    case AIUIConstant::EVENT_RESULT:
        {
            // cout<<"EVENT_RESULT arg:("<<event.getArg1()<<","<<event.getArg2()<<")"<<" result:"<<event.getInfo()<<endl;
            Buffer* buffer = event.getData()->getBinary("0");
            string data ;
            if(NULL!=buffer){
                data = string((char*)buffer->data());
                Document json;
                json.Parse(data.c_str());
                cout<<"data:"<<data<<endl;
                string result;
                Value& s = json["intent"];
                if(s.HasMember("answer")){
                    Value& s1 = s["answer"];
                    assert(s1.HasMember("text"));
                    result =  s1["text"].GetString();
                    cout<<"Response:  ----------> "<<result<<endl;
                    stopAIUI();
                    talker.init();
                    talker.talk((char*)result.c_str(),onPlayFinished);
                }else{
                    result = "这个问题有点难,换一个吧";
                    cout<<"Response:  ----------> "<<result<<endl;
                    stopAIUI();
                    //talker.playHard(onPlayFinished);
                    talker.init();
                    talker.talk((char*)result.c_str(),onPlayFinished);
                }

            }
        }
        break;

    case AIUIConstant::EVENT_ERROR:
        {
            cout << "EVENT_ERROR:" << event.getArg1() << endl;
            if(event.getArg1()==10111){
               stopAIUI();
               start_recording(session_record_begin_params);
               //cout<<"10111  login ret:"<<ret<<endl;
            }
        } break;
    }
}

//发送文本进行aiui文本对话
void writeText(string s){
     isInteracting = true;
    AIUISetting::setLogLevel(info);
    speechUtility = ISpeechUtility::createSingleInstance("", "",
        "appid=5a52e95f");
    string paramStr = FileUtil::readFileAsString("aiui.cfg");

    agent = IAIUIAgent::createAgent(paramStr.c_str(), &listener);
    cout<<"Create IAIUIAgent Success\n"<<endl;
    //start agent
    if(agent!=NULL){
        IAIUIMessage *startMsg = IAIUIMessage::create(AIUIConstant::CMD_START);
        agent->sendMessage(startMsg);
        startMsg->destroy();
    }else{
        cout<<"Agent is null"<<endl;
    }
    if(agent!=NULL){
        IAIUIMessage *wakeMsg = IAIUIMessage::create(AIUIConstant::CMD_WAKEUP);
        agent->sendMessage(wakeMsg);
        wakeMsg->destroy();
        cout<<"WAKEUP"<<endl;;
    }
    if(NULL!=agent){
        Buffer* textData = Buffer::alloc(s.length());
        s.copy((char*)textData->data() ,s.length());
        IAIUIMessage* writeMsg = IAIUIMessage::create(AIUIConstant::CMD_WRITE,0,0,"data_type=text",textData);
        agent->sendMessage(writeMsg);
        cout<<"Send: -----------------> "<<s<<endl;
        writeMsg->destroy();

    }
}

//开始录音，其目的是将用户说的话转换成文字
void start_recording(const char* session_begin_params){

    int ret = MSPLogin(NULL, NULL, login_params);
//     cout<<"---------------login()   start_recording"<<endl;
    if (MSP_SUCCESS != ret)	{
        cout<<"MSPLogin failed , Error code  "<<ret<<endl;
    }
   cout<<"\nYou can  speak to me(record  for 10 seconds) : "<<endl;
   int errcode;
   int i =0;
   struct speech_rec iat;
   struct speech_rec_notifier recnotifier = {
       on_result,
       on_speech_begin,
       on_speech_end
   };

   errcode = sr_init(&iat, session_begin_params, SR_MIC, &recnotifier);
   if (errcode) {
       cout<<"Speech recognizer init failed   code:"<<errcode<<endl;
       return;
   }

   errcode = sr_start_listening(&iat);
   if (errcode) {
       cout<<"Start listen failed  code:"<<errcode<<endl;
   }

   /* record for 10 seconds */
   for(i=0;i<10;i++){
       sleep(1);
   }
   cout<<endl;

   errcode = sr_stop_listening(&iat);
   if (errcode) {
       cout<<"Stop listening failed  code:"<<errcode<<endl;
   }

   sr_uninit(&iat);
}

//语音识别的结果回调
void on_result(const char *result, char is_last){
    cout<<"Speech recognition result: "<<result<<endl;
    MSPLogout();
//     cout<<"---------------logout()  on_result"<<endl;
    if (result) {
        size_t left = g_buffersize - 1 - strlen(g_result);
        size_t size = strlen(result);

        if (left < size) {
            g_result = (char*)realloc(g_result, g_buffersize + BUFFER_SIZE);
            if (g_result)
                g_buffersize += BUFFER_SIZE;
            else {
                cout<<"Memory alloc failed"<<endl;
                return;
            }
        }

        strncat(g_result, result, size);


        writeText(result);
    }

}

//语音识别开始
void on_speech_begin(){
    cout<<"Start listening........"<<endl;
    if (g_result){
        free(g_result);
    }
    g_result = (char*)malloc(BUFFER_SIZE);
    g_buffersize = BUFFER_SIZE;
    memset(g_result, 0, g_buffersize);
}

//语音识别结束
void on_speech_end(int reason){
     cout<<"On speech end -- code:"<<reason<<endl;

    if (reason == END_REASON_VAD_DETECT){
        cout<<"Speaking done.  ";
    }else{
         isInteracting = false;
    }

    if(reason==10114){
        cout<<"网络请求超时"<<endl;
    }else if(reason==10108){

    }



}

//停止AIUI交互
void stopAIUI(){
    if (NULL != agent)
    {
        IAIUIMessage *stopMsg = IAIUIMessage::create (AIUIConstant::CMD_STOP);
        agent->sendMessage(stopMsg);
        stopMsg->destroy();
        agent->destroy();

        speechUtility->destroy();
    }
}

