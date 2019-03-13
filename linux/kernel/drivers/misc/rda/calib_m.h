//==============================================================================
//                                                                              
//            Copyright (C) 2012-2014, RDA Microelectronics.                    
//                            All Rights Reserved                               
//                                                                              
//      This source code is the property of RDA Microelectronics and is         
//      confidential.  Any  modification, distribution,  reproduction or        
//      exploitation  of  any content of this file is totally forbidden,        
//      except  with the  written permission  of   RDA Microelectronics.        
//                                                                              
//==============================================================================
//                                                                              
//    THIS FILE WAS GENERATED FROM ITS CORRESPONDING XML VERSION WITH COOLXML.  
//                                                                              
//                       !!! PLEASE DO NOT EDIT !!!                             
//                                                                              
//  $HeadURL$                                                                   
//  $Author$                                                                    
//  $Date$                                                                      
//  $Revision$                                                                  
//                                                                              
//==============================================================================
//
/// @file
/// <!-- TODO Add a complete comment header, with @mainpage, etc --> @defgroup calib
/// Calibration Driver
///  @{
/// 
//
//==============================================================================

#ifndef _CALIB_M_H_
#define _CALIB_M_H_


//#include "cs_types.h"

// =============================================================================
//  MACROS
// =============================================================================
/// Version of the calibration stub and code (on 8 bits).
#define CALIB_MAJ_VERSION                       (2)
#define CALIB_MIN_VERSION                       (5)
#define CALIB_MARK_VERSION                      (0XCA1B0000)
#define CALIB_VERSION_NUMBER                    (( CALIB_MARK_VERSION | CALIB_MAJ_VERSION << 8 | CALIB_MIN_VERSION ))
#define CALIB_STUB_VERSION                      (0XCA5B0001)
/// Number of calib timings for the transceiver.
#define CALIB_XCV_TIME_QTY                      (20)
/// Number of calib timings for the PA.
#define CALIB_PA_TIME_QTY                       (15)
/// Number of calib timings for the switch.
#define CALIB_SW_TIME_QTY                       (15)
/// Number of calib timings for PAL.
#define CALIB_PAL_TIME_QTY                      (20)
/// Number of generic parameters for the transceiver.
#define CALIB_XCV_PARAM_QTY                     (20)
/// Number of generic parameters for the PA.
#define CALIB_PA_PARAM_QTY                      (15)
/// Number of generic parameters for the switch.
#define CALIB_SW_PARAM_QTY                      (15)
/// Mask for transceiver RF name.
#define CALIB_XCV_MASK                          ((1 << 24))
/// Mask for PA RF name.
#define CALIB_PA_MASK                           ((1 << 25))
/// Mask for switch RF name.
#define CALIB_SW_MASK                           ((1 << 26))
#define CALIB_GSM_PCL_QTY                       (15)
#define CALIB_DCS_PCL_QTY                       (17)
#define CALIB_PCS_PCL_QTY                       (18)
#define CALIB_PADAC_PROF_INTERP_QTY             (16)
#define CALIB_PADAC_PROF_QTY                    (1024)
#define CALIB_PADAC_RAMP_QTY                    (32)
#define CALIB_LOW_VOLT_QTY                      (6)
/// Number of coefficiens in the MDF FIR filter.
#define CALIB_VOC_MDF_QTY                       (64)
/// Number of coefficiens in the SDF FIR filter.
#define CALIB_VOC_SDF_QTY                       (64)
/// Mask for echo cancelation enable (to be used with audio VoC enable).
#define CALIB_EC_ON                             ((1 << 0))
/// Mask for MDF FIR filter enable (to be used with audio VoC enable).
#define CALIB_MDF_ON                            ((1 << 1))
/// Mask for SDF FIR filter enable (to be used with audio VoC enable).
#define CALIB_SDF_ON                            ((1 << 2))
/// Number of audio gain steps.
#define CALIB_AUDIO_GAIN_QTY                    (8)
/// The audio gain value standing for mute.
#define CALIB_AUDIO_GAIN_VALUE_MUTE             (-128)
/// Number of misc audio parameters.
#define CALIB_AUDIO_PARAM_QTY                   (8)
/// Unrealistic values meaning that the power measure is not complete yet.
#define CALIB_STUB_SEARCH_POWER                 (0X0)
/// Unrealistic values meaning that the FOf measure is not complete yet.
#define CALIB_STUB_SEARCH_FOF                   (-2000000)
/// For communication between Calib Stub and calibration tools.
#define CALIB_STUB_XTAL_IDLE                    (-2000001)
/// For communication between Calib Stub and calibration tools.
#define CALIB_STUB_PA_PROF_IDLE                 (-2000002)
/// For communication between Calib Stub and calibration tools.
#define CALIB_STUB_ILOSS_IDLE                   (-128)
/// For communication between Calib Stub and calibration tools.
#define CALIB_STUB_DCO_IDLE                     (-32768)
/// For communication between Calib Stub and calibration tools.
#define CALIB_STUB_DCO_ERROR                    (-32767)
/// For communication between Calib Stub and calibration tools.
#define CALIB_STUB_GPADC_ERROR                  (0XFFFF)
/// For communication between Calib Stub and calibration tools.
#define CALIB_STUB_GPADC_IDLE                   (0XFFFE)
/// Number of cells used for measurement averages
#define CALIB_NB_CELLS                          (5)
/// Maximum number of different Audio Interfaces supported by this calibration structure.
/// The value of CALIB_AUDIO_ITF_QTY must be the same as AUD_ITF_QTY!
#define CALIB_AUDIO_ITF_QTY                     (6)
/// The number of GP ADC channels.
#define CALIB_GPADC_CH_QTY                      (4)
#define CALIB_GPADC_ACC_COUNT                   (8)
#define CALIB_GPADC_ACC_COUNT_MAX               (128)
#define CALIB_AUDIO_DICTA_REC                   (1)
#define CALIB_AUDIO_DICTA_PLAY                  (2)

