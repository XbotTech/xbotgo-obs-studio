#include "falconm-stream.hpp"
#include "falconm-log.hpp"
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
constexpr uint32_t kDefaultMultiCamPullPort = 61018;
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
		FALCONM_LOG_INFO("FalconM: this=%p uniqueID=%d FalconMStreamSdk constructor", (void *)this,
				 instance_id_);
		event_factories_.emplace(HallCalibrationStatusEvent::kTopic, []() -> std::unique_ptr<FalconEvent> {
			return std::make_unique<HallCalibrationStatusEvent>();
		});
		event_factories_.emplace(CurrentZoomEvent::kTopic, []() -> std::unique_ptr<FalconEvent> {
			return std::make_unique<CurrentZoomEvent>();
		});
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
//		disable_media_sdk_logging();
		bool initialized_now = false;
		std::call_once(global_init_once_, [&initialized_now] {
			global_init_result_ = blink::utils::GlobalInit::getInstance().init(blink::utils::GlobalConfig());
			initialized_now = true;
		});
		if (global_init_result_ == 0) {
			FALCONM_LOG_INFO("FalconM: this=%p uniqueID=%d Media SDK GlobalInit result=%d initialized_now=%s",
					 (void *)this, instance_id_, global_init_result_, initialized_now ? "true" : "false");
		} else {
			blog(LOG_ERROR, "FalconM: this=%p uniqueID=%d Media SDK GlobalInit result=%d initialized_now=%s",
			     (void *)this, instance_id_, global_init_result_, initialized_now ? "true" : "false");
		}
		session_ = rtcsdk::BLRTCServerSession::create(rtcsdk::RTC_SESSION_TYPE_DRAGONFLY);
		if (session_) {
			session_->addListener(this);
		} else {
			blog(LOG_ERROR, "FalconM: this=%p uniqueID=%d failed to create Dragonfly server session", (void *)this,
			     instance_id_);
		}
	}
	~FalconMStreamSdk() override
	{
		FALCONM_LOG_INFO("FalconM: this=%p uniqueID=%d FalconMStreamSdk destructor", (void *)this,
				 instance_id_);
		disconnect();
		delete session_;
	}
	bool connect(const std::string &device_id, const std::string &broker_address, uint16_t broker_port) override
	{
		FALCONM_LOG_INFO(
			"FalconM: this=%p uniqueID=%d connect controller_id='%s' device_id='%s' broker_address='%s' "
			"broker_port=%u",
			(void *)this, instance_id_, controller_id_.c_str(), device_id.c_str(), broker_address.c_str(),
			broker_port);
		if (!session_ || broker_address.empty() || device_id.empty()) {
			blog(LOG_ERROR, "FalconM: this=%p uniqueID=%d invalid session or connection settings", (void *)this,
			     instance_id_);
			return false;
		}
		device_id_ = device_id;
		broker_address_ = broker_address;
		if (session_->startSession() != 0) {
			blog(LOG_ERROR, "FalconM: this=%p uniqueID=%d BLRTCServerSession startSession failed", (void *)this,
			     instance_id_);
			return false;
		}
		blink::signaling::ConnectClientConfig c;
		controller_id_ = "uuid" + std::to_string(instance_id_);
		c.controlerId = controller_id_;
		c.deviceId = device_id_;
		c.ipv4List.push_back(broker_address);
		c.mqttBrokerPort = broker_port;
		if (session_->connectPeerSession(c) != 0) {
			blog(LOG_ERROR, "FalconM: this=%p uniqueID=%d connectPeerSession failed for device '%s'", (void *)this,
			     instance_id_, device_id_.c_str());
			session_->stopSession();
			return false;
		}
		connected_ = true;
		return true;
	}
	bool startStreaming(const uint32_t video_ssrc = kDefaultVideoSsrc,
			    const uint32_t audio_ssrc = kDefaultAudioSsrc,
			    const uint32_t data_ssrc = kDefaultDataSsrc,
			    const falconm_video_encoder_options &encoder_options = {}) override
	{
		if (!connected_ || streaming_) {
			return connected_ && streaming_;
		}
		const auto valid_encoder_option = [](const std::optional<int> &value) {
			return !value || *value > 0;
		};
		if (!valid_encoder_option(encoder_options.width) || !valid_encoder_option(encoder_options.height) ||
		    !valid_encoder_option(encoder_options.fps) || !valid_encoder_option(encoder_options.bitrate)) {
			blog(LOG_ERROR,
			     "FalconM: this=%p uniqueID=%d startStreaming rejected invalid encoder options width=%d "
			     "height=%d fps=%d bitrate=%d",
			     (void *)this, instance_id_, encoder_options.width.value_or(-1),
			     encoder_options.height.value_or(-1), encoder_options.fps.value_or(-1),
			     encoder_options.bitrate.value_or(-1));
			return false;
		}
		rtcsdk::PullStreamParams p;
		p.srtPushCfg.videoSsrc = video_ssrc + instance_id_;
		p.srtPushCfg.audioSsrc = audio_ssrc + instance_id_;
		p.srtPushCfg.dataSsrc = data_ssrc + instance_id_;
		p.srtPullCfg.srtServerPort = kDefaultMultiCamPullPort + instance_id_;
		if (encoder_options.width) {
			p.videoEncodeCfg.width = *encoder_options.width;
		}
		if (encoder_options.height) {
			p.videoEncodeCfg.height = *encoder_options.height;
		}
		if (encoder_options.fps) {
			p.videoEncodeCfg.fps = *encoder_options.fps;
		}
		if (encoder_options.bitrate) {
			p.videoEncodeCfg.bitrate = *encoder_options.bitrate;
		}
		FALCONM_LOG_INFO(
			"FalconM: this=%p uniqueID=%d startStreaming controller_id='%s' videoSsrc=%u audioSsrc=%u "
			"dataSsrc=%u srtServerPort=%u encoderWidth=%d encoderHeight=%d encoderFps=%d encoderBitrate=%d",
			(void *)this, instance_id_, controller_id_.c_str(), p.srtPushCfg.videoSsrc, p.srtPushCfg.audioSsrc,
			p.srtPushCfg.dataSsrc, p.srtPullCfg.srtServerPort, p.videoEncodeCfg.width, p.videoEncodeCfg.height,
			p.videoEncodeCfg.fps, p.videoEncodeCfg.bitrate);
		if (session_->startPullStream(device_id_, p) != 0) {
			blog(LOG_ERROR, "FalconM: this=%p uniqueID=%d startPullStream failed for device '%s'", (void *)this,
			     instance_id_, device_id_.c_str());
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
			blog(LOG_ERROR, "FalconM: this=%p uniqueID=%d stopPullStream failed for device '%s'", (void *)this,
			     instance_id_, device_id_.c_str());
		}
		return result == 0;
	}
	bool isStreaming() const override { return streaming_; }
	void disconnect() override
	{
		send(SetMotorAngleReportingRequest{false});
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
		FALCONM_LOG_INFO(
			"FalconM: this=%p uniqueID=%d sendPeerMessage device='%s' topic='%s' payloadlen=%u payload=%s",
			(void *)this, instance_id_, device_id_.c_str(), m.topic.c_str(), m.payloadlen, payload_hex.c_str());
		return session_->sendPeerMessage(device_id_, m) == 0;
	}
	falconm_device_state state() const override
	{
		falconm_device_state result;
		std::scoped_lock lock(supported_modes_mutex_, capture_mode_mutex_, capture_parameters_mutex_,
				      angle_mutex_, hall_calibration_mutex_, current_zoom_mutex_);
		result.supported_modes = supported_modes_;
		result.supported_modes_sequence = supported_modes_sequence_;
		result.capture_mode_result = capture_mode_result_;
		result.capture_parameters = capture_parameters_;
		result.capture_parameters_sequence = capture_parameters_sequence_;
		result.default_capture_parameters = default_capture_parameters_;
		result.default_capture_parameters_sequence = default_capture_parameters_sequence_;
		result.motor_angle = angle_;
		result.hall_calibration_status = hall_calibration_status_;
		result.hall_calibration_sequence = hall_calibration_sequence_;
		result.current_zoom = current_zoom_;
		result.current_zoom_sequence = current_zoom_sequence_;
		return result;
	}

