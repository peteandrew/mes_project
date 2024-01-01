// ConsoleCommands.c
// This is where you add commands:
//    1. Add a protoype
//      static eCommandResult_T ConsoleCommandVer(const char buffer[]);
//    2. Add the command to mConsoleCommandTable
//        {"ver", &ConsoleCommandVer, HELP("Get the version string")},
//    3. Implement the function, using ConsoleReceiveParam<Type> to get the parameters from the buffer.

#include <string.h>
#include "stdbool.h"
#include "consoleCommands.h"
#include "console.h"
#include "consoleIo.h"
#include "version.h"
#include "application.h"
#include "audioTypes.h"
#include "sequence.h"

#define IGNORE_UNUSED_VARIABLE(x)     if ( &x == &x ) {}

static eCommandResult_T ConsoleCommandComment(const char buffer[]);
static eCommandResult_T ConsoleCommandVer(const char buffer[]);
static eCommandResult_T ConsoleCommandHelp(const char buffer[]);
static eCommandResult_T ConsoleCommandParamExampleInt16(const char buffer[]);
static eCommandResult_T ConsoleCommandParamExampleHexUint16(const char buffer[]);
static eCommandResult_T ConsoleCommandLEDToggle(const char buffer[]);
static eCommandResult_T ConsoleCommandFlashDeviceId(const char buffer[]);
static eCommandResult_T ConsoleCommandRecordAudio(const char buffer[]);
static eCommandResult_T ConsoleCommandPlayAudio(const char buffer[]);
static eCommandResult_T ConsoleCommandPlayAudioFromFlash(const char buffer[]);
static eCommandResult_T ConsoleCommandStopAudio(const char buffer[]);
static eCommandResult_T ConsoleCommandSetAudioClipNum(const char buffer[]);
static eCommandResult_T ConsoleCommandGetAudioClipNum(const char buffer[]);
static eCommandResult_T ConsoleCommandGetAudioClipUsed(const char buffer[]);
static eCommandResult_T ConsoleCommandSetAudioClipUsed(const char buffer[]);
static eCommandResult_T ConsoleCommandSetAudioStartSample(const char buffer[]);
static eCommandResult_T ConsoleCommandSetAudioEndSample(const char buffer[]);
static eCommandResult_T ConsoleCommandSetAudioLoop(const char buffer[]);
static eCommandResult_T ConsoleCommandStoreAudio(const char buffer[]);
static eCommandResult_T ConsoleCommandLoadAudio(const char buffer[]);
static eCommandResult_T ConsoleCommandSetAudioChannelParams(const char buffer[]);
static eCommandResult_T ConsoleCommandGetAudioChannelParams(const char buffer[]);
static eCommandResult_T ConsoleCommandSetAudioChannelRunning(const char buffer[]);
static eCommandResult_T ConsoleCommandOutputAudioData(const char buffer[]);
static eCommandResult_T ConsoleCommandStartSequence(const char buffer[]);
static eCommandResult_T ConsoleCommandStopSequence(const char buffer[]);
static eCommandResult_T ConsoleCommandSetAudioChannelStepParams(const char buffer[]);
static eCommandResult_T ConsoleCommandGetAudioChannelStepParams(const char buffer[]);
static eCommandResult_T ConsoleCommandSetSequenceNum(const char buffer[]);
static eCommandResult_T ConsoleCommandGetSequenceNum(const char buffer[]);
static eCommandResult_T ConsoleCommandStoreSequence(const char buffer[]);
static eCommandResult_T ConsoleCommandLoadSequence(const char buffer[]);
static eCommandResult_T ConsoleCommandGetSequenceUsed(const char buffer[]);


