/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/*
 *  All these are same with modem definitions
 *  and, from definitions of headers in modem's source codes.
 * 
*/


#ifndef RDA8810_MODEM_CODEC_ADP_H
#define RDA8810_MODEM_CODEC_ADP_H
// ============================================================================
// SND_SPK_LEVEL_T
// -----------------------------------------------------------------------------
/// This type describes the possible level value for the speaker volume.
// =============================================================================
typedef enum
{
    SND_SPK_MUTE                                = 0x00000000,
    SND_SPK_VOL_1                               = 0x00000001,
    SND_SPK_VOL_2                               = 0x00000002,
    SND_SPK_VOL_3                               = 0x00000003,
    SND_SPK_VOL_4                               = 0x00000004,
    SND_SPK_VOL_5                               = 0x00000005,
    SND_SPK_VOL_6                               = 0x00000006,
    SND_SPK_VOL_7                               = 0x00000007,
    SND_SPK_VOL_QTY                             = 0x00000008
} SND_SPK_LEVEL_T;


// ============================================================================
// SND_MIC_LEVEL_T
// -----------------------------------------------------------------------------
/// This type describes the possible level value for the mic: mute or enabled.
// =============================================================================
typedef enum
{
    SND_MIC_MUTE                                = 0x00000000,
    SND_MIC_ENABLE                              = 0x00000001,
    SND_MIC_VOL_QTY                             = 0x00000002
} SND_MIC_LEVEL_T;


// ============================================================================
// SND_SIDE_LEVEL_T
// -----------------------------------------------------------------------------
/// This type describes the possible level value for the side tone volume. The value
/// SND_SIDE_VOL_TEST is used for mic to spk simple loop back test.
// =============================================================================
typedef enum
{
    SND_SIDE_MUTE                               = 0x00000000,
    SND_SIDE_VOL_1                              = 0x00000001,
    SND_SIDE_VOL_2                              = 0x00000002,
    SND_SIDE_VOL_3                              = 0x00000003,
    SND_SIDE_VOL_4                              = 0x00000004,
    SND_SIDE_VOL_5                              = 0x00000005,
    SND_SIDE_VOL_6                              = 0x00000006,
    SND_SIDE_VOL_7                              = 0x00000007,
    SND_SIDE_VOL_TEST                           = 0x00000008,
    SND_SIDE_VOL_QTY                            = 0x00000009
} SND_SIDE_LEVEL_T;


// ============================================================================
// SND_TONE_ATTENUATION_T
// -----------------------------------------------------------------------------
/// Attenuation of the tone. The attenuation can be set to 0 dB, -3 dB, -9 dB and
/// -15dB.
// =============================================================================
typedef enum
{
/// No attenuation
    SND_TONE_0DB                                = 0x00000000,
/// -3dB
    SND_TONE_M3DB                               = 0x00000001,
/// -9db
    SND_TONE_M9DB                               = 0x00000002,
/// -15dB
    SND_TONE_M15DB                              = 0x00000003,
    SND_TONE_VOL_QTY                            = 0x00000004
} SND_TONE_ATTENUATION_T;