private:
	void onPeerDevicesRefresh(rtcsdk::BLNSPClient &client) override
	{
		FALCONM_LOG_INFO("FalconM: this=%p uniqueID=%d onPeerDevicesRefresh client=%s", (void *)this,
				 instance_id_, client.name.c_str());
	}
	void onPeerConnectStatus(rtcsdk::BLNSPClient &client, int status) override
	{
		FALCONM_LOG_INFO(
			"FalconM: this=%p uniqueID=%d onPeerConnectStatus client=%s status=%d device_id='%s' "
			"broker_address='%s'",
			(void *)this, instance_id_, client.name.c_str(), status, device_id_.c_str(), broker_address_.c_str());
		connected_ = status == rtcsdk::PEER_CONNECTION_STATUS_CONNECTED;
		if (connected_) {
			send(SetMotorAngleReportingRequest{true});
			send(QueryMotorAngleRequest{});
			send(QueryHallCalibrationRequest{});
			send(QueryCurrentZoomRequest{});
			if (!startStreaming()) {
				blog(LOG_ERROR, "FalconM: this=%p uniqueID=%d startStreaming failed after peer connection",
				     (void *)this, instance_id_);
			}
		} else {
			blog(LOG_ERROR, "FalconM: this=%p uniqueID=%d peer connection failed, status=%d", (void *)this,
			     instance_id_, status);
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
			blog(LOG_ERROR, "FalconM: this=%p uniqueID=%d onDecodedFrame received null data, ssrc=%u", (void *)this,
			     instance_id_, ssrc);
			return;
		}
		FALCONM_LOG_INFO(
			"FalconM: this=%p uniqueID=%d onDecodedFrame ssrc=%u data=%p type=%d format=%d streaming=%s "
			"buffer=%p bufferObj=%p width=%d height=%d sampleRate=%d channels=%d bitsPerSample=%d "
			"pts=%lld dts=%lld fromNodeId=%d toNodeId=%d rotation=%d",
			(void *)this, instance_id_, ssrc, (void *)d.get(), d->type, d->format,
			streaming_ ? "true" : "false", (void *)d->buffer.get(), (void *)d->bufferObj.get(), d->width,
			d->height, d->sampleRate, d->channels, d->bitsPerSample, (long long)d->pts, (long long)d->dts,
			d->fromNodeId, d->toNodeId, d->rotation);
		if (d->format == MEDIA_DATA_FORMAT_IMAGE_BUFFER) {
			if (!d->bufferObj) {
				blog(LOG_ERROR,
				     "FalconM: this=%p uniqueID=%d IMAGE_BUFFER decoded frame has null bufferObj, ssrc=%u",
				     (void *)this, instance_id_, ssrc);
				return;
			}
			CVPixelBufferRef pixel_buffer = (CVPixelBufferRef)d->bufferObj->getObject();
			if (!pixel_buffer) {
				blog(LOG_ERROR,
				     "FalconM: this=%p uniqueID=%d IMAGE_BUFFER GenericObject returned null CVPixelBuffer, ssrc=%u "
				     "bufferObj=%p",
				     (void *)this, instance_id_, ssrc, (void *)d->bufferObj.get());
				return;
			}
			const CVReturn lock_result = CVPixelBufferLockBaseAddress(pixel_buffer, kCVPixelBufferLock_ReadOnly);
			if (lock_result != kCVReturnSuccess) {
				blog(LOG_ERROR,
				     "FalconM: this=%p uniqueID=%d CVPixelBufferLockBaseAddress failed, ssrc=%u "
				     "pixelBuffer=%p result=%d",
				     (void *)this, instance_id_, ssrc, (void *)pixel_buffer, lock_result);
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
				blog(LOG_WARNING, "FalconM: this=%p uniqueID=%d unsupported CVPixelBuffer format 0x%08x",
				     (void *)this, instance_id_, (unsigned)pixel_format);
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
			} else {
				blog(LOG_WARNING,
				     "FalconM: this=%p uniqueID=%d IMAGE_BUFFER frame dropped because decoded callback is empty, "
				     "ssrc=%u size=%ux%u",
				     (void *)this, instance_id_, ssrc, frame.width, frame.height);
			}
			CVPixelBufferUnlockBaseAddress(pixel_buffer, kCVPixelBufferLock_ReadOnly);
			return;
		}
		if (!d->buffer) {
			blog(LOG_WARNING,
			     "FalconM: this=%p uniqueID=%d decoded frame rejected because buffer is null, ssrc=%u type=%d "
			     "format=%d streaming=%s",
			     (void *)this, instance_id_, ssrc, d->type, d->format, streaming_ ? "true" : "false");
			return;
		}
		if (!streaming_) {
			blog(LOG_WARNING,
			     "FalconM: this=%p uniqueID=%d decoded frame rejected because streaming is false, ssrc=%u "
			     "type=%d format=%d buffer=%p",
			     (void *)this, instance_id_, ssrc, d->type, d->format, (void *)d->buffer.get());
			return;
		}
		uint8_t *base = d->buffer->getData();
		const int size = d->buffer->getSize();
		if (!base) {
			blog(LOG_WARNING,
			     "FalconM: this=%p uniqueID=%d decoded frame rejected because buffer data is null, ssrc=%u "
			     "type=%d format=%d buffer=%p size=%d",
			     (void *)this, instance_id_, ssrc, d->type, d->format, (void *)d->buffer.get(), size);
			return;
		}
		if (size <= 0) {
			blog(LOG_WARNING,
			     "FalconM: this=%p uniqueID=%d decoded frame rejected because buffer size is invalid, ssrc=%u "
			     "type=%d format=%d data=%p size=%d",
			     (void *)this, instance_id_, ssrc, d->type, d->format, (void *)base, size);
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
			} else {
				blog(LOG_WARNING,
				     "FalconM: this=%p uniqueID=%d audio frame dropped because audio callback is empty, ssrc=%u "
				     "sampleRate=%u channels=%d frames=%u",
				     (void *)this, instance_id_, ssrc, a.samples_per_sec, d->channels, a.frames);
			}
			return;
		}
		if (d->type != MEDIA_DATA_TYPE_VIDEO) {
			blog(LOG_WARNING,
			     "FalconM: this=%p uniqueID=%d decoded frame rejected because media type is not video and is not "
			     "supported PCM audio, ssrc=%u type=%d format=%d",
			     (void *)this, instance_id_, ssrc, d->type, d->format);
			return;
		}
		if (d->width <= 0) {
			blog(LOG_WARNING,
			     "FalconM: this=%p uniqueID=%d decoded video frame rejected because width is invalid, ssrc=%u "
			     "format=%d width=%d height=%d size=%d",
			     (void *)this, instance_id_, ssrc, d->format, d->width, d->height, size);
			return;
		}
		if (d->height <= 0) {
			blog(LOG_WARNING,
			     "FalconM: this=%p uniqueID=%d decoded video frame rejected because height is invalid, ssrc=%u "
			     "format=%d width=%d height=%d size=%d",
			     (void *)this, instance_id_, ssrc, d->format, d->width, d->height, size);
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
			blog(LOG_WARNING, "FalconM: this=%p uniqueID=%d unsupported decoded format %d", (void *)this,
			     instance_id_, d->format);
			return;
		}
		if (decoded_cb_) {
			decoded_cb_(f);
		} else {
			blog(LOG_WARNING,
			     "FalconM: this=%p uniqueID=%d video frame dropped because decoded callback is empty, ssrc=%u "
			     "format=%d size=%dx%d",
			     (void *)this, instance_id_, ssrc, d->format, d->width, d->height);
		}
	}
	void onRTPMessage(uint32_t ssrc, std::shared_ptr<MediaData> data) override
	{
		UNUSED_PARAMETER(ssrc);
		UNUSED_PARAMETER(data);
	}
	void dispatchEvent(const FalconEvent &event)
	{
		if (const auto *hall = dynamic_cast<const HallCalibrationStatusEvent *>(&event)) {
			std::lock_guard<std::mutex> lock(hall_calibration_mutex_);
			hall_calibration_status_ = hall->status();
			++hall_calibration_sequence_;
		} else if (const auto *zoom = dynamic_cast<const CurrentZoomEvent *>(&event)) {
			std::lock_guard<std::mutex> lock(current_zoom_mutex_);
			current_zoom_ = zoom->value();
			++current_zoom_sequence_;
		} else if (const auto *modes = dynamic_cast<const SupportedModesEvent *>(&event)) {
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
		FALCONM_LOG_INFO(
			"FalconM: this=%p uniqueID=%d onPeerMessage client='%s' topic='%s' payloadlen=%u payload=%s",
			(void *)this, instance_id_, client.name.c_str(), m.topic.c_str(), m.payloadlen, payload_hex.c_str());
		if (!m.payload && m.payloadlen != 0) {
			blog(LOG_ERROR, "FalconM: this=%p uniqueID=%d invalid signaling payload from '%s'", (void *)this,
			     instance_id_, client.name.c_str());
			return;
		}
		const auto factory = event_factories_.find(m.topic);
		if (factory != event_factories_.end()) {
			auto event = factory->second();
			if (event->parse(payload, m.payloadlen)) {
				dispatchEvent(*event);
			} else {
				blog(LOG_WARNING, "FalconM: this=%p uniqueID=%d invalid %s payload, size=%u hex=%s", (void *)this,
				     instance_id_, m.topic.c_str(), m.payloadlen, payload_hex.c_str());
			}
		}
		if (signaling_cb_) {
			signaling_cb_(m.topic,
				      std::vector<uint8_t>((uint8_t *)m.payload, (uint8_t *)m.payload + m.payloadlen));
		}
	}
	void onNewSrtStream(const MediaStreamInfo &stream) override
	{
		FALCONM_LOG_INFO(
			"FalconM: this=%p uniqueID=%d [socket_source] new srt stream ssrc=%u type=%d format=%d",
			(void *)this, instance_id_, stream.ssrc, stream.mediaType, stream.mediaFormat);
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
		FALCONM_LOG_INFO("FalconM: this=%p uniqueID=%d onDeleteSrtStream ssrc=%u", (void *)this, instance_id_,
				 stream.ssrc);
		if (stream.mediaType == blink::media::MEDIA_DATA_TYPE_VIDEO) {
			session_->removeSurface(stream.ssrc);
		} else if (stream.mediaType == blink::media::MEDIA_DATA_TYPE_AUDIO) {
			session_->removeRemoteAudioPlayer(stream.ssrc);
		}
		streaming_ = false;
		blog(LOG_WARNING, "FalconM: this=%p uniqueID=%d SRT stream deleted, ssrc=%u", (void *)this, instance_id_,
		     stream.ssrc);
	}
	void onVideoFormatChanged(uint32_t ssrc, int width, int height) override
	{
		FALCONM_LOG_INFO("FalconM: this=%p uniqueID=%d onVideoFormatChanged ssrc=%u size=%dx%d", (void *)this,
				 instance_id_, ssrc, width, height);
	}
	void onSrtPullStates(const SrtPullStatesMessage &state) override
	{
		// FALCONM_LOG_INFO("FalconM: this=%p uniqueID=%d onSrtPullStates quality=%d loss=%.2f%%", (void *)this,
		//                  instance_id_, state.networkQualityLevel, state.packetLossRate * 100.0);
		if (state.networkQualityLevel == 0) {
			blog(LOG_WARNING, "FalconM: this=%p uniqueID=%d SRT network quality is lowest, loss=%.2f%%",
			     (void *)this, instance_id_, state.packetLossRate * 100.0);
		}
	}

	rtcsdk::BLRTCServerSession *session_ = nullptr;
	static int uniqueID;
	static std::once_flag global_init_once_;
	static int global_init_result_;
	const int instance_id_;
	std::string device_id_;
	std::string broker_address_;
	std::string controller_id_ = "uuid";
	std::unordered_map<std::string, falcon_event_factory> event_factories_;
	std::atomic<bool> connected_{false};
	std::atomic<bool> streaming_{false};
	mutable std::mutex angle_mutex_;
	falconm_motor_angle angle_;
	mutable std::mutex hall_calibration_mutex_;
	falconm_hall_calibration_status hall_calibration_status_ = falconm_hall_calibration_status::uncalibrated;
	uint64_t hall_calibration_sequence_ = 0;
	mutable std::mutex current_zoom_mutex_;
	uint8_t current_zoom_ = 10;
	uint64_t current_zoom_sequence_ = 0;
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
std::once_flag FalconMStreamSdk::global_init_once_;
int FalconMStreamSdk::global_init_result_ = -1;

std::unique_ptr<FalconMStream> falconm_stream_create()
{
	return std::make_unique<FalconMStreamSdk>();
}

} // namespace xbotgo
