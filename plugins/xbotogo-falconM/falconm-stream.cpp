#include "falconm-stream.hpp"
#include <BLRTCServerSession.h>
#include <GlobalInit.h>
#include <util/base.h>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <CoreVideo/CoreVideo.h>

using namespace blink::media;

namespace xbotgo {
constexpr uint32_t kDefaultVideoSsrc = 6666;
constexpr uint32_t kDefaultAudioSsrc = 7777;
constexpr uint32_t kDefaultDataSsrc = 8888;
class FalconMStreamSdk final : public FalconMStream, private rtcsdk::BLRTCServerSessionListener {
public:
	FalconMStreamSdk()
	{
		const int init_result = blink::utils::GlobalInit::getInstance().init(blink::utils::GlobalConfig());
		if (init_result != 0) {
			blog(LOG_ERROR, "FalconM: Media SDK GlobalInit failed, result=%d", init_result);
		}
		session_ = rtcsdk::BLRTCServerSession::create(rtcsdk::RTC_SESSION_TYPE_DRAGONFLY);
		if (session_) {
			session_->addListener(this);
		} else {
			blog(LOG_ERROR, "FalconM: failed to create Dragonfly server session");
		}
	}
	~FalconMStreamSdk() override
	{
		disconnect();
		delete session_;
	}
	bool connect(const std::string &device_id, const std::string &broker_address,
			    uint16_t broker_port) override
	{
		if (!session_ || broker_address.empty() || device_id.empty()) {
			blog(LOG_ERROR, "FalconM: invalid session or connection settings");
			return false;
		}
		device_id_ = device_id;
		if (session_->startSession() != 0) {
			blog(LOG_ERROR, "FalconM: BLRTCServerSession startSession failed");
			return false;
		}
		blink::signaling::ConnectClientConfig c;
		c.controlerId = controller_id_;
		c.deviceId = device_id_;
		blog(LOG_INFO, "FalconM: connect device_id='%s', broker_address='%s', broker_port=%u",
		     device_id.c_str(), broker_address.c_str(), broker_port);
		c.ipv4List.push_back(broker_address);
		c.mqttBrokerPort = broker_port;
		if (session_->connectPeerSession(c) != 0) {
			blog(LOG_ERROR, "FalconM: connectPeerSession failed for device '%s'", device_id_.c_str());
			session_->stopSession();
			return false;
		}
		connected_ = true;
		return true;
	}
	bool startStreaming(const uint32_t video_ssrc = kDefaultVideoSsrc, const uint32_t audio_ssrc = kDefaultAudioSsrc, const uint32_t dataSsrc = kDefaultDataSsrc) override
	{
		if (!connected_ || streaming_) {
			return connected_ && streaming_;
		}
		rtcsdk::PullStreamParams p;
		p.srtPushCfg.videoSsrc = video_ssrc;
		p.srtPushCfg.audioSsrc = audio_ssrc;
		p.srtPushCfg.dataSsrc = dataSsrc;
		if (session_->startPullStream(device_id_, p) != 0) {
			blog(LOG_ERROR, "FalconM: startPullStream failed for device '%s'", device_id_.c_str());
			return false;
		}
		streaming_ = true;
		return true;
	}
	bool stopStreaming() override
	{
		if (!streaming_) {
			return true;
		}
		const int result = session_->stopPullStream(device_id_);
		streaming_ = false;
		if (result != 0) {
			blog(LOG_ERROR, "FalconM: stopPullStream failed for device '%s'", device_id_.c_str());
		}
		return result == 0;
	}
	bool isStreaming() const override
	{
		return streaming_;
	}
	void disconnect() override
	{
		stopStreaming();
		if (session_ && connected_) {
			session_->disconnectPeerSession(device_id_);
		}
		if (session_) {
			session_->stopSession();
		}
		connected_ = false;
	}
	void setDecodedFrameCallback(decoded_callback cb) override
	{
		decoded_cb_ = std::move(cb);
	}
	void setAudioCallback(audio_callback cb) override
	{
		audio_cb_ = std::move(cb);
	}
	void setSignalingCallback(signaling_callback cb) override
	{
		signaling_cb_ = std::move(cb);
	}
	bool sendSignalingMessage(const std::string &topic, const uint8_t *data, size_t size) override
	{
		if (!session_ || !connected_ || (!data && size)) {
			return false;
		}
		rtcsdk::MQTTMessage m;
		m.topic = topic;
		m.payload = const_cast<uint8_t *>(data);
		m.payloadlen = (uint32_t)size;
		return session_->sendPeerMessage(device_id_, m) == 0;
	}

private:
	void onPeerDevicesRefresh(rtcsdk::BLNSPClient &client) override
	{
		blog(LOG_INFO, "FalconM: onPeerDevicesRefresh client=%s", client.name.c_str());
	}
	void onPeerConnectStatus(rtcsdk::BLNSPClient &client, int status) override
	{
		blog(LOG_INFO, "FalconM: onPeerConnectStatus client=%s status=%d", client.name.c_str(), status);
		connected_ = status == rtcsdk::PEER_CONNECTION_STATUS_CONNECTED;
		if (connected_) {
			if (!startStreaming()) {
				blog(LOG_ERROR, "FalconM: startStreaming failed after peer connection");
			}
		} else {
			blog(LOG_ERROR, "FalconM: peer connection failed, status=%d", status);
			return;
		}
		
	}
	void onEncodedFrame(uint32_t ssrc, std::shared_ptr<MediaData> data) override
	{
		blog(LOG_INFO, "FalconM: onEncodedFrame ssrc=%u data=%p", ssrc, data.get());
	}
	void onDecodedFrame(uint32_t ssrc, std::shared_ptr<MediaData> d) override
	{
		if (!d) {
			blog(LOG_ERROR, "FalconM: onDecodedFrame received null data, ssrc=%u", ssrc);
			return;
		}
		blog(LOG_INFO, "FalconM: onDecodedFrame ssrc=%u format=%d type=%d", ssrc, d->format, d->type);
		if (d->format == MEDIA_DATA_FORMAT_IMAGE_BUFFER) {
			if (!d->bufferObj) {
				blog(LOG_ERROR, "FalconM: IMAGE_BUFFER decoded frame has null bufferObj, ssrc=%u", ssrc);
				return;
			}
			CVPixelBufferRef pixel_buffer = (CVPixelBufferRef)d->bufferObj->getObject();
			if (!pixel_buffer || CVPixelBufferLockBaseAddress(pixel_buffer, kCVPixelBufferLock_ReadOnly) != kCVReturnSuccess)
				return;
			obs_source_frame frame = {};
			frame.width = (uint32_t)CVPixelBufferGetWidth(pixel_buffer);
			frame.height = (uint32_t)CVPixelBufferGetHeight(pixel_buffer);
			frame.timestamp = d->pts >= 0 ? (uint64_t)d->pts * 1000 : 0;
			const OSType pixel_format = CVPixelBufferGetPixelFormatType(pixel_buffer);
			enum video_colorspace colorspace = VIDEO_CS_709;
			enum video_range_type range = VIDEO_RANGE_PARTIAL;
			switch (pixel_format) {
			case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
				frame.format = VIDEO_FORMAT_NV12;
				frame.full_range = false;
				break;
			case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
				frame.format = VIDEO_FORMAT_NV12;
				frame.full_range = true;
				range = VIDEO_RANGE_FULL;
				break;
			default:
				blog(LOG_WARNING, "FalconM: unsupported CVPixelBuffer format 0x%08x", (unsigned)pixel_format);
				CVPixelBufferUnlockBaseAddress(pixel_buffer, kCVPixelBufferLock_ReadOnly);
				return;
			}
			CFTypeRef matrix_attachment = CVBufferCopyAttachment(pixel_buffer, kCVImageBufferYCbCrMatrixKey, nullptr);
			if (matrix_attachment == kCVImageBufferYCbCrMatrix_ITU_R_601_4)
				colorspace = VIDEO_CS_601;
			else if (matrix_attachment == kCVImageBufferYCbCrMatrix_ITU_R_709_2)
				colorspace = VIDEO_CS_709;
			else if (matrix_attachment == kCVImageBufferYCbCrMatrix_ITU_R_2020)
				colorspace = VIDEO_CS_2100_PQ;
			CFTypeRef primaries_attachment = CVBufferCopyAttachment(pixel_buffer, kCVImageBufferColorPrimariesKey, nullptr);
			if (primaries_attachment == kCVImageBufferColorPrimaries_ITU_R_2020)
				colorspace = VIDEO_CS_2100_PQ;
			if (matrix_attachment)
				CFRelease(matrix_attachment);
			if (primaries_attachment)
				CFRelease(primaries_attachment);
			size_t plane_count = CVPixelBufferGetPlaneCount(pixel_buffer);
			for (size_t i = 0; i < plane_count && i < MAX_AV_PLANES; i++) {
				frame.data[i] = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, i);
				frame.linesize[i] = (uint32_t)CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, i);
			}
			video_format_get_parameters_for_format(colorspace, range, frame.format,
							      frame.color_matrix, frame.color_range_min, frame.color_range_max);
			if (decoded_cb_)
				decoded_cb_(frame);
			CVPixelBufferUnlockBaseAddress(pixel_buffer, kCVPixelBufferLock_ReadOnly);
			return;
		}
		if (!d || !d->buffer || !streaming_) {
			blog(LOG_WARNING, "FalconM: decoded callback received without an active buffer");
			return;
		}
		uint8_t *base = d->buffer->getData();
		const int size = d->buffer->getSize();
		if (!base || size <= 0) {
			blog(LOG_WARNING, "FalconM: decoded callback contains an empty buffer");
			return;
		}
		if (d->type == MEDIA_DATA_TYPE_AUDIO && d->format == MEDIA_DATA_FORMAT_PCM_S16) {
			obs_source_audio a = {};
			a.data[0] = base;
			a.samples_per_sec = d->sampleRate > 0 ? (uint32_t)d->sampleRate : 48000;
			a.speakers = d->channels == 1 ? SPEAKERS_MONO : SPEAKERS_STEREO;
			a.format = AUDIO_FORMAT_16BIT;
			a.frames = (uint32_t)(size / (std::max(1, d->channels) * 2));
			a.timestamp = d->pts >= 0 ? (uint64_t)d->pts * 1000 : 0;
			if (audio_cb_) {
				audio_cb_(a);
			}
			return;
		}
		if (d->type != MEDIA_DATA_TYPE_VIDEO || d->width <= 0 || d->height <= 0) {
			return;
		}
		obs_source_frame f = {};
		f.width = d->width;
		f.height = d->height;
		f.timestamp = d->pts >= 0 ? d->pts * 1000 : 0;
		if (d->format == MEDIA_DATA_FORMAT_NV12) {
			f.format = VIDEO_FORMAT_NV12;
			f.data[0] = base;
			f.linesize[0] = d->width;
			f.data[1] = base + d->width * d->height;
			f.linesize[1] = d->width;
		} else if (d->format == MEDIA_DATA_FORMAT_YUV420P) {
			const int y = d->width * d->height;
			const int uv = y / 4;
			f.format = VIDEO_FORMAT_I420;
			f.data[0] = base;
			f.linesize[0] = d->width;
			f.data[1] = base + y;
			f.linesize[1] = d->width / 2;
			f.data[2] = base + y + uv;
			f.linesize[2] = d->width / 2;
		} else {
			blog(LOG_WARNING, "FalconM: unsupported decoded format %d", d->format);
			return;
		}
		if (decoded_cb_) {
			decoded_cb_(f);
		}
	}
	void onRTPMessage(uint32_t ssrc, std::shared_ptr<MediaData> data) override
	{
		blog(LOG_INFO, "FalconM: onRTPMessage ssrc=%u data=%p", ssrc, data.get());
	}
	void onPeerMessage(rtcsdk::BLNSPClient &client, const rtcsdk::MQTTMessage &m) override
	{
		if (!m.payload && m.payloadlen != 0) {
			blog(LOG_ERROR, "FalconM: invalid signaling payload from '%s'", client.name.c_str());
			return;
		}
		if (signaling_cb_) {
			signaling_cb_(m.topic,
				      std::vector<uint8_t>((uint8_t *)m.payload, (uint8_t *)m.payload + m.payloadlen));
		}
	}
	void onNewSrtStream(const MediaStreamInfo &stream) override
	{
		blog(LOG_INFO, "[socket_source] new srt stream ssrc %u type %d format %d", stream.ssrc, stream.mediaType,
		     stream.mediaFormat);
		/* Wires up the SDK-internal decode pipeline (VideoDecoderIos/AudioDecoderIos);
		 * no surface/renderer is created on mac, only on Android. */
		if (stream.mediaType == blink::media::MEDIA_DATA_TYPE_VIDEO)
			session_->addSurface(stream.ssrc, rtcsdk::VideoRenderParams());
		else if (stream.mediaType == blink::media::MEDIA_DATA_TYPE_AUDIO)
			session_->addRemoteAudioPlayer(stream.ssrc);
	}
	void onDeleteSrtStream(const MediaStreamInfo &stream) override
	{
		blog(LOG_INFO, "FalconM: onDeleteSrtStream ssrc=%u", stream.ssrc);
		if (stream.mediaType == blink::media::MEDIA_DATA_TYPE_VIDEO)
			session_->removeSurface(stream.ssrc);
		else if (stream.mediaType == blink::media::MEDIA_DATA_TYPE_AUDIO)
			session_->removeRemoteAudioPlayer(stream.ssrc);
		streaming_ = false;
		blog(LOG_WARNING, "FalconM: SRT stream deleted, ssrc=%u", stream.ssrc);
	}
	void onVideoFormatChanged(uint32_t ssrc, int width, int height) override
	{
		blog(LOG_INFO, "FalconM: onVideoFormatChanged ssrc=%u size=%dx%d", ssrc, width, height);
	}
	void onSrtPullStates(const SrtPullStatesMessage &state) override
	{
		blog(LOG_INFO, "FalconM: onSrtPullStates quality=%d loss=%.2f%%", state.networkQualityLevel,
		     state.packetLossRate * 100.0);
		if (state.networkQualityLevel == 0) {
			blog(LOG_WARNING, "FalconM: SRT network quality is lowest, loss=%.2f%%", state.packetLossRate * 100.0);
		}
	}

	rtcsdk::BLRTCServerSession *session_ = nullptr;
	std::string device_id_;
	std::string controller_id_ = "uuid";
	std::atomic<bool> connected_{false};
	std::atomic<bool> streaming_{false};
	decoded_callback decoded_cb_;
	audio_callback audio_cb_;
	signaling_callback signaling_cb_;
};

std::unique_ptr<FalconMStream> falconm_stream_create()
{
	return std::make_unique<FalconMStreamSdk>();
}

} // namespace xbotgo