// ============================================================================
// SND_TONE_TYPE_T
// -----------------------------------------------------------------------------
/// Tone types. The DTMF Tones are used to inform the user that the number is being
/// composed. All the standard DTMF are available: 0 to 9, A to D, pound and star.
/// \n The Comfort Tones are used to inform the user on the current state of the
/// call: Ringing, Busy, Unavailable... All frequencies needed to do the standard
/// Comfort Tones are available: 425 Hz, 950 Hz, 1400 Hz and 1800 Hz. \n
// =============================================================================
typedef enum
{
/// Tone when the '0' key
    SND_DTMF_0                                  = 0x00000000,
/// Tone when the '1' key
    SND_DTMF_1                                  = 0x00000001,
/// Tone when the '2' key
    SND_DTMF_2                                  = 0x00000002,
/// Tone when the '3' key
    SND_DTMF_3                                  = 0x00000003,
/// Tone when the '4' key
    SND_DTMF_4                                  = 0x00000004,
/// Tone when the '5' key
    SND_DTMF_5                                  = 0x00000005,
/// Tone when the '6' key
    SND_DTMF_6                                  = 0x00000006,
/// Tone when the '7' key
    SND_DTMF_7                                  = 0x00000007,
/// Tone when the '8' key
    SND_DTMF_8                                  = 0x00000008,
/// Tone when the '9' key
    SND_DTMF_9                                  = 0x00000009,
    SND_DTMF_A                                  = 0x0000000A,
    SND_DTMF_B                                  = 0x0000000B,
    SND_DTMF_C                                  = 0x0000000C,
    SND_DTMF_D                                  = 0x0000000D,
/// Tone when the * key
    SND_DTMF_S                                  = 0x0000000E,
/// Tone when the # key
    SND_DTMF_P                                  = 0x0000000F,
/// Comfort tone at 425 Hz
    SND_COMFORT_425                             = 0x00000010,
/// Comfort tone at 950 Hz
    SND_COMFORT_950                             = 0x00000011,
/// Comfort tone at 1400 Hz
    SND_COMFORT_1400                            = 0x00000012,
/// Confort tone at 1800 Hz
    SND_COMFORT_1800                            = 0x00000013,
/// No tone is emitted
    SND_NO_TONE                                 = 0x00000014
} SND_TONE_TYPE_T;


// ============================================================================
// SND_ITF_T
// -----------------------------------------------------------------------------
/// That type provide a way to identify the different audio interface.
// =============================================================================
typedef enum
{
    SND_ITF_RECEIVER                            = 0x00000000,
    SND_ITF_EAR_PIECE                           = 0x00000001,
    SND_ITF_LOUD_SPEAKER                        = 0x00000002,
    SND_ITF_BLUETOOTH                           = 0x00000003,
    SND_ITF_FM                                  = 0x00000004,
    SND_ITF_TV                                  = 0x00000005,
/// Number (max) of available interface to the SND driver
    SND_ITF_QTY                                 = 0x00000006,
    SND_ITF_NONE                                = 0x000000FF
} SND_ITF_T;


// ============================================================================
// SND_EQUALIZER_MODE_T
// -----------------------------------------------------------------------------
/// SND equalizer modes enumerator
// =============================================================================
typedef enum
{
    SND_EQUALIZER_OFF                           = 0xFFFFFFFF,
    SND_EQUALIZER_NORMAL                        = 0x00000000,
    SND_EQUALIZER_BASS                          = 0x00000001,
    SND_EQUALIZER_DANCE                         = 0x00000002,
    SND_EQUALIZER_CLASSICAL                     = 0x00000003,
    SND_EQUALIZER_TREBLE                        = 0x00000004,
    SND_EQUALIZER_PARTY                         = 0x00000005,
    SND_EQUALIZER_POP                           = 0x00000006,
    SND_EQUALIZER_ROCK                          = 0x00000007,
    SND_EQUALIZER_CUSTOM                        = 0x00000008,
    SND_EQUALIZER_QTY                           = 0x00000009
} SND_EQUALIZER_MODE_T;

//////////////////////////////////////////////////////////////////////////////////
// =============================================================================
// HAL_AIF_HANDLER_T
// -----------------------------------------------------------------------------
/// Type use to define the user handling function called when an interrupt 
/// related to the IFC occurs. Interrupt can be generated when playing or
/// recording a buffer, when we reach the middle or the end of the buffer.
// =============================================================================
// typedef VOID (*HAL_AIF_HANDLER_T)(VOID);
typedef void (*HAL_AIF_HANDLER_T)(void);

