#include "falconm-stream.hpp"
#include "protocol/falcon-events.hpp"
#include <BLRTCServerSession.h>
#include <GlobalInit.h>
#include <log.h>
#include <util/base.h>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <CoreVideo/CoreVideo.h>
#include <mutex>
#include <unordered_map>

using namespace blink::media;

namespace xbotgo {
constexpr uint32_t kDefaultVideoSsrc = 600000;
constexpr uint32_t kDefaultAudioSsrc = 700000;
constexpr uint32_t kDefaultDataSsrc = 800000;
static void disable_media_sdk_logging()
{
	static const bool logging_disabled = [] {
		blink::media::init_logger([](int, const std::string &, const std::string &) {});
		return true;
	}();
	UNUSED_PARAMETER(logging_disabled);
}

static std::string payload_to_hex(const uint8_t *payload, size_t size)
{
	if (!payload && size != 0) {
		return "<null>";
	}
	static constexpr char hex[] = "0123456789abcdef";
	std::string result;
	result.reserve(size * 3);
	for (size_t i = 0; i < size; ++i) {
		if (i != 0) {
			result.push_back(' ');
		}
		result.push_back(hex[payload[i] >> 4]);
		result.push_back(hex[payload[i] & 0x0f]);
	}
	return result;
}

class FalconMStreamSdk final : public FalconMStream, private rtcsdk::BLRTCServerSessionListener {
public:
	FalconMStreamSdk() : instance_id_(++uniqueID)
	{
		blog(LOG_INFO, "FalconM: FalconMStreamSdk constructor this=%p uniqueID=%d", (void *)this, instance_id_);
		event_factories_.emplace(SupportedModesEvent::kTopic, []() -> std::unique_ptr<FalconEvent> {
			return std::make_unique<SupportedModesEvent>();
		});
		event_factories_.emplace(CaptureModeResultEvent::kTopic, []() -> std::unique_ptr<FalconEvent> {
			return std::make_unique<CaptureModeResultEvent>();
		});
		event_factories_.emplace(MotorAngleEvent::kTopic, []() -> std::unique_ptr<FalconEvent> {
			return std::make_unique<MotorAngleEvent>();
		});
		event_factories_.emplace(MotorAngleReportEvent::kTopic, []() -> std::unique_ptr<FalconEvent> {
			return std::make_unique<MotorAngleReportEvent>();
		});
		event_factories_.emplace(CaptureParametersEvent::kTopic, []() -> std::unique_ptr<FalconEvent> {
			return std::make_unique<CaptureParametersEvent>();
		});
		event_factories_.emplace(DefaultCaptureParametersEvent::kTopic, []() -> std::unique_ptr<FalconEvent> {
			return std::make_unique<DefaultCaptureParametersEvent>();
		});
		disable_media_sdk_logging();
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
		blog(LOG_INFO, "FalconM: FalconMStreamSdk destructor this=%p uniqueID=%d", (void *)this, instance_id_);
		disconnect();
		delete session_;
	}
	bool connect(const std::string &device_id, const std::string &broker_address, uint16_t broker_port) override
	{
		blog(LOG_INFO, "FalconM: connect this=%p uniqueID=%d controller_id='%s' device_id='%s' broker_address='%s' "
			      "broker_port=%u",
		     (void *)this, instance_id_, controller_id_.c_str(), device_id.c_str(), broker_address.c_str(), broker_port);
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
		controller_id_ = "uuid" + std::to_string(instance_id_);
		c.controlerId = controller_id_;
		c.deviceId = device_id_;
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
	bool startStreaming(const uint32_t video_ssrc = kDefaultVideoSsrc,
			    const uint32_t audio_ssrc = kDefaultAudioSsrc,
			    const uint32_t dataSsrc = kDefaultDataSsrc) override
	{
		if (!connected_ || streaming_) {
			return connected_ && streaming_;
		}
		rtcsdk::PullStreamParams p;
		p.srtPushCfg.videoSsrc = video_ssrc + instance_id_;
		p.srtPushCfg.audioSsrc = audio_ssrc + instance_id_;
		p.srtPushCfg.dataSsrc = dataSsrc + instance_id_;
		blog(LOG_INFO, "FalconM: startStreaming this=%p uniqueID=%d controller_id='%s' videoSsrc=%u audioSsrc=%u "
		              "dataSsrc=%u",
		     (void *)this, instance_id_, controller_id_.c_str(), p.srtPushCfg.videoSsrc, p.srtPushCfg.audioSsrc,
		     p.srtPushCfg.dataSsrc);
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
	bool isStreaming() const override { return streaming_; }
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
	void setDecodedFrameCallback(decoded_callback cb) override { decoded_cb_ = std::move(cb); }
	void setAudioCallback(audio_callback cb) override { audio_cb_ = std::move(cb); }
	void setSignalingCallback(signaling_callback cb) override { signaling_cb_ = std::move(cb); }
	void setSupportedModesCallback(supported_modes_callback cb) override
	{
		std::lock_guard<std::mutex> lock(supported_modes_mutex_);
		supported_modes_cb_ = std::move(cb);
	}
	bool send(const FalconRequest &request) override
	{
		const auto payload = request.encodePayload();
		if (!session_ || !connected_) {
			return false;
		}
		rtcsdk::MQTTMessage m;
		m.topic = std::string(request.topic());
		m.payload = const_cast<uint8_t *>(payload.data());
		m.payloadlen = (uint32_t)payload.size();
		const std::string payload_hex = payload_to_hex(payload.data(), payload.size());
		blog(LOG_INFO, "FalconM: sendPeerMessage device='%s' topic='%s' payloadlen=%u payload=%s",
		     device_id_.c_str(), m.topic.c_str(), m.payloadlen, payload_hex.c_str());
		return session_->sendPeerMessage(device_id_, m) == 0;
	}
	falconm_device_state state() const override
	{
		falconm_device_state result;
		std::scoped_lock lock(supported_modes_mutex_, capture_mode_mutex_, capture_parameters_mutex_,
				      angle_mutex_);
		result.supported_modes = supported_modes_;
		result.supported_modes_sequence = supported_modes_sequence_;
		result.capture_mode_result = capture_mode_result_;
		result.capture_parameters = capture_parameters_;
		result.capture_parameters_sequence = capture_parameters_sequence_;
		result.default_capture_parameters = default_capture_parameters_;
		result.default_capture_parameters_sequence = default_capture_parameters_sequence_;
		result.motor_angle = angle_;
		return result;
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
			send(SetMotorAngleReportingRequest{true});
			send(QueryMotorAngleRequest{});
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
		UNUSED_PARAMETER(ssrc);
		UNUSED_PARAMETER(data);
	}
	void onDecodedFrame(uint32_t ssrc, std::shared_ptr<MediaData> d) override
	{
		if (!d) {
			blog(LOG_ERROR, "FalconM: onDecodedFrame received null data, ssrc=%u", ssrc);
			return;
		}
		if (d->format == MEDIA_DATA_FORMAT_IMAGE_BUFFER) {
			if (!d->bufferObj) {
				blog(LOG_ERROR, "FalconM: IMAGE_BUFFER decoded frame has null bufferObj, ssrc=%u",
				     ssrc);
				return;
			}
			CVPixelBufferRef pixel_buffer = (CVPixelBufferRef)d->bufferObj->getObject();
			if (!pixel_buffer || CVPixelBufferLockBaseAddress(pixel_buffer, kCVPixelBufferLock_ReadOnly) !=
						     kCVReturnSuccess) {
				return;
			}
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
				blog(LOG_WARNING, "FalconM: unsupported CVPixelBuffer format 0x%08x",
				     (unsigned)pixel_format);
				CVPixelBufferUnlockBaseAddress(pixel_buffer, kCVPixelBufferLock_ReadOnly);
				return;
			}
			CFTypeRef matrix_attachment =
				CVBufferCopyAttachment(pixel_buffer, kCVImageBufferYCbCrMatrixKey, nullptr);
			if (matrix_attachment == kCVImageBufferYCbCrMatrix_ITU_R_601_4) {
				colorspace = VIDEO_CS_601;
			} else if (matrix_attachment == kCVImageBufferYCbCrMatrix_ITU_R_709_2) {
				colorspace = VIDEO_CS_709;
			} else if (matrix_attachment == kCVImageBufferYCbCrMatrix_ITU_R_2020) {
				colorspace = VIDEO_CS_2100_PQ;
			}
			CFTypeRef primaries_attachment =
				CVBufferCopyAttachment(pixel_buffer, kCVImageBufferColorPrimariesKey, nullptr);
			if (primaries_attachment == kCVImageBufferColorPrimaries_ITU_R_2020) {
				colorspace = VIDEO_CS_2100_PQ;
			}
			if (matrix_attachment) {
				CFRelease(matrix_attachment);
			}
			if (primaries_attachment) {
				CFRelease(primaries_attachment);
			}
			size_t plane_count = CVPixelBufferGetPlaneCount(pixel_buffer);
			for (size_t i = 0; i < plane_count && i < MAX_AV_PLANES; i++) {
				frame.data[i] = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, i);
				frame.linesize[i] = (uint32_t)CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, i);
			}
			video_format_get_parameters_for_format(colorspace, range, frame.format, frame.color_matrix,
							       frame.color_range_min, frame.color_range_max);
			if (decoded_cb_) {
				decoded_cb_(frame);
			}
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
		UNUSED_PARAMETER(ssrc);
		UNUSED_PARAMETER(data);
	}
	void dispatchEvent(const FalconEvent &event)
	{
		if (const auto *modes = dynamic_cast<const SupportedModesEvent *>(&event)) {
			supported_modes_callback callback;
			{
				std::lock_guard<std::mutex> lock(supported_modes_mutex_);
				supported_modes_ = modes->modes();
				++supported_modes_sequence_;
				callback = supported_modes_cb_;
			}
			if (callback) {
				callback(modes->modes());
			}
		} else if (const auto *capture = dynamic_cast<const CaptureModeResultEvent *>(&event)) {
			std::lock_guard<std::mutex> lock(capture_mode_mutex_);
			capture_mode_result_.success = capture->success();
			++capture_mode_result_.sequence;
		} else if (const auto *parameters = dynamic_cast<const CaptureParametersEvent *>(&event)) {
			std::lock_guard<std::mutex> lock(capture_parameters_mutex_);
			capture_parameters_ = parameters->parameters();
			++capture_parameters_sequence_;
		} else if (const auto *parameters = dynamic_cast<const DefaultCaptureParametersEvent *>(&event)) {
			std::lock_guard<std::mutex> lock(capture_parameters_mutex_);
			default_capture_parameters_ = parameters->parameters();
			++default_capture_parameters_sequence_;
		} else if (const auto *angle = dynamic_cast<const MotorAngleEvent *>(&event)) {
			std::lock_guard<std::mutex> lock(angle_mutex_);
			angle_ = angle->angle();
		} else if (const auto *report = dynamic_cast<const MotorAngleReportEvent *>(&event)) {
			std::lock_guard<std::mutex> lock(angle_mutex_);
			angle_ = report->angle();
		}
	}
	void onPeerMessage(rtcsdk::BLNSPClient &client, const rtcsdk::MQTTMessage &m) override
	{
		const auto *payload = static_cast<const uint8_t *>(m.payload);
		const std::string payload_hex = payload_to_hex(payload, m.payloadlen);
		blog(LOG_INFO, "FalconM: onPeerMessage client='%s' topic='%s' payloadlen=%u payload=%s",
		     client.name.c_str(), m.topic.c_str(), m.payloadlen, payload_hex.c_str());
		if (!m.payload && m.payloadlen != 0) {
			blog(LOG_ERROR, "FalconM: invalid signaling payload from '%s'", client.name.c_str());
			return;
		}
		const auto factory = event_factories_.find(m.topic);
		if (factory != event_factories_.end()) {
			auto event = factory->second();
			if (event->parse(payload, m.payloadlen)) {
				dispatchEvent(*event);
			} else {
				blog(LOG_WARNING, "FalconM: invalid %s payload, size=%u hex=%s", m.topic.c_str(),
				     m.payloadlen, payload_hex.c_str());
			}
		}
		if (signaling_cb_) {
			signaling_cb_(m.topic,
				      std::vector<uint8_t>((uint8_t *)m.payload, (uint8_t *)m.payload + m.payloadlen));
		}
	}
	void onNewSrtStream(const MediaStreamInfo &stream) override
	{
		// 	blog(LOG_INFO, "[socket_source] new srt stream ssrc %u type %d format %d", stream.ssrc,
		// 	     stream.mediaType, stream.mediaFormat);
		/* Wires up the SDK-internal decode pipeline (VideoDecoderIos/AudioDecoderIos);
		 * no surface/renderer is created on mac, only on Android. */
		if (stream.mediaType == blink::media::MEDIA_DATA_TYPE_VIDEO) {
			session_->addSurface(stream.ssrc, rtcsdk::VideoRenderParams());
		} else if (stream.mediaType == blink::media::MEDIA_DATA_TYPE_AUDIO) {
			session_->addRemoteAudioPlayer(stream.ssrc);
		}
	}
	void onDeleteSrtStream(const MediaStreamInfo &stream) override
	{
		blog(LOG_INFO, "FalconM: onDeleteSrtStream ssrc=%u", stream.ssrc);
		if (stream.mediaType == blink::media::MEDIA_DATA_TYPE_VIDEO) {
			session_->removeSurface(stream.ssrc);
		} else if (stream.mediaType == blink::media::MEDIA_DATA_TYPE_AUDIO) {
			session_->removeRemoteAudioPlayer(stream.ssrc);
		}
		streaming_ = false;
		blog(LOG_WARNING, "FalconM: SRT stream deleted, ssrc=%u", stream.ssrc);
	}
	void onVideoFormatChanged(uint32_t ssrc, int width, int height) override
	{
		blog(LOG_INFO, "FalconM: onVideoFormatChanged ssrc=%u size=%dx%d", ssrc, width, height);
	}
	void onSrtPullStates(const SrtPullStatesMessage &state) override
	{
		// blog(LOG_INFO, "FalconM: onSrtPullStates quality=%d loss=%.2f%%", state.networkQualityLevel,
		//      state.packetLossRate * 100.0);
		if (state.networkQualityLevel == 0) {
			blog(LOG_WARNING, "FalconM: SRT network quality is lowest, loss=%.2f%%",
			     state.packetLossRate * 100.0);
		}
	}

