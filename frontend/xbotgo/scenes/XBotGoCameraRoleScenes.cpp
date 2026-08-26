#include "XBotGoCameraRoleScenes.hpp"

#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include <cstring>
#include <vector>

namespace XBotGo {
namespace {

struct CameraRoleSceneConfig {
	CameraRole role;
	const char *sceneUuidSetting;
	const char *sceneName;
};

constexpr CameraRoleSceneConfig CameraRoleScenes[] = {
	{CameraRole::Center, "xbotgo_camera_role_scene_center_uuid", "XBotGo-Center"},
	{CameraRole::Left, "xbotgo_camera_role_scene_left_uuid", "XBotGo-Left"},
	{CameraRole::Right, "xbotgo_camera_role_scene_right_uuid", "XBotGo-Right"},
};

struct SceneItemState {
	obs_transform_info transform{};
	obs_sceneitem_crop crop{};
	bool visible = true;
	bool locked = false;
	obs_blending_method blendMethod = OBS_BLEND_METHOD_DEFAULT;
	obs_blending_type blendMode = OBS_BLEND_NORMAL;
	obs_scale_type scaleFilter = OBS_SCALE_DISABLE;
	OBSData showTransition;
	OBSData hideTransition;
	OBSData privateSettings;
};

const CameraRoleSceneConfig *FindCameraRoleSceneConfig(CameraRole role)
{
	for (const auto &config : CameraRoleScenes) {
		if (role == config.role) {
			return &config;
		}
	}
	return nullptr;
}

/**
 * 遍历当前 OBS 场景集合中的所有场景，包括普通场景和多 Canvas 场景。
 *
 * @tparam Callback
 * @param main     OBS 主窗口，用于取得所有 Canvas。
 * @param callback 每找到一个场景时执行的回调。
 * @param param    透传给回调的上下文数据。
 */
template<typename Callback> void EnumSceneCollectionScenes(OBSBasic &main, Callback callback, void *param)
{
	// 1. 先遍历 OBS 默认场景集合里的场景
	obs_enum_scenes(callback, param);

	// 2. 然后遍历所有 Canvas：
	for (const auto &canvas : main.GetCanvases()) {
		// 跳过带有 EPHEMERAL 标志的临时 Canvas。
		// 临时 Canvas 通常用于预览或内部工作，不应参与场景角色查找或项目持久化
		if (!(obs_canvas_get_flags(canvas) & EPHEMERAL)) {
			// 遍历该持久化 Canvas 中的场景，并调用同一个回调。
			obs_canvas_enum_scenes(canvas, callback, param);
		}
	}
}

OBSSource FindMarkedCameraRoleScene(OBSBasic &main, const CameraRoleSceneConfig &config)
{
	struct FindSceneData {
		const CameraRoleSceneConfig &config;
		OBSSource sceneSource;
	} data{config};

	EnumSceneCollectionScenes(
		main,
		[](void *param, obs_source_t *sceneSource) {
			auto &data = *static_cast<FindSceneData *>(param);
			if (data.sceneSource) {
				// 已经找到sceneSource，返回false表示不再遍历
				return false;
			}

			if (obs_source_is_group(sceneSource)) {
				// 排除 Group，返回true表示继续遍历循环
				return true;
			}

			OBSDataAutoRelease privateSettings = obs_source_get_private_settings(sceneSource);
			const char *registeredUuid = obs_data_get_string(privateSettings, data.config.sceneUuidSetting);
			const char *actualUuid = obs_source_get_uuid(sceneSource);
			if (registeredUuid && actualUuid && strcmp(registeredUuid, actualUuid) == 0) {
				data.sceneSource = sceneSource;
				return false;
			}
			return true;
		},
		&data);

	return data.sceneSource;
}

OBSSource FindNamedCameraRoleScene(const CameraRoleSceneConfig &config)
{
	OBSSourceAutoRelease sceneSource = obs_get_source_by_name(config.sceneName);
	if (!sceneSource || obs_source_is_group(sceneSource) || !obs_scene_from_source(sceneSource)) {
		return {};
	}
	return sceneSource.Get();
}

OBSSource FindCameraRoleScene(OBSBasic &main, const CameraRoleSceneConfig &config)
{
	OBSSource sceneSource = FindMarkedCameraRoleScene(main, config);
	return sceneSource ? sceneSource : FindNamedCameraRoleScene(config);
}

void MarkCameraRoleScene(obs_source_t *sceneSource, const CameraRoleSceneConfig &config)
{
	const char *sceneUuid = obs_source_get_uuid(sceneSource);
	if (!sceneUuid) {
		return;
	}

	OBSDataAutoRelease privateSettings = obs_source_get_private_settings(sceneSource);
	obs_data_set_string(privateSettings, config.sceneUuidSetting, sceneUuid);
}

OBSSource GetOrCreateCameraRoleScene(OBSBasic &main, const CameraRoleSceneConfig &config)
{
	OBSSource sceneSource = FindCameraRoleScene(main, config);
	if (!sceneSource) {
		OBSSceneAutoRelease createdScene = obs_scene_create(config.sceneName);
		obs_source_t *createdSceneSource = createdScene ? obs_scene_get_source(createdScene) : nullptr;
		if (!createdSceneSource || obs_source_is_group(createdSceneSource) ||
		    !obs_scene_from_source(createdSceneSource)) {
			return {};
		}
		sceneSource = createdSceneSource;
	}

	MarkCameraRoleScene(sceneSource, config);
	return sceneSource;
}

std::vector<OBSSceneItem> FindSourceSceneItems(OBSBasic &main, obs_source_t *source)
{
	struct FindItemsData {
		obs_source_t *source;
		std::vector<OBSSceneItem> items;
	} data{source};

	EnumSceneCollectionScenes(
		main,
		[](void *param, obs_source_t *sceneSource) {
			auto &data = *static_cast<FindItemsData *>(param);
			OBSScene scene = obs_group_or_scene_from_source(sceneSource);
			if (!scene) {
				return true;
			}
			obs_scene_enum_items(
				scene,
				[](obs_scene_t *, obs_sceneitem_t *item, void *param) {
					auto &data = *static_cast<FindItemsData *>(param);
					if (obs_sceneitem_get_source(item) == data.source) {
						data.items.emplace_back(item);
					}
					return true;
				},
				&data);
			return true;
		},
		&data);

	return data.items;
}

SceneItemState CaptureSceneItemState(obs_sceneitem_t *item)
{
	SceneItemState state;
	obs_sceneitem_get_info2(item, &state.transform);
	obs_sceneitem_get_crop(item, &state.crop);
	state.visible = obs_sceneitem_visible(item);
	state.locked = obs_sceneitem_locked(item);
	state.blendMethod = obs_sceneitem_get_blending_method(item);
	state.blendMode = obs_sceneitem_get_blending_mode(item);
	state.scaleFilter = obs_sceneitem_get_scale_filter(item);
	OBSDataAutoRelease showTransition = obs_sceneitem_transition_save(item, true);
	OBSDataAutoRelease hideTransition = obs_sceneitem_transition_save(item, false);
	state.showTransition = showTransition.Get();
	state.hideTransition = hideTransition.Get();

	OBSDataAutoRelease privateSettings = obs_sceneitem_get_private_settings(item);
	OBSDataAutoRelease copiedPrivateSettings = obs_data_create();
	obs_data_apply(copiedPrivateSettings, privateSettings);
	state.privateSettings = copiedPrivateSettings.Get();
	return state;
}

OBSSceneItem AddSourceToScene(obs_scene_t *scene, obs_source_t *source, const SceneItemState *state)
{
	struct AddItemData {
		obs_source_t *source;
		const SceneItemState *state;
		obs_sceneitem_t *item = nullptr;
	} data{source, state};

	obs_scene_atomic_update(
		scene,
		[](void *param, obs_scene_t *scene) {
			auto &data = *static_cast<AddItemData *>(param);
			data.item = obs_scene_add(scene, data.source);
			if (!data.item || !data.state) {
				return;
			}

			const SceneItemState &state = *data.state;
			obs_sceneitem_defer_update_begin(data.item);
			obs_sceneitem_set_info2(data.item, &state.transform);
			obs_sceneitem_set_crop(data.item, &state.crop);
			obs_sceneitem_set_blending_method(data.item, state.blendMethod);
			obs_sceneitem_set_blending_mode(data.item, state.blendMode);
			obs_sceneitem_set_scale_filter(data.item, state.scaleFilter);
			obs_sceneitem_transition_load(data.item, state.showTransition, true);
			obs_sceneitem_transition_load(data.item, state.hideTransition, false);

			OBSDataAutoRelease privateSettings = obs_sceneitem_get_private_settings(data.item);
			obs_data_apply(privateSettings, state.privateSettings);
			obs_sceneitem_set_visible(data.item, state.visible);
			obs_sceneitem_set_locked(data.item, state.locked);
			obs_sceneitem_defer_update_end(data.item);
		},
		&data);

	return OBSSceneItem(data.item);
}

OBSSceneItem FindPreferredSceneItem(obs_scene_t *currentScene, obs_source_t *source,
				    const std::vector<OBSSceneItem> &items)
{
	if (currentScene) {
		obs_sceneitem_t *currentItem =
			obs_scene_find_source_recursive(currentScene, obs_source_get_name(source));
		if (currentItem && obs_sceneitem_get_source(currentItem) == source) {
			return OBSSceneItem(currentItem);
		}
	}
	return items.empty() ? OBSSceneItem() : items.front();
}

} // namespace

const char *CameraRoleSceneName(CameraRole role)
{
	const CameraRoleSceneConfig *config = FindCameraRoleSceneConfig(role);
	return config ? config->sceneName : "";
}

std::optional<CameraRole> GetSourceCameraRole(OBSBasic &main, obs_source_t *source)
{
	if (!source) {
		return std::nullopt;
	}

	const std::vector<OBSSceneItem> sourceItems = FindSourceSceneItems(main, source);
	for (const auto &config : CameraRoleScenes) {
		OBSSource sceneSource = FindCameraRoleScene(main, config);
		OBSScene scene = sceneSource ? obs_scene_from_source(sceneSource) : nullptr;
		if (!scene) {
			continue;
		}

		for (const OBSSceneItem &item : sourceItems) {
			if (obs_sceneitem_get_scene(item) == scene) {
				return config.role;
			}
		}
	}

	return std::nullopt;
}

bool AssignSourceToCameraRoleScene(OBSBasic &main, obs_source_t *source, CameraRole role)
{
	const CameraRoleSceneConfig *config = FindCameraRoleSceneConfig(role);
	if (!source || !config) {
		return false;
	}

	OBSSource targetSceneSource = GetOrCreateCameraRoleScene(main, *config);
	OBSScene targetScene = targetSceneSource ? obs_scene_from_source(targetSceneSource) : nullptr;
	if (!targetScene) {
		return false;
	}

	std::vector<OBSSceneItem> sourceItems = FindSourceSceneItems(main, source);
	OBSSceneItem targetItem;
	for (const OBSSceneItem &item : sourceItems) {
		if (obs_sceneitem_get_scene(item) == targetScene) {
			targetItem = item;
			break;
		}
	}

	if (!targetItem) {
		OBSScene currentScene = main.GetCurrentScene();
		OBSSceneItem preferredItem = FindPreferredSceneItem(currentScene, source, sourceItems);
		if (preferredItem) {
			const SceneItemState state = CaptureSceneItemState(preferredItem);
			targetItem = AddSourceToScene(targetScene, source, &state);
		} else {
			targetItem = AddSourceToScene(targetScene, source, nullptr);
		}
		if (!targetItem) {
			return false;
		}
	}

	for (const OBSSceneItem &item : sourceItems) {
		if (obs_sceneitem_get_scene(item) != targetScene) {
			obs_sceneitem_remove(item);
		}
	}
	main.SaveProject();
	return true;
}

} // namespace XBotGo