// ============================================================================
// CALIB_METHOD_T
// -----------------------------------------------------------------------------
/// Calib process method type.
// =============================================================================
typedef enum
{
    CALIB_METH_DEFAULT                          = 0xCA11E700,
    CALIB_METH_COMPILATION                      = 0xCA11E701,
    CALIB_METH_MANUAL                           = 0xCA11E702,
    CALIB_METH_AUTOMATIC                        = 0xCA11E703,
    CALIB_METH_SIMULATION                       = 0xCA11E704
} CALIB_METHOD_T;


// ============================================================================
// CALIB_H_ENUM_0_T
// -----------------------------------------------------------------------------
/// Used for autonomous calib processes, results from Calib Stub to remote.
// =============================================================================
typedef enum
{
    CALIB_PROCESS_STOP                          = 0x00000000,
    CALIB_PROCESS_CONTINUE                      = 0x00000001,
    CALIB_PROCESS_PENDING                       = 0x00000002,
    CALIB_PROCESS_NEED_CALM                     = 0x00000003,
    CALIB_PROCESS_ERR_BAD_POW                   = 0x000000F0,
    CALIB_PROCESS_ERR_NO_MONO_POW               = 0x000000F1,
    CALIB_PROCESS_ERR_ZERO_DAC                  = 0x000000F2,
    CALIB_PROCESS_ERROR                         = 0x000000FF
} CALIB_H_ENUM_0_T;


// ============================================================================
// CALIB_STUB_BAND_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef enum
{
    CALIB_STUB_BAND_GSM850                      = 0x00000000,
    CALIB_STUB_BAND_GSM900                      = 0x00000001,
    CALIB_STUB_BAND_DCS1800                     = 0x00000002,
    CALIB_STUB_BAND_PCS1900                     = 0x00000003,
    CALIB_STUB_BAND_QTY                         = 0x00000004
} CALIB_STUB_BAND_T;


// ============================================================================
// CALIB_STUB_CMDS_T
// -----------------------------------------------------------------------------
/// Values used to define the contexts of the Calib Stub.
// =============================================================================
typedef enum
{
    CALIB_STUB_NO_STATE                         = 0x00000000,
    CALIB_STUB_MONIT_STATE                      = 0x00000001,
    CALIB_STUB_FOF_STATE                        = 0x00000002,
    CALIB_STUB_TX_STATE                         = 0x00000003,
    CALIB_STUB_PA_STATE                         = 0x00000004,
    CALIB_STUB_AUDIO_OUT                        = 0x00000005,
    CALIB_STUB_AUDIO_IN                         = 0x00000006,
    CALIB_STUB_AUDIO_SIDE                       = 0x00000007,
    CALIB_STUB_SYNCH_STATE                      = 0x00000008,
    CALIB_STUB_IDLE_STATE                       = 0x00000009
} CALIB_STUB_CMDS_T;


// ============================================================================
// CALIB_STUB_AFC_BOUND_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef enum
{
    CALIB_STUB_AFC_BOUND_CENTER                 = 0x00000000,
    CALIB_STUB_AFC_BOUND_NEG_FREQ               = 0x00000001,
    CALIB_STUB_AFC_BOUND_POS_FREQ               = 0x00000002,
    CALIB_STUB_AFC_BOUND_NO                     = 0x00000003
} CALIB_STUB_AFC_BOUND_T;

