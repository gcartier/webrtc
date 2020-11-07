#include <stdbool.h>
#include <stdio.h>

#include <rtc_base/logging.h>
#include <api/audio/audio_frame.h>
#include <modules/audio_processing/include/audio_processing.h>


#ifdef _WIN32
  #define SHARED_PUBLIC __declspec(dllexport)
#else
  #define SHARED_PUBLIC __attribute__ ((visibility ("default")))
#endif


extern "C" SHARED_PUBLIC int  ap_setup(int, bool, bool, bool);
extern "C" SHARED_PUBLIC void ap_delete();
extern "C" SHARED_PUBLIC void ap_delay(int);
extern "C" SHARED_PUBLIC int  ap_process_reverse(int, int, int16_t*);
extern "C" SHARED_PUBLIC int  ap_process(int, int, int16_t*);


webrtc::AudioProcessing* apm;


int ap_setup(int processing_rate, bool echo_cancel, bool noise_suppress, bool use_logging)
{
    webrtc::AudioProcessing::Config config;
    
    int err = 0;

    rtc::LogMessage::LogToDebug(use_logging ? rtc::LS_INFO : rtc::LS_NONE);
    
    config.pipeline.maximum_internal_processing_rate = processing_rate;
    
    config.echo_canceller.enabled = echo_cancel;
    config.echo_canceller.mobile_mode = false;
    config.noise_suppression.enabled = noise_suppress;
    // config.noise_suppression.level = webrtc::AudioProcessing::Config::NoiseSuppression::kVeryHigh;
    
    apm = webrtc::AudioProcessingBuilder().Create();
    apm->ApplyConfig(config);

    if ((err = apm->Initialize()) < 0)
      return err;

    return err;
}


void ap_delete()
{
    delete apm;
    apm = NULL;
}


void ap_delay(int delay)
{
    apm->set_stream_delay_ms(delay);
}


int ap_process_reverse(int rate, int channels, int16_t* data)
{
    webrtc::StreamConfig config(rate, channels, false);
    
    return apm->ProcessReverseStream(data, config, config, data);
}


int ap_process(int rate, int channels, int16_t* data)
{
    webrtc::StreamConfig config(rate, channels, false);
    
    return apm->ProcessStream(data, config, config, data);
}
