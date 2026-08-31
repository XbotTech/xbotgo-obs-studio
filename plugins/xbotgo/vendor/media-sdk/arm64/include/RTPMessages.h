#pragma once
#include <cstdint>
#include <vector>

namespace blink {
namespace media {


struct ObjectDetectResultBox {
  // label is the class label of the detected object
  // this is related to the AI model used for detection
  // 0: person, for all model
  // 1: ball, for multi-detection model
  // 2: ball board, for multi-detection model
  int32_t label;
  // the coordinates of the bounding box
  // x, y are the top-left corner of the box
  // width, height are the dimensions of the box
  // all coordinates are in percent (0.0-1.0) of the image size
  float x;
  float y;
  float width;
  float height;
  // confidence is the detection confidence score (0.0-1.0)
  float confidence;
  ObjectDetectResultBox() : label(0), x(0.0f), y(0.0f), width(0.0f), height(0.0f), confidence(0.0f) {}
};

struct ObjectDetectResult {
  int64_t pts;
  float yaw;
  float pitch;
  std::vector<ObjectDetectResultBox> boxes;
  ObjectDetectResult() : pts(0), yaw(0.0f), pitch(0.0f) {}
};


}
}