/// Value that defines the number of measure to do before the DC offset average is
/// considered valid.
#define CALIB_DCO_ACC_COUNT                     (32)
/// This magiv tag is used as a parameter to the boot loader to force is to run the
/// calibration stub
#define CALIB_MAGIC_TAG                         (0XCA1BCA1B)

// ============================================================================
// CALIB_COMMAND_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef enum
{
/// Command used by the Calibration Embedded Stub to inform HST that the command
/// is done.
    CALIB_CMD_DONE                              = 0xCA11B042,
    CALIB_CMD_NOT_ACCESSIBLE                    = 0xCA11B043,
    CALIB_CMD_UPDATE                            = 0xCA11B044,
    CALIB_CMD_UPDATE_ERROR                      = 0xCA11B045,
    CALIB_CMD_PA_PROFILE_GSM                    = 0xCA11B046,
    CALIB_CMD_PA_PROFILE_DCSPCS                 = 0xCA11B047,
    CALIB_CMD_RF_FLASH_BURN                     = 0xCA11B048,
    CALIB_CMD_FLASH_ERASE                       = 0xCA11B049,
    CALIB_CMD_FLASH_ERROR                       = 0xCA11B04A,
    CALIB_CMD_RF_RESET                          = 0xCA11B04B,
    CALIB_CMD_AUDIO_RESET                       = 0xCA11B04C,
    CALIB_CMD_RESET                             = 0xCA11B04D,
    CALIB_CMD_AUDIO_FLASH_BURN                  = 0xCA11B04E,
    CALIB_CMD_FLASH_BURN                        = 0xCA11B04F,
    CALIB_CMD_CFP_BURN                          = 0xCA11B050
} CALIB_COMMAND_T;


// ============================================================================
// CALIB_PARAM_STATUS_T
// -----------------------------------------------------------------------------
/// Calibration parameter type identifier.
// =============================================================================
typedef enum
{
    CALIB_PARAM_DEFAULT                         = 0xCA11B042,
    CALIB_PARAM_DEFAULT_RF_MIS                  = 0xCA11B043,
    CALIB_PARAM_INIT_ERROR                      = 0xCA10DEAD,
    CALIB_PARAM_RF_CALIBRATED                   = 0x007F0011,
    CALIB_PARAM_AUDIO_CALIBRATED                = 0x00A0D011,
    CALIB_PARAM_AUDIO_CALIBRATED_RF_MIS         = 0x00A0D043,
    CALIB_PARAM_CALIBRATED                      = 0x00DEF011
} CALIB_PARAM_STATUS_T;


// =============================================================================
//  TYPES
// =============================================================================

// ============================================================================
// CALIB_GLOBALS_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef struct
{
} CALIB_GLOBALS_T; //Size : 0x0



// ============================================================================
// CALIB_GPADC_ALL_CH_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef u16 CALIB_GPADC_ALL_CH_T[CALIB_GPADC_CH_QTY];


// ============================================================================
// CALIB_VERSION_TAG_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef u32 CALIB_VERSION_TAG_T;


// ============================================================================
// CALIB_OP_INFO_T
// -----------------------------------------------------------------------------
/// Calib process method and date type.
// =============================================================================
typedef struct
{
    u32                         date;                         //0x00000000
    CALIB_METHOD_T                 method;                       //0x00000004
} CALIB_OP_INFO_T; //Size : 0x8



// ============================================================================
// CALIB_XCV_TIMES_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef s16 CALIB_XCV_TIMES_T[CALIB_XCV_TIME_QTY];


// ============================================================================
// CALIB_PA_TIMES_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef s16 CALIB_PA_TIMES_T[CALIB_PA_TIME_QTY];


// ============================================================================
// CALIB_SW_TIMES_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef s16 CALIB_SW_TIMES_T[CALIB_SW_TIME_QTY];


// ============================================================================
// CALIB_PAL_TIMES_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef s16 CALIB_PAL_TIMES_T[CALIB_PAL_TIME_QTY];


// ============================================================================
// CALIB_XCV_PARAM_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef s32 CALIB_XCV_PARAM_T[CALIB_XCV_PARAM_QTY];


// ============================================================================
// CALIB_PA_PARAM_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef s32 CALIB_PA_PARAM_T[CALIB_PA_PARAM_QTY];


// ============================================================================
// CALIB_SW_PARAM_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef s32 CALIB_SW_PARAM_T[CALIB_SW_PARAM_QTY];


// ============================================================================
// CALIB_RF_CHIP_NAME_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef u32 CALIB_RF_CHIP_NAME_T;


// ============================================================================
// CALIB_RXTX_FREQ_OFFSET_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef s16 CALIB_RXTX_FREQ_OFFSET_T[CALIB_STUB_BAND_QTY];


