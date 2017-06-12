#include "setup_obs.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include "string_conversions.hpp"

#define do_log(level, format, ...) \
	blog(level, "[setup_obs.cpp] " format, ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG,   format, ##__VA_ARGS__)

#ifdef __APPLE__
#define INPUT_AUDIO_SOURCE  L"coreaudio_input_capture"
#define OUTPUT_AUDIO_SOURCE L"coreaudio_output_capture"
#elif _WIN32
#define INPUT_AUDIO_SOURCE  L"wasapi_input_capture"
#define OUTPUT_AUDIO_SOURCE L"wasapi_output_capture"
#else
#define INPUT_AUDIO_SOURCE  L"pulse_input_capture"
#define OUTPUT_AUDIO_SOURCE L"pulse_output_capture"
#endif

#include <iostream>

namespace {
	int find_monitor_idx(MonitorInfo monitor)
	{
		// find matching monitor index for capture module that is used.
		// this is an hack and may break with different versions of the
		// "monitor_capture" module.
		struct gs_monitor_info info;

		obs_enter_graphics();

		int monitor_idx = -1;
		for (int idx = 0; true; idx++) {
			if (!gs_get_duplicator_monitor_info(idx, &info)){
				// failed to find monitor index.
				monitor_idx = -1;
				break;
			}
			if (info.cx == monitor.width
				&& info.cy == monitor.height
				&& info.x == monitor.left
				&& info.y == monitor.top) {
				// found monitor index
				monitor_idx = idx;
				break;
			}
		}

		obs_leave_graphics();

		info("monitor %dx%d @ %d,%d index %d changed to %d to match internal indexes.",
			info.cx, info.cy, info.x, info.y, monitor.monitor_id, monitor_idx);

		return monitor_idx;
	}
}

OBSSource setup_video_input(MonitorInfo monitor)
{
	int monitor_idx = find_monitor_idx(monitor);

	std::string name("monitor " + std::to_string(monitor_idx) + " capture");
	OBSSource source = obs_source_create("monitor_capture", name.c_str(), nullptr, nullptr);

	{
		obs_data_t * source_settings = obs_data_create();
		obs_data_set_int(source_settings, "monitor", monitor_idx);
		obs_data_set_bool(source_settings, "capture_cursor", false);

		obs_source_update(source, source_settings);
		obs_data_release(source_settings);
	}

	// set this source as output.
	obs_set_output_source(0, source);
	obs_source_release(source);
	return source;
}

std::wstring search_audio_device_by_type(std::wstring* audio_device, std::wstring* device_type)
{

	std::string device_id;
	obs_properties_t* props = obs_get_source_properties(wstring_to_utf8(*device_type).c_str());

	obs_property_t *property = obs_properties_first(props);

	while (property){
		const char        *name = obs_property_name(property);
		//only check device_id properties
		if (strcmp(name, "device_id") != 0)
			break;

		obs_property_type type = obs_property_get_type(property);
		obs_combo_format cformat = obs_property_list_format(property);
		size_t           ccount = obs_property_list_item_count(property);

		switch (type)
		{
		case OBS_PROPERTY_LIST:

			for (size_t cidx = 0; cidx < ccount; cidx++){
				const char *nameListItem = obs_property_list_item_name(property, cidx);

				if (cformat == OBS_COMBO_FORMAT_STRING){
					if (boost::iequals(nameListItem, audio_device->c_str())) {
						device_id = obs_property_list_item_string(property, cidx);
						return utf8_to_wstring(device_id);
					}
				}
			}

			break;
		default:
			break;
		}
		obs_property_next(&property);
	}
	return utf8_to_wstring(device_id);
}

void search_audio_device_by_name(std::wstring audio_device, std::wstring* device_id, std::wstring* device_type)
{
	//std::string device_id;
	std::wstring st = std::wstring(INPUT_AUDIO_SOURCE);
	*device_type = st;
	*device_id = search_audio_device_by_type(&audio_device, device_type);

	//device has been found exit
	if (!device_id->empty())
		return;

	*device_type = std::wstring(OUTPUT_AUDIO_SOURCE);
	*device_id = search_audio_device_by_type(&audio_device, device_type);
	return;
}

OBSSource setup_audio_input(std::wstring audio_device)
{
	std::wstring device_id;
	std::wstring device_type;

	search_audio_device_by_name(audio_device, &device_id, &device_type);
	if (device_id.empty())
		return nullptr;

	OBSSource source = obs_source_create(wstring_to_utf8(device_type).c_str(), "audio capture", nullptr, nullptr);
	{
		obs_data_t * source_settings = obs_data_create();

		obs_data_set_string(source_settings, "device_id", wstring_to_utf8(device_id).c_str());

		obs_source_update(source, source_settings);
		obs_data_release(source_settings);
	}

	// set this source as output.
	obs_set_output_source(1, source);

	obs_source_release(source);
	return source;
}

Outputs setup_outputs(std::wstring video_encoder_id,
	std::wstring rate_control,
	std::wstring preset,
	std::wstring profile,
	int video_bitrate,
	int video_cqp,
	std::vector<std::wstring> output_paths)
{
	OBSEncoder video_encoder = obs_video_encoder_create(wstring_to_utf8(video_encoder_id).c_str(), "video_encoder", nullptr, nullptr);
	obs_encoder_release(video_encoder);
	obs_encoder_set_video(video_encoder, obs_get_video());
	{
		obs_data_t * encoder_settings = obs_data_create();
		obs_data_set_string(encoder_settings, "rate_control", wstring_to_utf8(rate_control).c_str());
		obs_data_set_string(encoder_settings, "preset", wstring_to_utf8(preset).c_str());
		obs_data_set_string(encoder_settings, "profile", wstring_to_utf8(profile).c_str());
		obs_data_set_int(encoder_settings, "bitrate", video_bitrate);
		obs_data_set_int(encoder_settings, "cqp", video_cqp);

		obs_encoder_update(video_encoder, encoder_settings);
		obs_data_release(encoder_settings);
	}

	OBSEncoder audio_encoder = obs_audio_encoder_create("mf_aac", "audio_encoder", nullptr, 0, nullptr);
	obs_encoder_release(audio_encoder);
	obs_encoder_set_audio(audio_encoder, obs_get_audio());
	{
		obs_data_t * encoder_settings = obs_data_create();
		obs_data_set_int(encoder_settings, "samplerate", 44100);
		obs_data_set_int(encoder_settings, "bitrate", 160);
		obs_data_set_default_bool(encoder_settings, "allow he-aac", true);

		obs_encoder_update(audio_encoder, encoder_settings);
		obs_data_release(encoder_settings);
	}


	std::vector<OBSOutput> outputs;
	for (int i = 0; i < output_paths.size(); i++) {
		auto output_path = output_paths[i];
		std::wstring name(L"file_output_" + std::to_wstring(i));
		OBSOutput file_output = obs_output_create("ffmpeg_muxer", wstring_to_utf8(name).c_str(), nullptr, nullptr);
		obs_output_release(file_output);
		{
			obs_data_t * output_settings = obs_data_create();
			obs_data_set_string(output_settings, "path", wstring_to_utf8(output_path).c_str());
			obs_data_set_string(output_settings, "muxer_settings", NULL);

			obs_output_update(file_output, output_settings);
			obs_data_release(output_settings);
		}

		obs_output_set_video_encoder(file_output, video_encoder);
		obs_output_set_audio_encoder(file_output, audio_encoder, 0);

		outputs.push_back(file_output);
	}

	Outputs out;
	out.video_encoder = video_encoder;
	out.audio_encoder = audio_encoder;
	out.outputs = outputs;
	return out;
}
