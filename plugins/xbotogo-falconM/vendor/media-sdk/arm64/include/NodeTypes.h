#pragma once

namespace blink {
namespace media {

class NodeType  { 
public:
  static const char* VideoEncoderAndroid;
  static const char* VideoDecoderAndroid;
  static const char* VideoRenderAndroid;
  static const char* AudioEncoderAndroid;
  static const char* AudioDecoderAndroid;
  static const char* AudioPlayAndroid;
  static const char* CameraCaptureAndroid;
  static const char* VideoEncoderIos;
  static const char* VideoDecoderIos;
  static const char* VideoRenderIos;
  static const char* CameraCaptureIos;
  static const char* AudioEncoderIos;
  static const char* AudioDecoderIos;
  static const char* AudioPlayIos;
  static const char* SrtPull;
  static const char* SrtPush;
  static const char* FileDump;
  static const char* EncodedAudioDump;
  static const char* YUVFileSource;
  static const char* H264FileSource;
  static const char* PCMFileSource;
  static const char* EncodedAudioFileSource;
  static const char* RTPMessageEncoder;
  static const char* RTPMessageDecoder;
  static const char* VideoScaler;
  static const char* VideoFps;
  static const char* VideoJitterBuffer;
  static const char* MediaDataObserver;
  static const char* NdiSend;
  static const char* NdiRecv;
  static const char* OesToGlTextureAndroid;
  static const char* ExternalMediaNodeAdapterAndroid;
};

class NodeMessageName {
public:
  static const char* Base_GetNodeStatesMessage;
  static const char* SrtPull_NewSrtStreamMessage;
  static const char* SrtPull_DeleteSrtStreamMessage;
  static const char* SrtPull_AddSrtDecodeMessage;
  static const char* SrtPull_RemoveSrtDecodeMessage;
  static const char* SrtPull_UpdateSrtPullConfigMessage;
  static const char* SrtPush_SrtStreamStatusMessage;
  static const char* VideoDecoder_VideoFormatChangedMessage;
  static const char* VideoEncoder_StatesMessage;
  static const char* VideoEncoder_ConfigChangeMessage;
  static const char* VideoDecoder_StatesMessage;
  static const char* AudioDecoder_StatesMessage;
  static const char* SrtPush_StatesMessage;
  static const char* SrtPull_StatesMessage;
  static const char* SrtPush_SetTargetBitrateMessage;
  static const char* VideoScaler_SizeChangeMessage;
  static const char* VideoFps_FpsChangeMessage;
  static const char* FileDump_FileNameChangeMessage;
  static const char* ExternalMediaNodeAdapter_SetNodeMessage;
};

}
}