// ============================================================================
// CALIB_RXTX_TIME_OFFSET_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef s16 CALIB_RXTX_TIME_OFFSET_T;


// ============================================================================
// CALIB_RXTX_IQ_TIME_OFFSET_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef s16 CALIB_RXTX_IQ_TIME_OFFSET_T;


// ============================================================================
// CALIB_DCO_CAL_TIME_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef s16 CALIB_DCO_CAL_TIME_T;


// ============================================================================
// CALIB_XCV_PALCUST_T
// -----------------------------------------------------------------------------
/// XCV PAL custom types.
// =============================================================================
typedef struct
{
    CALIB_RF_CHIP_NAME_T           name;                         //0x00000000
    CALIB_RXTX_FREQ_OFFSET_T       rxTxFreqOffset;               //0x00000004
    CALIB_RXTX_TIME_OFFSET_T       rxTxTimeOffset;               //0x0000000C
    CALIB_RXTX_IQ_TIME_OFFSET_T    rxIqTimeOffset;               //0x0000000E
    CALIB_RXTX_IQ_TIME_OFFSET_T    txIqTimeOffset;               //0x00000010
    CALIB_DCO_CAL_TIME_T           dcoCalTime;                   //0x00000012
    s32                          spare[7];                     //0x00000014
} CALIB_XCV_PALCUST_T; //Size : 0x30



// ============================================================================
// CALIB_ARFCN_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef s16 CALIB_ARFCN_T[2];


// ============================================================================
// CALIB_PCL2DBM_ARFCN_G_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef CALIB_ARFCN_T CALIB_PCL2DBM_ARFCN_G_T[CALIB_GSM_PCL_QTY];


// ============================================================================
// CALIB_PCL2DBM_ARFCN_D_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef CALIB_ARFCN_T CALIB_PCL2DBM_ARFCN_D_T[CALIB_DCS_PCL_QTY];


// ============================================================================
// CALIB_PCL2DBM_ARFCN_P_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef CALIB_ARFCN_T CALIB_PCL2DBM_ARFCN_P_T[CALIB_PCS_PCL_QTY];


// ============================================================================
// CALIB_PADAC_PROFILE_INTERP_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef u16 CALIB_PADAC_PROFILE_INTERP_T[CALIB_PADAC_PROF_INTERP_QTY];


// ============================================================================
// CALIB_PADAC_PROFILE_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef u16 CALIB_PADAC_PROFILE_T[CALIB_PADAC_PROF_QTY];


// ============================================================================
// CALIB_PADAC_PROFILE_EXTREM_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef s16 CALIB_PADAC_PROFILE_EXTREM_T;


// ============================================================================
// CALIB_PADAC_RAMP_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef u16 CALIB_PADAC_RAMP_T[CALIB_PADAC_RAMP_QTY];


// ============================================================================
// CALIB_PADAC_RAMP_SWAP_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef u16 CALIB_PADAC_RAMP_SWAP_T;


// ============================================================================
// CALIB_PADAC_LOW_VOLT_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef u16 CALIB_PADAC_LOW_VOLT_T[CALIB_LOW_VOLT_QTY];


// ============================================================================
// CALIB_PADAC_LOW_DAC_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef u16 CALIB_PADAC_LOW_DAC_T[CALIB_LOW_VOLT_QTY];


// ============================================================================
// CALIB_PA_PALCUST_T
// -----------------------------------------------------------------------------
/// PA PAL custom types.
// =============================================================================
typedef struct
{
    CALIB_RF_CHIP_NAME_T           name;                         //0x00000000
    CALIB_PCL2DBM_ARFCN_G_T        pcl2dbmArfcnG;                //0x00000004
    CALIB_PCL2DBM_ARFCN_D_T        pcl2dbmArfcnD;                //0x00000040
    CALIB_PCL2DBM_ARFCN_P_T        pcl2dbmArfcnP;                //0x00000084
    CALIB_PADAC_PROFILE_INTERP_T   profileInterpG;               //0x000000CC
    CALIB_PADAC_PROFILE_INTERP_T   profileInterpDp;              //0x000000EC
    CALIB_PADAC_PROFILE_T          profileG;                     //0x0000010C
    CALIB_PADAC_PROFILE_T          profileDp;                    //0x0000090C
    CALIB_PADAC_PROFILE_EXTREM_T   profileDbmMinG;               //0x0000110C
    CALIB_PADAC_PROFILE_EXTREM_T   profileDbmMinDp;              //0x0000110E
    CALIB_PADAC_PROFILE_EXTREM_T   profileDbmMaxG;               //0x00001110
    CALIB_PADAC_PROFILE_EXTREM_T   profileDbmMaxDp;              //0x00001112
    CALIB_PADAC_RAMP_T             rampLow;                      //0x00001114
    CALIB_PADAC_RAMP_T             rampHigh;                     //0x00001154
    CALIB_PADAC_RAMP_SWAP_T        rampSwapG;                    //0x00001194
    CALIB_PADAC_RAMP_SWAP_T        rampSwapDp;                   //0x00001196
    CALIB_PADAC_LOW_VOLT_T         lowVoltLimit;                 //0x00001198
    CALIB_PADAC_LOW_DAC_T          lowDacLimit;                  //0x000011A4
    s32                          spare[8];                     //0x000011B0
} CALIB_PA_PALCUST_T; //Size : 0x11D0