static const sConsoleCommandTable_T mConsoleCommandTable[] =
{
    {";", &ConsoleCommandComment, HELP("Comment! You do need a space after the semicolon. ")},
    {"help", &ConsoleCommandHelp, HELP("Lists the commands available")},
    {"ver", &ConsoleCommandVer, HELP("Get the version string")},
    {"int", &ConsoleCommandParamExampleInt16, HELP("How to get a signed int16 from params list: int -321")},
    {"u16h", &ConsoleCommandParamExampleHexUint16, HELP("How to get a hex u16 from the params list: u16h aB12")},
    {"led", &ConsoleCommandLEDToggle, HELP("Toggle LED")},
    {"flashid", &ConsoleCommandFlashDeviceId, HELP("Read SPI Flash device ID")},
    {"record", &ConsoleCommandRecordAudio, HELP("Recprd audio clip")},
    {"play", &ConsoleCommandPlayAudio, HELP("Play audio clip")},
    {"playflash", &ConsoleCommandPlayAudioFromFlash, HELP("Play audio clip from Flash")},
    {"stop", &ConsoleCommandStopAudio, HELP("Stop audio playback")},
    {"clipset", &ConsoleCommandSetAudioClipNum, HELP("Set audio clip number")},
    {"clipget", &ConsoleCommandGetAudioClipNum, HELP("Get audio clip number")},
    {"clipused", &ConsoleCommandGetAudioClipUsed, HELP("Get whether audio clip has been used")},
    {"setclipused", &ConsoleCommandSetAudioClipUsed, HELP("Set audio clip used flag")},
    {"startsample", &ConsoleCommandSetAudioStartSample, HELP("Set start sample")},
    {"endsample", &ConsoleCommandSetAudioEndSample, HELP("Set start sample")},
    {"loop", &ConsoleCommandSetAudioLoop, HELP("Set loop")},
    {"store", &ConsoleCommandStoreAudio, HELP("Store audio data")},
    {"load", &ConsoleCommandLoadAudio, HELP("Load audio data")},
    {"output", &ConsoleCommandOutputAudioData, HELP("Output audio data")},
    {"schparams", &ConsoleCommandSetAudioChannelParams, HELP("Set audio channel params")},
    {"gchparams", &ConsoleCommandGetAudioChannelParams, HELP("Get audio channel params")},
    {"chrunning", &ConsoleCommandSetAudioChannelRunning, HELP("Set audio channel running state")},
    {"startseq", &ConsoleCommandStartSequence, HELP("Start sequence")},
    {"stopseq", &ConsoleCommandStopSequence, HELP("Stop sequence")},
    {"schsparams", &ConsoleCommandSetAudioChannelStepParams, HELP("Set audio channel step params")},
    {"gchsparams", &ConsoleCommandGetAudioChannelStepParams, HELP("Get audio channel step params")},
    {"seqset", &ConsoleCommandSetSequenceNum, HELP("Set sequence number")},
    {"seqget", &ConsoleCommandGetSequenceNum, HELP("Get sequence number")},
    {"seqstore", &ConsoleCommandStoreSequence, HELP("Store sequence")},
    {"seqload", &ConsoleCommandLoadSequence, HELP("Load sequence")},
    {"seqused", &ConsoleCommandGetSequenceUsed, HELP("Load sequence")},

  CONSOLE_COMMAND_TABLE_END // must be LAST
};

static eCommandResult_T ConsoleCommandComment(const char buffer[])
{
  // do nothing
  IGNORE_UNUSED_VARIABLE(buffer);
  return COMMAND_SUCCESS;
}

static eCommandResult_T ConsoleCommandHelp(const char buffer[])
{
  uint32_t i;
  uint32_t tableLength;
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  tableLength = sizeof(mConsoleCommandTable) / sizeof(mConsoleCommandTable[0]);
  for ( i = 0u ; i < tableLength - 1u ; i++ )
  {
    ConsoleIoSendString(mConsoleCommandTable[i].name);
#if CONSOLE_COMMAND_MAX_HELP_LENGTH > 0
    ConsoleIoSendString(" : ");
    ConsoleIoSendString(mConsoleCommandTable[i].help);
#endif // CONSOLE_COMMAND_MAX_HELP_LENGTH > 0
    ConsoleIoSendString(STR_ENDLINE);
  }
  return result;
}

