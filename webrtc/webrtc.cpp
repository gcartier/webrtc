#include <stdbool.h>
#include <stdio.h>

#include <rtc_base/logging.h>
#include <modules/audio_processing/include/audio_processing.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <glib.h>
#endif


// #define PROFILE_TIME


#ifdef _WIN32
  #define SHARED_PUBLIC __declspec(dllexport)
#else
  #define SHARED_PUBLIC __attribute__ ((visibility ("default")))
#endif


extern "C" SHARED_PUBLIC const char* ap_error(int);
extern "C" SHARED_PUBLIC void ap_setup(int, bool, bool, int, bool, int);
extern "C" SHARED_PUBLIC void ap_delete();
extern "C" SHARED_PUBLIC void ap_delay(int);
extern "C" SHARED_PUBLIC int ap_process_reverse(int, int, int16_t*);
extern "C" SHARED_PUBLIC int ap_process(int, int, int16_t*);


static const webrtc::AudioProcessing::Config::NoiseSuppression::Level noise_suppression_levels[] = {
    webrtc::AudioProcessing::Config::NoiseSuppression::kLow,
    webrtc::AudioProcessing::Config::NoiseSuppression::kModerate,
    webrtc::AudioProcessing::Config::NoiseSuppression::kHigh,
    webrtc::AudioProcessing::Config::NoiseSuppression::kVeryHigh,
};


static const rtc::LoggingSeverity logging_severities[] = {
    rtc::LS_VERBOSE,
    rtc::LS_INFO,
    rtc::LS_WARNING,
    rtc::LS_ERROR,
    rtc::LS_NONE,
};


bool apm_created = false;
rtc::scoped_refptr<webrtc::AudioProcessing> apm;


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
bool configured = false;

#ifdef PROFILE_TIME
gint64 reverse_time = 0;
gint64 process_time = 0;
#endif

const char* ap_error(int err)
{
  const char* str = "unknown error";

  switch (err) {
    case webrtc::AudioProcessing::kNoError:
      str = "success";
      break;
    case webrtc::AudioProcessing::kUnspecifiedError:
      str = "unspecified error";
      break;
    case webrtc::AudioProcessing::kCreationFailedError:
      str = "creating failed";
      break;
    case webrtc::AudioProcessing::kUnsupportedComponentError:
      str = "unsupported component";
      break;
    case webrtc::AudioProcessing::kUnsupportedFunctionError:
      str = "unsupported function";
      break;
    case webrtc::AudioProcessing::kNullPointerError:
      str = "null pointer";
      break;
    case webrtc::AudioProcessing::kBadParameterError:
      str = "bad parameter";
      break;
    case webrtc::AudioProcessing::kBadSampleRateError:
      str = "bad sample rate";
      break;
    case webrtc::AudioProcessing::kBadDataLengthError:
      str = "bad data length";
      break;
    case webrtc::AudioProcessing::kBadNumberChannelsError:
      str = "bad number of channels";
      break;
    case webrtc::AudioProcessing::kFileError:
      str = "file IO error";
      break;
    case webrtc::AudioProcessing::kStreamParameterNotSetError:
      str = "stream parameter not set";
      break;
    case webrtc::AudioProcessing::kNotEnabledError:
      str = "not enabled";
      break;
    case webrtc::AudioProcessing::kBadStreamParameterWarning:
      str = "bad stream parameter warning";
      break;
    default:
      break;
  }

  return str;
}


void ap_setup(int processing_rate, bool echo_cancel, bool noise_suppress, int noise_suppression_level, bool gain_controller, int logging_severity)
{
    lock_mutex();

    rtc::LogMessage::LogToDebug(logging_severities[logging_severity]);
    
    config.pipeline.maximum_internal_processing_rate = processing_rate;
    
    config.echo_canceller.enabled = echo_cancel;
    config.echo_canceller.mobile_mode = false;
    config.noise_suppression.enabled = noise_suppress;
    config.noise_suppression.level = noise_suppression_levels[noise_suppression_level];
    config.gain_controller1.enabled = gain_controller;
    config.residual_echo_detector.enabled = false;
    
    configured = true;
    
    unlock_mutex();
}


void ap_create()
{
    std::unique_ptr<webrtc::AudioProcessingBuilder> ap_builder = std::make_unique<webrtc::AudioProcessingBuilder>();
    apm = ap_builder->Create();
    
    apm->ApplyConfig(config);

    apm->Initialize();
    
    apm_created = true;
}


void ap_delete()
{
    lock_mutex();
    
    if (apm_created)
    {
        // not sure but I think the previous apm will
        // be released when the new one overrides it...
        apm_created = false;
    }
    
    unlock_mutex();
}


void ap_delay(int delay)
{
    lock_mutex();
    
    if (configured)
    {
        if (! apm_created)
            ap_create();
        
        apm->set_stream_delay_ms(delay);
    }
    
    unlock_mutex();
}


int ap_process_reverse(int rate, int channels, int16_t* data)
{
    lock_mutex();
    
    int err = 0;
    
    if (configured)
    {
        if (! apm_created)
            ap_create();
        
#ifdef PROFILE_TIME
        gint64 before = g_get_monotonic_time();
#endif
        
        webrtc::StreamConfig config(rate, channels);
        
        err = apm->ProcessReverseStream(data, config, config, data);
        
#ifdef PROFILE_TIME
        gint64 after = g_get_monotonic_time();
        reverse_time += (after - before);
        
        printf("reverse %.1fms\n", (double) (after - before) / 10);
#endif
    }
    
    unlock_mutex();
    
    return err;
}


int ap_process(int rate, int channels, int16_t* data)
{
    lock_mutex();
    
    int err = 0;
    
    if (configured)
    {
        if (! apm_created)
            ap_create();
        
#ifdef PROFILE_TIME
        gint64 before = g_get_monotonic_time();
#endif
        
        webrtc::StreamConfig config(rate, channels);
        
        err = apm->ProcessStream(data, config, config, data);
        
#ifdef PROFILE_TIME
        gint64 after = g_get_monotonic_time();
        process_time += (after - before);
        
        printf("process %.1fms\n", (double) (after - before) / 10);
#endif
    }
    
    unlock_mutex();
    
    return err;
}