// ============================================================================
// CALIB_SW_PALCUST_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef struct
{
    CALIB_RF_CHIP_NAME_T           name;                         //0x00000000
    s32                          spare[8];                     //0x00000004
} CALIB_SW_PALCUST_T; //Size : 0x24



// ============================================================================
// CALIB_AUDIO_VOC_EC_T
// -----------------------------------------------------------------------------
/// Echo Cancelling for VoC.
// =============================================================================
typedef struct
{
    /// Enables Echo Canceller algorithm when >0.
    u8                          ecMu;                         //0x00000000
    /// Echo Canceller REL parameter (0,+16).
    u8                          ecRel;                        //0x00000001
    /// Echo Canceller MIN parameter (0, 32).
    u8                          ecMin;                        //0x00000002
    /// Enable Echo Suppressor algorithm (0,1).
    u8                          esOn;                         //0x00000003
    /// Double talk threshold for Echo Suppressor algo (0,32).
    u8                          esDtd;                        //0x00000004
    /// Donwlink vad threshold for Echo Suppressor algo (0,32).
    u8                          esVad;                        //0x00000005
    /// Enable for echo cancelling.
    u32                         enableField;                  //0x00000008
} CALIB_AUDIO_VOC_EC_T; //Size : 0xC



// ============================================================================
// CALIB_AUDIO_VOC_FILTERS_T
// -----------------------------------------------------------------------------
/// VoC anti-distortion filters.
// =============================================================================
typedef struct
{
    /// VoC needs the MDF coeffs to be 32-bit aligned.
    u16                         mdfFilter[CALIB_VOC_MDF_QTY]; //0x00000000
    /// SDF coeffs must stay right after MDF.
    u16                         sdfFilter[CALIB_VOC_SDF_QTY]; //0x00000080
} CALIB_AUDIO_VOC_FILTERS_T; //Size : 0x100

#define CALIB_AUDIO_IIR_GAIN_BAND_NUM 10

// ============================================================================
// CALIB_AUDIO_IN_GAINS_T
// -----------------------------------------------------------------------------
/// Input (MIC) gains.
// =============================================================================
typedef struct
{
    /// Input analog gain.
    s8                           ana;                          //0x00000000
    /// Input ADC gain.
    s8                           adc;                          //0x00000001
    /// Input algorithm gain.
    s8                           alg;                          //0x00000002
    /// Reserved.
    s8                           reserv;                       //0x00000003
} CALIB_AUDIO_IN_GAINS_T; //Size : 0x4

// ============================================================================
// CALIB_AUDIO_IIR_PARAM_ITF_T
// -----------------------------------------------------------------------------
// iir params
// =============================================================================
typedef struct
{
    s8                           gain[CALIB_AUDIO_IIR_GAIN_BAND_NUM]; //0x00000000
    u8                           qual[CALIB_AUDIO_IIR_GAIN_BAND_NUM]; //0x0000000A
    u16                          freq[CALIB_AUDIO_IIR_GAIN_BAND_NUM]; //0x00000014
} CALIB_AUDIO_IIR_PARAM_ITF_T; //Size : 0x28

typedef CALIB_AUDIO_IIR_PARAM_ITF_T CALIB_AUDIO_IIR_PARAM_T[CALIB_AUDIO_ITF_QTY];

// ============================================================================
// CALIB_AUDIO_OUT_GAINS_T
// -----------------------------------------------------------------------------
/// Output gains.
// =============================================================================
typedef struct
{
    /// Output voice gains for physical interfaces, or earpiece gains for application
    /// interfaces. Output analog gain.
    s8                           voiceOrEpAna;                 //0x00000000
    /// Output DAC gain.
    s8                           voiceOrEpDac;                 //0x00000001
    /// Output algorithm gain.
    s8                           voiceOrEpAlg;                 //0x00000002
    /// Reserved.
    s8                           reserv1;                      //0x00000003
    /// Output music gains for physical interfaces, or loudspeaker gains for application
    /// interfaces. Output analog gain.
    s8                           musicOrLsAna;                 //0x00000004
    /// Output DAC gain.
    s8                           musicOrLsDac;                 //0x00000005
    /// Output algorithm gain.
    s8                           musicOrLsAlg;                 //0x00000006
    /// Reserved.
    s8                           reserv2;                      //0x00000007
} CALIB_AUDIO_OUT_GAINS_T; //Size : 0x8



