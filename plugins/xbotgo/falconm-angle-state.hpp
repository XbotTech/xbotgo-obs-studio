#pragma once

#include "protocol/falcon-protocol-parser.hpp"

#include <functional>
#include <mutex>
#include <utility>

namespace xbotgo {

class FalconMAngleState {
public:
	using report_callback = std::function<void(const falconm_motor_angle &)>;

	void setReportCallback(report_callback callback)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		reportCallback_ = std::move(callback);
	}

	void updateQuery(const falconm_motor_angle &angle)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		angle_ = angle;
	}

	void updateReport(const falconm_motor_angle &angle)
	{
		report_callback callback;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			angle_ = angle;
			callback = reportCallback_;
		}
		if (callback) {
			callback(angle);
		}
	}

	falconm_motor_angle snapshot() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return angle_;
	}

private:
	mutable std::mutex mutex_;
	falconm_motor_angle angle_;
	report_callback reportCallback_;
};

} // namespace xbotgo
