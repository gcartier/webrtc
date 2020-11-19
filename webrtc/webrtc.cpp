#include <stdbool.h>
#include <stdio.h>

#include <rtc_base/logging.h>
#include <api/audio/audio_frame.h>
#include <modules/audio_processing/include/audio_processing.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <glib.h>
#endif


#ifdef _WIN32
  #define SHARED_PUBLIC __declspec(dllexport)
#else
  #define SHARED_PUBLIC __attribute__ ((visibility ("default")))
#endif


extern "C" SHARED_PUBLIC void ap_setup(int, bool, bool, int, int);
extern "C" SHARED_PUBLIC void ap_delete();
extern "C" SHARED_PUBLIC void ap_delay(int);
extern "C" SHARED_PUBLIC int  ap_process_reverse(int, int, int16_t*);
extern "C" SHARED_PUBLIC int  ap_process(int, int, int16_t*);


static const webrtc::AudioProcessing::Config::NoiseSuppression::Level noise_suppression_levels[] = {
    webrtc::AudioProcessing::Config::NoiseSuppression::kLow,
    webrtc::AudioProcessing::Config::NoiseSuppression::kModerate,
    webrtc::AudioProcessing::Config::NoiseSuppression::kHigh,
    webrtc::AudioProcessing::Config::NoiseSuppression::kVeryHigh,
};


static const rtc::LoggingSeverity logging_severities[] = {
    rtc::LS_NONE,
    rtc::LS_ERROR,
    rtc::LS_WARNING,
    rtc::LS_INFO,
    rtc::LS_VERBOSE,
};


webrtc::AudioProcessing* apm = NULL;


#ifdef _WIN32
HANDLE mutex = NULL;
#else
GMutex mutex;
#endif
    
void lock_mutex()
{
#ifdef _WIN32
    if (! mutex)
        mutex = CreateMutex(NULL, FALSE, NULL);
    WaitForSingleObject(mutex, INFINITE);
#else
    g_mutex_lock(&mutex);
#endif
}
    
void unlock_mutex()
{
#ifdef _WIN32
    ReleaseMutex(mutex);
#else
    g_mutex_unlock(&mutex);
#endif
}


webrtc::AudioProcessing::Config config;


void ap_setup(int processing_rate, bool echo_cancel, bool noise_suppress, int noise_suppression_level, int logging_severity)
{
    lock_mutex();

    rtc::LogMessage::LogToDebug(logging_severities[logging_severity]);
    
    config.pipeline.maximum_internal_processing_rate = processing_rate;
    
    config.echo_canceller.enabled = echo_cancel;
    config.echo_canceller.mobile_mode = false;
    config.noise_suppression.enabled = noise_suppress;
    config.noise_suppression.level = noise_suppression_levels[noise_suppression_level];
    
    unlock_mutex();
}


void ap_create()
{
    apm = webrtc::AudioProcessingBuilder().Create();
    apm->ApplyConfig(config);

    apm->Initialize();
}


void ap_delete()
{
    lock_mutex();
    
    if (apm)
    {
        delete apm;
        apm = NULL;
    }
    
    unlock_mutex();
}


void ap_delay(int delay)
{
    lock_mutex();
    
    if (! apm)
        ap_create();
    
    apm->set_stream_delay_ms(delay);
    
    unlock_mutex();
}


int ap_process_reverse(int rate, int channels, int16_t* data)
{
    lock_mutex();
    
    if (! apm)
        ap_create();
    
    int err = 0;
    
    webrtc::StreamConfig config(rate, channels, false);
    
    err = apm->ProcessReverseStream(data, config, config, data);
    
    unlock_mutex();
    
    return err;
}


int ap_process(int rate, int channels, int16_t* data)
{
    lock_mutex();
    
    if (! apm)
        ap_create();
    
    int err = 0;
    
    webrtc::StreamConfig config(rate, channels, false);
    
    err = apm->ProcessStream(data, config, config, data);
    
    unlock_mutex();
    
    return err;
}