// ============================================================================
// CALIB_AUDIO_GAINS_T
// -----------------------------------------------------------------------------
/// Calib audio gain types.
// =============================================================================
typedef struct
{
    /// Params accessible by the API.
    CALIB_AUDIO_IN_GAINS_T         inGainsCall;                    //0x00000000
    /// Output gains.
    CALIB_AUDIO_OUT_GAINS_T        outGains[CALIB_AUDIO_GAIN_QTY]; //0x00000004
    s8                             sideTone[CALIB_AUDIO_GAIN_QTY]; //0x00000044
    /// change reserv to record in-gains, for record own and call own in-gains
    CALIB_AUDIO_IN_GAINS_T         inGains;                        //0x0000004c
    s8                             reserv[20];                     //0x00000050
} CALIB_AUDIO_GAINS_T; //Size : 0x64



// ============================================================================
// CALIB_AUDIO_PARAMS_T
// -----------------------------------------------------------------------------
/// Audio calibration parameters.
// =============================================================================
typedef struct
{
    u32                         reserv1;                      //0x00000000
    s8                           AecEnbleFlag;                 //0x00000004
    s8                           AgcEnbleFlag;                 //0x00000005
    s8                           StrongEchoFlag;               //0x00000006
    s8                           reserv2;                      //0x00000007
    s8                           NoiseGainLimit;               //0x00000008
    s8                           NoiseMin;                     //0x00000009
    s8                           NoiseGainLimitStep;           //0x0000000A
    s8                           AmpThr;                       //0x0000000B
    s8                           HighPassFilterFlag;           //0x0000000C
    s8                           NotchFilterFlag;              //0x0000000D
    s8                           NoiseSuppresserFlag;          //0x0000000E
    s8                           NoiseSuppresserWithoutSpeechFlag; //0x0000000F
    u32                         AudioParams[CALIB_AUDIO_PARAM_QTY-4]; //0x00000010
} CALIB_AUDIO_PARAMS_T; //Size : 0x20



// ============================================================================
// CALIB_AUDIO_ITF_T
// -----------------------------------------------------------------------------
/// Calibration of an audio interface. It gathers the audio gains and VoC calibrations
/// data
// =============================================================================
typedef struct
{
    /// Echo Cancelling for VoC.
    CALIB_AUDIO_VOC_EC_T           vocEc;                        //0x00000000
    /// VoC anti-distortion filters.
    CALIB_AUDIO_VOC_FILTERS_T      vocFilters;                   //0x0000000C
    /// Calib audio gain types.
    CALIB_AUDIO_GAINS_T            audioGains;                   //0x0000010C
    /// Audio calibration parameters.
    CALIB_AUDIO_PARAMS_T           audioParams;                  //0x00000170
} CALIB_AUDIO_ITF_T; //Size : 0x190



// ============================================================================
// CALIB_GPADC_T
// -----------------------------------------------------------------------------
/// Calib GPADC analog type.
// =============================================================================
typedef struct
{
    u16                         sensorGainA;                  //0x00000000
    u16                         sensorGainB;                  //0x00000002
} CALIB_GPADC_T; //Size : 0x4



// ============================================================================
// CALIB_RF_CRC_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef u32 CALIB_RF_CRC_T;


// ============================================================================
// CALIB_XCV_T
// -----------------------------------------------------------------------------
/// Transceiver calibration.
// =============================================================================
typedef struct
{
    CALIB_XCV_TIMES_T              times;                        //0x00000000
    CALIB_XCV_PARAM_T              param;                        //0x00000028
    /// XCV PAL custom types.
    CALIB_XCV_PALCUST_T            palcust;                      //0x00000078
} CALIB_XCV_T; //Size : 0xA8



// ============================================================================
// CALIB_PA_T
// -----------------------------------------------------------------------------
/// Power Amplifier RF calibration.
// =============================================================================
typedef struct
{
    CALIB_PA_TIMES_T               times;                        //0x00000000
    CALIB_PA_PARAM_T               param;                        //0x00000020
    /// PA PAL custom types.
    CALIB_PA_PALCUST_T             palcust;                      //0x0000005C
} CALIB_PA_T; //Size : 0x122C