// =============================================================================
// HAL_AIF_SR_T
// -----------------------------------------------------------------------------
///  Audio stream sample rate. 
// =============================================================================
typedef enum
{
    HAL_AIF_FREQ_8000HZ  =  8000,
    HAL_AIF_FREQ_11025HZ = 11025,
    HAL_AIF_FREQ_12000HZ = 12000,
    HAL_AIF_FREQ_16000HZ = 16000,
    HAL_AIF_FREQ_22050HZ = 22050,
    HAL_AIF_FREQ_24000HZ = 24000,
    HAL_AIF_FREQ_32000HZ = 32000,
    HAL_AIF_FREQ_44100HZ = 44100,
    HAL_AIF_FREQ_48000HZ = 48000
} HAL_AIF_FREQ_T;



// =============================================================================
// HAL_AIF_CH_NB_T
// -----------------------------------------------------------------------------
/// This type defines the number of channels used by a stream
// ============================================================================
typedef enum
{
    HAL_AIF_MONO = 1,
    HAL_AIF_STEREO = 2,
} HAL_AIF_CH_NB_T;

// =============================================================================
// HAL_AIF_STREAM_T
// -----------------------------------------------------------------------------
/// This type describes a stream, played or record.
// =============================================================================
typedef struct
{
    // UINT32* startAddress;
    unsigned int* startAddress;
    /// Stream length in bytes.
    /// The length of a stream must be a multiple of 16 bytes.
    /// The maximum size is 32 KB.
    // UINT16 length;
	unsigned short length;
    HAL_AIF_FREQ_T sampleRate;
    HAL_AIF_CH_NB_T channelNb;
    /// True if this is a voice stream, coded on 13 bits, mono, at 8kHz.
    /// Voice quality streams are the only that can be output through 
    /// the receiver interface.
    // BOOL voiceQuality;
    unsigned char voiceQuality;
    /// True if the play stream is started along with the record stream.
    /// In this case, the play stream will be configured at first,
    /// but it is not started until the record stream is ready to start.
    // BOOL playSyncWithRecord;
    unsigned char playSyncWithRecord;
    /// Handler called when the middle of the buffer is reached.
    /// If this field is left blank (NULL), no interrupt will be
    /// generated.
    HAL_AIF_HANDLER_T halfHandler;
    /// Handler called when the end of the buffer is reached.
    /// If this field is left blank (NULL), no interrupt will be
    /// generated.
    HAL_AIF_HANDLER_T endHandler;
} HAL_AIF_STREAM_T;

///////////////////////////////////////////////////////////////////////////////////////

// =============================================================================
// AUD_SPK_T
// -----------------------------------------------------------------------------
/// Speaker output selection.
// =============================================================================
typedef enum
{
    /// Output on receiver
    /// This output can only use voice quality streams (Mono, 8kHz, 
    /// voiceQuality field set to TRUE).
    AUD_SPK_RECEIVER = 0,
    /// Output on ear-piece
    AUD_SPK_EAR_PIECE,
    /// Output on hand-free loud speaker
    AUD_SPK_LOUD_SPEAKER,
    /// Output on both hand-free loud speaker and ear-piece
    AUD_SPK_LOUD_SPEAKER_EAR_PIECE,

    AUD_SPK_QTY,

    AUD_SPK_DISABLE=255
} AUD_SPK_T;


// =============================================================================
// AUD_MIC_T
// -----------------------------------------------------------------------------
/// Microphone input selection. 
// =============================================================================
typedef enum
{
    /// Input from the regular microphone port
    AUD_MIC_RECEIVER = 0,
    /// Input from the ear-piece port,
    AUD_MIC_EAR_PIECE,
    /// Input from regular microphone, but for loudspeaker mode.
    AUD_MIC_LOUD_SPEAKER,

    AUD_MIC_QTY,

    AUD_MIC_DISABLE = 255
} AUD_MIC_T;


