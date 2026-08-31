# FalconM Direction Control Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add FalconM AYR direction control with motor-angle feedback and a per-device non-modal control window.

**Architecture:** Keep MQTT transport in `FalconMStream`, expose typed control/state callbacks through the FalconM source, and let a Qt window bind to one source without owning its connection. Encode protocol payloads in a small testable helper.

**Tech Stack:** C++17, Qt6, OBS source API, existing FalconM MQTT SDK, CMake/Xcode.

---

### Task 1: Protocol and stream API

**Files:** `plugins/xbotgo/falconm-stream.hpp`, `plugins/xbotgo/falconm-stream.cpp`, new protocol helper files.

- [ ] Define direction/operation enums, motor-angle state, and callbacks.
- [ ] Add AYR, BXR, and DGR send methods; parse BXA/DFA with bounds checks and big-endian signed angles.
- [ ] Dispatch parsed angle state from `onPeerMessage` while preserving the generic signaling callback.

### Task 2: FalconM source control bridge

**Files:** `plugins/xbotgo/falconm.hpp`, `plugins/xbotgo/falconm-source.cpp`, plugin public header/CMake.

- [ ] Forward control methods and angle callbacks from each source instance.
- [ ] Query angle and enable periodic reporting after peer connection; disable reporting before disconnect.
- [ ] Provide a source lookup/control API for the frontend without exposing SDK internals.

### Task 3: Control window and device-management integration

**Files:** new `frontend/dialogs/OBSBasicFalconMControl.*`, `OBSBasicFalconMDevices.*`, frontend CMake and localization.

- [ ] Add per-device non-modal window with angle/limit status and press/release direction buttons.
- [ ] Open or activate one window per selected FalconM source; cleanly unbind on source destruction.
- [ ] Add localized labels and error/connection status.

### Task 4: Verification

- [ ] Add protocol unit coverage for AYR values, signed angles, limit bytes, short packets, and unknown topics.
- [ ] Run formatting, XML/localization checks, targeted tests, and the macOS target build where environment permits.