// ============================================================================
// CALIB_SW_T
// -----------------------------------------------------------------------------
/// Switch calibration.
// =============================================================================
typedef struct
{
    CALIB_SW_TIMES_T               times;                        //0x00000000
    CALIB_SW_PARAM_T               param;                        //0x00000020
    CALIB_SW_PALCUST_T             palcust;                      //0x0000005C
} CALIB_SW_T; //Size : 0x80



// ============================================================================
// CALIB_BB_T
// -----------------------------------------------------------------------------
/// Baseband calibration.
// =============================================================================
typedef struct
{
    CALIB_PAL_TIMES_T              times;                        //0x00000000
    /// Audio calibration, for each interface
    CALIB_AUDIO_ITF_T              audio[CALIB_AUDIO_ITF_QTY];   //0x00000028
    /// Analog macros calibration: GPADC.
    CALIB_GPADC_T                  gpadc;                        //0x00000988
} CALIB_BB_T; //Size : 0x98C


// ============================================================================
// CALIB_BUFFER_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef struct
{
    /// Information about this calib buffer.
    CALIB_VERSION_TAG_T            versionTag;                   //0x00000000
    /// Calib process method and date type.
    CALIB_OP_INFO_T                opInfo;                       //0x00000004
    /// Transceiver calibration.
    CALIB_XCV_T                    xcv;                          //0x0000000C
    /// Power Amplifier RF calibration.
    CALIB_PA_T                     pa;                           //0x000000B4
    /// Switch calibration.
    CALIB_SW_T                     sw;                           //0x000012E0
    /// Baseband calibration.
    CALIB_BB_T                     bb;                           //0x00001360
    /// Reserved for future use.
    u32                         reserved[2];                  //0x00001CEC
    /// Information for audio calibration data.
    CALIB_VERSION_TAG_T            audioVersionTag;              //0x00001CF4
    /// CRC value for RF calibration data.
    CALIB_RF_CRC_T                 rfCrc;                        //0x00001CF8
    // iir params
    CALIB_AUDIO_IIR_PARAM_T        iirParam; //0x00001CFC
} CALIB_BUFFER_T; //Size : 0x1DEC