// =============================================================================
// AUD_SPEAKER_TYPE_T
// -----------------------------------------------------------------------------
/// Describes how the speaker is plugged on the stereo output:
///   - is it a stereo speaker ? (speakers ?)
///   - is it a mono speaker on the left channel ?
///   - is it a mono speaker on the right channel ?
///   - is this a mono output ?
// =============================================================================
typedef enum
{
    AUD_SPEAKER_STEREO,
    AUD_SPEAKER_MONO_RIGHT,
    AUD_SPEAKER_MONO_LEFT,
    /// The output is mono only.
    AUD_SPEAKER_STEREO_NA,

    AUD_SPEAKER_QTY
} AUD_SPEAKER_TYPE_T;


// =============================================================================
// AUD_LEVEL_T
// -----------------------------------------------------------------------------
/// Level configuration structure.
/// 
/// A level configuration structure allows to start an AUD operation (start 
/// stream, start record, or start tone) with the desired gains on an interface.
// =============================================================================
typedef struct
{
    /// Speaker level, 
    SND_SPK_LEVEL_T spkLevel;
    
    /// Microphone level: muted or enabled
    SND_MIC_LEVEL_T micLevel;
    
    /// Sidetone
    SND_SIDE_LEVEL_T sideLevel;

    SND_TONE_ATTENUATION_T toneLevel;

} AUD_LEVEL_T;

// =============================================================================
//  AUD_TEST_T
// -----------------------------------------------------------------------------
/// Used to choose the audio mode: normal, test, dai's ...
// =============================================================================
typedef enum
{
    /// No test mode.
    AUD_TEST_NO,

    /// For audio test loop; analog to DAI loop + DAI to analog loop.
    AUD_TEST_LOOP_ACOUSTIC,

    /// For audio test loop; radio loop on DAI interface
    AUD_TEST_LOOP_RF_DAI,

    /// For audio test loop; radio loop on analog interface
    AUD_TEST_LOOP_RF_ANALOG,

    /// For board check: mic input is fedback to the speaker output.
    AUD_TEST_SIDE_TEST,

    /// For board check; audio loop in VOC
    AUD_TEST_LOOP_VOC,

    /// For board check; regular mic input and earpiece output
    AUD_TEST_RECVMIC_IN_EARPIECE_OUT,

	/// headset mic in & headset out
	AUD_TEST_HEADSETMIC_IN_HEADSET_OUT,

	/// main mic in & receiver out
	AUD_TEST_MAINMIC_IN_RECEIVER_OUT,

	AUD_TEST_MODE_QTY
} AUD_TEST_T;

typedef enum
{
	AUD_APP_CODEC = 0,
	AUD_APP_FM,
	AUD_APP_ATV,

	AUD_APP_QTY,
} AUD_APP_MODE_T;

/* Mixer control names */
#define MIXER_PLAYBACK_VOLUME                    "Playback Volume"
#define MIXER_CAPTURE_VOLUME                     "Capture Volume"
#define MIXER_ITF                                "ITF"
#define MIXER_SPK_SEL                            "SpkSel"
#define MIXER_FORCE_MAINMIC                      "ForceMainMic"
#define MIXER_CODEC_APP_MODE                     "CodecAppMode"
#define MIXER_START_PLAY                         "StartPlay"
#define MIXER_START_RECORD                       "StartRecord"
#define MIXER_STOP                               "Stop"
#define MIXER_OUT_SAMPLE_RATE                    "OutSampleRate"
#define MIXER_IN_SAMPLE_RATE                     "InSampleRate"
#define MIXER_OUT_CHANNEL_NB                     "OutChannelNumber"
#define MIXER_IN_CHANNEL_NB                      "InChannelNumber"
#define MIXER_COMMIT_SETUP                       "Commit Setup"
#define MIXER_CODEC_OPEN_STATUS                  "CodecOpenStatus"
#define MIXER_MUTE_MIC                           "MuteMic"
#define MIXER_LOOP_MODE                          "Loop Mode"

#endif /* RDA8810_MODEM_CODEC_ADP_H */