static eCommandResult_T ConsoleCommandParamExampleInt16(const char buffer[])
{
  int16_t parameterInt;
  eCommandResult_T result;
  result = ConsoleReceiveParamInt16(buffer, 1, &parameterInt);
  if ( COMMAND_SUCCESS == result )
  {
    ConsoleIoSendString("Parameter is ");
    ConsoleSendParamInt16(parameterInt);
    ConsoleIoSendString(" (0x");
    ConsoleSendParamHexUint16((uint16_t)parameterInt);
    ConsoleIoSendString(")");
    ConsoleIoSendString(STR_ENDLINE);
  }
  return result;
}
static eCommandResult_T ConsoleCommandParamExampleHexUint16(const char buffer[])
{
  uint16_t parameterUint16;
  eCommandResult_T result;
  result = ConsoleReceiveParamHexUint16(buffer, 1, &parameterUint16);
  if ( COMMAND_SUCCESS == result )
  {
    ConsoleIoSendString("Parameter is 0x");
    ConsoleSendParamHexUint16(parameterUint16);
    ConsoleIoSendString(STR_ENDLINE);
  }
  return result;
}

static eCommandResult_T ConsoleCommandVer(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  ConsoleIoSendString(VERSION_STRING);
  ConsoleIoSendString(STR_ENDLINE);
  return result;
}