// ============================================================================
// CALIB_STUB_CTX_T
// -----------------------------------------------------------------------------
/// Used to send calibration context change requests from the remote calibration
/// tools to the Calib Stub.
// =============================================================================
typedef struct
{
    /// Commands from the remote calibration tools.
     CALIB_STUB_CMDS_T     state;                        //0x00000000
     //BOOL                  firstFint;                    //0x00000004
     s32                  firstFint;                    //0x00000004
    /// Subcommands from the remote calibration tools.
     s32                 setXtalFreqOffset;            //0x00000008
     u8                 setAfcBound;                  //0x0000000C
     u8                 setChangeDACAfcBound;         //0x0000000D
     s32                 setChangeDACAfcFreqOffset;    //0x00000010
     s32                 setChangeDACAfcFreq;          //0x00000014
     s32                 setAfcFreqOffset;             //0x00000018
     s8                  setILossOffset;               //0x0000001C
     s32                 setPAProfMeas;                //0x00000020
     s8                  setCalibUpdate;               //0x00000024
     u8                 setRestartGpadcMeasure;       //0x00000025
    /// Cells information
     u16                arfcn[CALIB_NB_CELLS];        //0x00000026
     u8                 power[CALIB_NB_CELLS];        //0x00000030
     //BOOL                  isPcs[CALIB_NB_CELLS];        //0x00000035
     s32                  isPcs[CALIB_NB_CELLS];        //0x00000035
     u8                 bsic;                         //0x0000003A
     u32                fn;                           //0x0000003C
     u8                 t2;                           //0x00000040
     u8                 t3;                           //0x00000041
     u16                qbOf;                         //0x00000042
     u16                pFactor;                      //0x00000044
     s32                 tOf;                          //0x00000048
     s32                 fOf;                          //0x0000004C
     u16                snR;                          //0x00000050
     u8                 bitError;                     //0x00000052
     u8                 monPower;                     //0x00000053
     u8                 nbPower;                      //0x00000054
     u8                 monBitmap;                    //0x00000055
     s32                 meanFOf;                      //0x00000058
    /// This is initialized by HST
     u8                 xtalCalibDone;                //0x0000005C
    /// This is initialized by HST
     u16                paProfNextDacVal;             //0x0000005E
     u8                 paProfCalibDone;              //0x00000060
     s16                 dcoAverI;                     //0x00000062
     s16                 dcoAverQ;                     //0x00000064
    /// Can go up to 2 * CT_CALIB_DCO_ACC_COUNT - 1.
     u8                 dcoAccCount;                  //0x00000066
     s16                 dcoI[CALIB_DCO_ACC_COUNT];    //0x00000068
     s16                 dcoQ[CALIB_DCO_ACC_COUNT];    //0x000000A8
    /// Status of the iloss calibration porcess. This is initialized by HST.
    u8                          iLossCalibDone;               //0x000000E8
    /// ARFCN for which to measure the insertion loss. This is not initialized.
    u16                         iLossNextArfcn;               //0x000000EA
    u16                         gpadcAver[CALIB_GPADC_CH_QTY]; //0x000000EC
    /// Can go up to 2*CALIB_GPADC_ACC_COUNT-1.
    u8                          gpadcAccCount;                //0x000000F4
    CALIB_GPADC_ALL_CH_T           gpadcAcc[CALIB_GPADC_ACC_COUNT_MAX]; //0x000000F6
    /// Parameters for Tx commands from the remote calibration tools.
     u16                txArfcn;                      //0x000004F6
     u8                 txPcl;                        //0x000004F8
     u16                txDac;                        //0x000004FA
     u8                 txBand;                       //0x000004FC
     u8                 txTsc;                        //0x000004FD
    /// Parameters for monitoring commands from the remote calibration tools.
     u16                monArfcn;                     //0x000004FE
     u8                 monBand;                      //0x00000500
     u8                 monExpPow;                    //0x00000501
    /// Parameters for audio commands from the remote calibration tools. This parameter
    /// is used to select the Audio Interface to calibrate
     u8                 itfSel;                       //0x00000502
    /// This field selects which input (microphone) is used on the Audio Interface
    /// defined by the itfSel field.
     u8                 inSel;                        //0x00000503
     u8                 inGain;                       //0x00000504
     u8                 inUart;                       //0x00000505
    /// This field selects which input (speaker) is used on the Audio Interface defined
    /// by the itfSel field.
     u8                 outSel;                       //0x00000506
     u8                 outGain;                      //0x00000507
     u8                 polyGain;                     //0x00000508
     u8                 sideGain;                     //0x00000509
     u16                outFreq;                      //0x0000050A
     u8                 outAmpl;                      //0x0000050C
     u8                 startAudioDictaphone;         //0x0000050D
     u8                 audioDictaphoneStatus;        //0x0000050E
    /// Command to start the custom calibration of the PMD. This value is given to
    /// the pmd_CustomCalibration() function. This is initialized by the stub and
    /// written by the HST.
     u8                 pmdCustomCalibStart;          //0x0000050F
    /// Status of the custom calibration of the PMD. This is initialized by the HST
    /// and returned by the stub.
     u32                pmdCustomCalibStatus;         //0x00000510
} CALIB_STUB_CTX_T; //Size : 0x514



// ============================================================================
// CALIB_CALIBRATION_T
// -----------------------------------------------------------------------------
/// This struct will contain pointers to the calibration info and to the struct where
/// to put the calibration context requests. It also contains the address of the
/// calibration sector in flash.
// =============================================================================
typedef struct
{
    CALIB_VERSION_TAG_T            codeVersion;                  //0x00000000
    CALIB_PARAM_STATUS_T           paramStatus;                  //0x00000004
    CALIB_COMMAND_T                command;                      //0x00000008
    CALIB_OP_INFO_T*               opInfo;                       //0x0000000C
    CALIB_XCV_T*                   xcv;                          //0x00000010
    CALIB_PA_T*                    pa;                           //0x00000014
    CALIB_SW_T*                    sw;                           //0x00000018
    CALIB_BB_T*                    bb;                           //0x0000001C
     CALIB_OP_INFO_T*      hstOpInfo;                    //0x00000020
     CALIB_XCV_T*          hstXcv;                       //0x00000024
     CALIB_PA_T*           hstPa;                        //0x00000028
     CALIB_SW_T*           hstSw;                        //0x0000002C
     CALIB_BB_T*           hstBb;                        //0x00000030
     CALIB_STUB_CTX_T*     stubCtx;                      //0x00000034
     CALIB_BUFFER_T*          flash;                        //0x00000038
     CALIB_AUDIO_IIR_PARAM_T*	   iirParam;		 //0x0000003c
     CALIB_AUDIO_IIR_PARAM_T* hstIIRParam;      //0x00000040
} CALIB_CALIBRATION_T; //Size : 0x44





//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------

#define CALIB_MAJ_MASK(x) ((x >> 8) & 0xFF)
#define CALIB_MIN_MASK(x) (x & 0xFF)
#define CALIB_MARK_MASK(x) (x & 0xFFFF0000)
#define CALIB_VERSION(maj, min) (CALIB_MARK_VERSION | maj << 8 | min << 0)

#endif