	rtcsdk::BLRTCServerSession *session_ = nullptr;
	static int uniqueID;
	const int instance_id_;
	std::string device_id_;
	std::string controller_id_ = "uuid";
	std::unordered_map<std::string, falcon_event_factory> event_factories_;
	std::atomic<bool> connected_{false};
	std::atomic<bool> streaming_{false};
	mutable std::mutex angle_mutex_;
	falconm_motor_angle angle_;
	mutable std::mutex supported_modes_mutex_;
	falconm_supported_modes supported_modes_;
	uint64_t supported_modes_sequence_ = 0;
	mutable std::mutex capture_mode_mutex_;
	falconm_capture_mode_result capture_mode_result_;
	mutable std::mutex capture_parameters_mutex_;
	falconm_capture_parameters capture_parameters_;
	uint64_t capture_parameters_sequence_ = 0;
	falconm_capture_parameters default_capture_parameters_;
	uint64_t default_capture_parameters_sequence_ = 0;
	decoded_callback decoded_cb_;
	audio_callback audio_cb_;
	signaling_callback signaling_cb_;
	supported_modes_callback supported_modes_cb_;
};

int FalconMStreamSdk::uniqueID = 0;

std::unique_ptr<FalconMStream> falconm_stream_create()
{
	return std::make_unique<FalconMStreamSdk>();
}

} // namespace xbotgo