static eCommandResult_T ConsoleCommandLEDToggle(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  appToggleLED();
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandFlashDeviceId(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  uint16_t deviceId = appFlashReadDeviceId();
  ConsoleIoSendString("Device ID is 0x");
  ConsoleSendParamHexUint16(deviceId);
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandRecordAudio(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  appRecordAudio();
  ConsoleIoSendString("Recording audio clip");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandPlayAudio(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  appPlayAudio();
  ConsoleIoSendString("Playing audio clip");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandPlayAudioFromFlash(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  appPlayAudioFromFlash();
  ConsoleIoSendString("Playing audio clip from Flash");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandStopAudio(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  appStopAudio();
  ConsoleIoSendString("Stopping audio playback");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandSetAudioClipNum(const char buffer[])
{
  int16_t parameterInt;
  eCommandResult_T result;
  result = ConsoleReceiveParamInt16(buffer, 1, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }

  if (parameterInt < 1 || parameterInt > NUM_CLIPS)
  {
    ConsoleIoSendString(STR_ENDLINE);
    ConsoleIoSendString("Audio clip number must be 1-" STRINGIZE(NUM_CLIPS));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }

  appSetAudioClipNum((uint8_t)parameterInt);
  ConsoleIoSendString(STR_ENDLINE);
  ConsoleIoSendString("Audio clip number set");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandGetAudioClipNum(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  uint8_t audioClipNum = appGetAudioClipNum();
  ConsoleIoSendString(STR_ENDLINE);
  ConsoleIoSendString("Audio clip number: ");
  ConsoleSendParamUInt8(audioClipNum);
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandGetAudioClipUsed(const char buffer[])
{
  int16_t parameterInt;
  eCommandResult_T result;
  result = ConsoleReceiveParamInt16(buffer, 1, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }

  if (parameterInt < 1 || parameterInt > NUM_CLIPS)
  {
    ConsoleIoSendString(STR_ENDLINE);
    ConsoleIoSendString("Audio clip number must be 1-" STRINGIZE(NUM_CLIPS));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }

  ConsoleIoSendString(STR_ENDLINE);
  ConsoleIoSendString("Audio clip used: ");
  if (appGetAudioClipUsed((uint8_t)parameterInt)) {
    ConsoleIoSendString("Yes");
  } else {
    ConsoleIoSendString("No");
  }
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandSetAudioClipUsed(const char buffer[])
{
  int16_t parameterInt;
  eCommandResult_T result;
  result = ConsoleReceiveParamInt16(buffer, 1, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }

  if (parameterInt < 1 || parameterInt > NUM_CLIPS)
  {
    ConsoleIoSendString(STR_ENDLINE);
    ConsoleIoSendString("Audio clip number must be 1-" STRINGIZE(NUM_CLIPS));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }

  appSetAudioClipUsed((uint8_t)parameterInt);
  ConsoleIoSendString(STR_ENDLINE);
  ConsoleIoSendString("Audio clip used flag set");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandSetAudioStartSample(const char buffer[])
{
  int16_t parameterInt;
  eCommandResult_T result;
  result = ConsoleReceiveParamInt16(buffer, 1, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }

  if (parameterInt < 0 || parameterInt > MAX_SAMPLE_IDX)
  {
    ConsoleIoSendString(STR_ENDLINE);
    ConsoleIoSendString("Audio start sample must be 0-" STRINGIZE(MAX_SAMPLE_IDX));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }

  appSetAudioStartSample((uint16_t)parameterInt);
  ConsoleIoSendString(STR_ENDLINE);
  ConsoleIoSendString("Audio start sample set");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandSetAudioEndSample(const char buffer[])
{
  int16_t parameterInt;
  eCommandResult_T result;
  result = ConsoleReceiveParamInt16(buffer, 1, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }

  if (parameterInt < 0 || parameterInt > MAX_SAMPLE_IDX)
  {
    ConsoleIoSendString(STR_ENDLINE);
    ConsoleIoSendString("Audio end sample must be 0-" STRINGIZE(MAX_SAMPLE_IDX));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }

  appSetAudioEndSample((uint16_t)parameterInt);
  ConsoleIoSendString(STR_ENDLINE);
  ConsoleIoSendString("Audio end sample set");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandSetAudioLoop(const char buffer[])
{
  uint32_t startIndex = 0;
  eCommandResult_T result;
  result = ConsoleParamFindN(buffer, 1, &startIndex);
  ConsoleIoSendString(STR_ENDLINE);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }

  char charVal = buffer[startIndex];

  if (charVal == 't') {
    appSetAudioLoop(true);
    ConsoleIoSendString("Loop on");
  } else {
    appSetAudioLoop(false);
    ConsoleIoSendString("Loop off");
  }
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandStoreAudio(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  ConsoleIoSendString("Storing audio data");
  ConsoleIoSendString(STR_ENDLINE);
  appStoreAudio();
  ConsoleIoSendString("Audio data stored");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandLoadAudio(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  ConsoleIoSendString("Loading audio data");
  ConsoleIoSendString(STR_ENDLINE);
  appLoadAudio();
  ConsoleIoSendString("Audio data loaded");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandOutputAudioData(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  int16_t *data = appOutputAudioData();
  ConsoleIoSendString(STR_ENDLINE);

  for (int i=0; i<1024; i++) {
    ConsoleIoSendString("0x");
    ConsoleSendParamHexUint16((uint16_t)data[i]);
    ConsoleIoSendString(STR_ENDLINE);
  }

  return result;
}


static eCommandResult_T ConsoleCommandSetAudioChannelParams(const char buffer[])
{
  ChannelParams_T params;
  int16_t parameterInt;
  uint8_t channelIdx;
  uint32_t startIndex = 0;
  eCommandResult_T result;

  ConsoleIoSendString(STR_ENDLINE);

  result = ConsoleReceiveParamInt16(buffer, 1, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  if (parameterInt < 0 || parameterInt > MAX_CHANNEL_IDX)
  {
    ConsoleIoSendString("Channel index must be 0-" STRINGIZE(MAX_CHANNEL_IDX));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }
  channelIdx = (uint8_t) parameterInt;

  result = ConsoleReceiveParamInt16(buffer, 2, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  if (parameterInt < 1 || parameterInt > NUM_CLIPS)
  {
    ConsoleIoSendString("Audio clip number must be 1-" STRINGIZE(NUM_CLIPS));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }
  params.clipNum = (uint8_t) parameterInt;

  result = ConsoleReceiveParamInt16(buffer, 3, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  if (parameterInt < 0 || parameterInt > MAX_SAMPLE_IDX)
  {
    ConsoleIoSendString("Audio start sample must be 0-" STRINGIZE(MAX_SAMPLE_IDX));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }
  params.startSample = (uint16_t) parameterInt;

  result = ConsoleReceiveParamInt16(buffer, 4, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  if (parameterInt < 0 || parameterInt > MAX_SAMPLE_IDX)
  {
    ConsoleIoSendString("Audio end sample must be 0-" STRINGIZE(MAX_SAMPLE_IDX));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }
  params.endSample = (uint16_t) parameterInt;

  if (params.endSample <= params.startSample) {
    ConsoleIoSendString("End sample must be greater than start sample");
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }

  result = ConsoleParamFindN(buffer, 5, &startIndex);
  ConsoleIoSendString(STR_ENDLINE);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  char charVal = buffer[startIndex];
  params.loop = (charVal == 't');

  appSetAudioChannelParams(channelIdx, params);

  ConsoleIoSendString("Channel params set");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandGetAudioChannelParams(const char buffer[])
{
  int16_t parameterInt;
  uint8_t channelIdx;
  ChannelParams_T params;
  eCommandResult_T result;

  ConsoleIoSendString(STR_ENDLINE);

  result = ConsoleReceiveParamInt16(buffer, 1, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  if (parameterInt < 0 || parameterInt > MAX_CHANNEL_IDX)
  {
    ConsoleIoSendString("Channel index must be 0-" STRINGIZE(MAX_CHANNEL_IDX));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }
  channelIdx = (uint8_t) parameterInt;

  params = appGetAudioChannelParams(channelIdx);

  ConsoleIoSendString("Clip number: ");
  ConsoleSendParamUInt8(params.clipNum);
  ConsoleIoSendString(STR_ENDLINE);

  ConsoleIoSendString("Start sample: ");
  ConsoleSendParamInt16((int16_t) params.startSample);
  ConsoleIoSendString(STR_ENDLINE);

  ConsoleIoSendString("End sample: ");
  ConsoleSendParamInt16((int16_t) params.endSample);
  ConsoleIoSendString(STR_ENDLINE);

  ConsoleIoSendString("Loop: ");
  if (params.loop) {
    ConsoleIoSendString("Yes");
  } else {
    ConsoleIoSendString("No");
  }
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandSetAudioChannelRunning(const char buffer[])
{
  int16_t parameterInt;
  uint8_t channelIdx;
  uint32_t startIndex = 0;
  eCommandResult_T result;

  ConsoleIoSendString(STR_ENDLINE);

  result = ConsoleReceiveParamInt16(buffer, 1, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  if (parameterInt < 0 || parameterInt > MAX_CHANNEL_IDX)
  {
    ConsoleIoSendString("Channel index must be 0-" STRINGIZE(MAX_CHANNEL_IDX));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }
  channelIdx = (uint8_t) parameterInt;

  result = ConsoleParamFindN(buffer, 2, &startIndex);
  ConsoleIoSendString(STR_ENDLINE);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  char charVal = buffer[startIndex];
  bool running = (charVal == 't');

  appSetAudioChannelRunning(channelIdx, running);

  ConsoleIoSendString("Set channel running state");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandStartSequence(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  ConsoleIoSendString(STR_ENDLINE);

  appStartSequence();

  ConsoleIoSendString("Sequence started");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandStopSequence(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  ConsoleIoSendString(STR_ENDLINE);

  appStopSequence();

  ConsoleIoSendString("Sequence stopped");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandSetAudioChannelStepParams(const char buffer[])
{
  ChannelParams_T params;
  int16_t parameterInt;
  uint8_t channelIdx;
  uint8_t stepIdx;
  uint32_t startIndex = 0;
  eCommandResult_T result;

  ConsoleIoSendString(STR_ENDLINE);

  result = ConsoleReceiveParamInt16(buffer, 1, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  if (parameterInt < 0 || parameterInt > MAX_CHANNEL_IDX)
  {
    ConsoleIoSendString("Channel index must be 0-" STRINGIZE(MAX_CHANNEL_IDX));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }
  channelIdx = (uint8_t) parameterInt;

  result = ConsoleReceiveParamInt16(buffer, 2, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  if (parameterInt < 0 || parameterInt > MAX_STEP_IDX)
  {
    ConsoleIoSendString("Step must be 0-" STRINGIZE(MAX_STEP_IDX));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }
  stepIdx = (uint8_t) parameterInt;

  result = ConsoleReceiveParamInt16(buffer, 3, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  if (parameterInt < 0 || parameterInt > NUM_CLIPS)
  {
    ConsoleIoSendString("Audio clip number must be 0-" STRINGIZE(NUM_CLIPS));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }
  params.clipNum = (uint8_t) parameterInt;

  result = ConsoleReceiveParamInt16(buffer, 4, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  if (parameterInt < 0 || parameterInt > MAX_SAMPLE_IDX)
  {
    ConsoleIoSendString("Audio start sample must be 0-" STRINGIZE(MAX_SAMPLE_IDX));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }
  params.startSample = (uint16_t) parameterInt;

  result = ConsoleReceiveParamInt16(buffer, 5, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  if (parameterInt < 0 || parameterInt > MAX_SAMPLE_IDX)
  {
    ConsoleIoSendString("Audio end sample must be 0-" STRINGIZE(MAX_SAMPLE_IDX));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }
  params.endSample = (uint16_t) parameterInt;

  if (params.endSample <= params.startSample) {
    ConsoleIoSendString("End sample must be greater than start sample");
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }

  result = ConsoleParamFindN(buffer, 6, &startIndex);
  ConsoleIoSendString(STR_ENDLINE);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  char charVal = buffer[startIndex];
  params.loop = (charVal == 't');

  appSetSequenceStepChannelParams(stepIdx, channelIdx, params);

  ConsoleIoSendString("Step channel params set");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandGetAudioChannelStepParams(const char buffer[])
{
  int16_t parameterInt;
  uint8_t channelIdx;
  uint8_t stepIdx;
  ChannelParams_T params;
  eCommandResult_T result;

  ConsoleIoSendString(STR_ENDLINE);

  result = ConsoleReceiveParamInt16(buffer, 1, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  if (parameterInt < 0 || parameterInt > MAX_CHANNEL_IDX)
  {
    ConsoleIoSendString("Channel index must be 0-" STRINGIZE(MAX_CHANNEL_IDX));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }
  channelIdx = (uint8_t) parameterInt;

  result = ConsoleReceiveParamInt16(buffer, 2, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }
  if (parameterInt < 0 || parameterInt > MAX_STEP_IDX)
  {
    ConsoleIoSendString("Step must be 0-" STRINGIZE(MAX_STEP_IDX));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }
  stepIdx = (uint8_t) parameterInt;

  params = appGetSequenceStepChannelParams(stepIdx, channelIdx);

  ConsoleIoSendString("Clip number: ");
  ConsoleSendParamUInt8(params.clipNum);
  ConsoleIoSendString(STR_ENDLINE);

  ConsoleIoSendString("Start sample: ");
  ConsoleSendParamInt16((int16_t) params.startSample);
  ConsoleIoSendString(STR_ENDLINE);

  ConsoleIoSendString("End sample: ");
  ConsoleSendParamInt16((int16_t) params.endSample);
  ConsoleIoSendString(STR_ENDLINE);

  ConsoleIoSendString("Loop: ");
  if (params.loop) {
    ConsoleIoSendString("Yes");
  } else {
    ConsoleIoSendString("No");
  }
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandSetSequenceNum(const char buffer[])
{
  int16_t parameterInt;
  eCommandResult_T result;
  result = ConsoleReceiveParamInt16(buffer, 1, &parameterInt);
  if (result != COMMAND_SUCCESS)
  {
    return result;
  }

  if (parameterInt < 1 || parameterInt > NUM_SEQUENCES)
  {
    ConsoleIoSendString(STR_ENDLINE);
    ConsoleIoSendString("Sequence number must be 1-" STRINGIZE(NUM_SEQUENCES));
    ConsoleIoSendString(STR_ENDLINE);
    return COMMAND_PARAMETER_ERROR;
  }

  appSetSequenceNum((uint8_t)parameterInt);
  ConsoleIoSendString(STR_ENDLINE);
  ConsoleIoSendString("Sequence number set");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandGetSequenceNum(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  ConsoleIoSendString(STR_ENDLINE);
  ConsoleIoSendString("Sequence number: ");
  ConsoleSendParamUInt8(appGetSequenceNum());
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandStoreSequence(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  ConsoleIoSendString("Storing sequence");
  ConsoleIoSendString(STR_ENDLINE);
  appStoreSequence();
  ConsoleIoSendString("Sequence stored");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandLoadSequence(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  ConsoleIoSendString("Loading sequence");
  ConsoleIoSendString(STR_ENDLINE);
  appLoadSequence();
  ConsoleIoSendString("Sequence loaded");
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


static eCommandResult_T ConsoleCommandGetSequenceUsed(const char buffer[])
{
  eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

  ConsoleIoSendString(STR_ENDLINE);
  ConsoleIoSendString("Sequence used: ");
  if (appGetSequenceUsed()) {
    ConsoleIoSendString("Yes");
  } else {
    ConsoleIoSendString("No");
  }
  ConsoleIoSendString(STR_ENDLINE);

  return result;
}


const sConsoleCommandTable_T* ConsoleCommandsGetTable(void)
{
  return (mConsoleCommandTable);
}
