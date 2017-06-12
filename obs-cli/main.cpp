/******************************************************************************
	Copyright (C) 2016 by Jo�o Portela <email@joaoportela.net>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	******************************************************************************/

#include <iostream>
#include <string>
#include <memory>

#include <boost/program_options.hpp>

#include <obs.h>
#include <obs.hpp>
#include <util/platform.h>

#include "enum_types.hpp"
#include "monitor_info.hpp"
#include "setup_obs.hpp"
#include "event_loop.hpp"
#include "string_conversions.hpp"

#define do_log(level, format, ...) \
	blog(level, "[main] " format, ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG,   format, ##__VA_ARGS__)

namespace {
	namespace Ret {
		enum ENUM : int {
			success = 0,
			error_in_command_line = 1,
			error_unhandled_exception = 2,
			error_obs = 3
		};
	}

	struct CliOptions {
		// default values
		static const std::wstring default_encoder;
		static const int default_video_bitrate;
		static const int default_video_cqp;
		static const int default_fps;
		static const std::wstring default_rate_control;
		static const std::wstring default_preset;
		static const std::wstring default_profile;

		// cli options
		int monitor_to_record = 0;
		std::wstring encoder;
		std::wstring audio_device;
		std::wstring rate_control;
		std::wstring preset;
		std::wstring profile;
		int video_bitrate;
		int video_cqp;
		int fps;
		std::vector<std::wstring> outputs_paths;
		bool show_help = false;
		bool list_monitors = false;
		bool list_audios = false;
		bool list_encoders = false;
		bool list_inputs = false;
		bool list_outputs = false;
		std::wstring list_presets = L"";
		std::wstring list_profiles = L"";
	} cli_options;
	const std::wstring CliOptions::default_encoder = L"obs_x264";
	const int CliOptions::default_video_bitrate = 2500;
	const int CliOptions::default_video_cqp = 23;
	const int CliOptions::default_fps = 60;
	const std::wstring CliOptions::default_rate_control = L"CBR";
	const std::wstring CliOptions::default_preset = L"medium";
	const std::wstring CliOptions::default_profile = L"main";
} // namespace

/**
* Resets/Initializes video settings.
*
*   Calls obs_reset_video internally. Assumes some video options.
*/
void reset_video(int monitor_index, int fps)
{
	struct obs_video_info ovi;

	ovi.fps_num = fps;
	ovi.fps_den = 1;

	MonitorInfo monitor = monitor_at_index(monitor_index);
	ovi.graphics_module = "libobs-d3d11.dll"; // DL_D3D11
	ovi.base_width = monitor.width;
	ovi.base_height = monitor.height;
	ovi.output_width = monitor.width;
	ovi.output_height = monitor.height;
	ovi.output_format = VIDEO_FORMAT_NV12;
	ovi.colorspace = VIDEO_CS_601;
	ovi.range = VIDEO_RANGE_PARTIAL;
	ovi.adapter = 0;
	ovi.gpu_conversion = true;
	ovi.scale_type = OBS_SCALE_BICUBIC;

	int ret = obs_reset_video(&ovi);
	if (ret != OBS_VIDEO_SUCCESS) {
		std::cout << "reset_video failed!" << std::endl;
	}
}

/**
* Resets/Initializes video settings.
*
*   Calls obs_reset_audio internally. Assumes some audio options.
*/
void reset_audio()
{
	struct obs_audio_info ai;
	ai.samples_per_sec = 44100;
	ai.speakers = SPEAKERS_STEREO;

	bool success = obs_reset_audio(&ai);
	if (!success)
		std::cerr << "Audio reset failed!" << std::endl;
}

/**
* Stop recording of multiple outputs
*
*   Calls obs_output_stop(output) internally.
*/
void stop_recording(std::vector<OBSOutput> outputs)
{

	for (auto output : outputs) {
		obs_output_stop(output);
	}
}
/**
* Start recording to multiple outputs
*
*   Calls obs_output_start(output) internally.
*/
bool start_recording(std::vector<OBSOutput> outputs)
{
	int outputs_started = 0;
	for (auto output : outputs) {
		bool success = obs_output_start(output);
		outputs_started += success ? 1 : 0;
	}

	if (outputs_started == outputs.size()) {
		std::cout << "Recording started for all outputs!" << std::endl;
		return true;
	}
	else {
		size_t outputs_failed = outputs.size() - outputs_started;
		std::cerr << outputs_failed << "/" << outputs.size() << " file outputs are not recording." << std::endl;
		return false;
	}
}

bool should_print_lists()
{
	return cli_options.list_monitors
		|| cli_options.list_audios
		|| cli_options.list_encoders
		|| cli_options.list_inputs
		|| cli_options.list_outputs
		|| !cli_options.list_presets.empty()
		|| !cli_options.list_profiles.empty()
		;
}

bool do_print_lists()
{
	if (cli_options.list_monitors) {
		print_monitors_info();
	}
	if (cli_options.list_audios) {
		print_obs_enum_audio_types();
	}
	if (cli_options.list_encoders) {
		print_obs_enum_encoder_types();
	}
	if (cli_options.list_inputs) {
		print_obs_enum_input_types();
	}
	if (cli_options.list_outputs) {
		print_obs_enum_output_types();
	}
	if (!cli_options.list_presets.empty()) {
		print_obs_enum_presets(cli_options.list_presets);
	}
	if (!cli_options.list_profiles.empty()) {
		print_obs_enum_profiles(cli_options.list_profiles);
	}

	return should_print_lists();
}


int parse_args(int argc, char **argv)
{
	// print args to log
	std::ostringstream args;
	for (int i = 0; i < argc; i++)
	{
		std::string arg = std::string(argv[i]);
		if (arg.find(' ') != std::string::npos) {
			args << "\"" << arg << "\" ";
		}
		else {
			args << arg << " ";
		}
	}
	debug("%s", args.str().c_str());


	namespace po = boost::program_options;

	// note: required options will be checked manually.
	po::options_description all_options("obs-cli Allowed options");
	all_options.add_options()
		("help,h", po::bool_switch(&(cli_options.show_help)), "Show help message")
		("listmonitors", po::bool_switch(&(cli_options.list_monitors)), "List available monitors resolutions")
		("listinputs", po::bool_switch(&(cli_options.list_inputs)), "List available inputs")
		("listencoders", po::bool_switch(&(cli_options.list_encoders)), "List available encoders")
		("listoutputs", po::bool_switch(&(cli_options.list_outputs)), "List available outputs")
		("listaudios", po::bool_switch(&(cli_options.list_audios)), "List available audios")
		("listpresets", po::wvalue<std::wstring>(&(cli_options.list_presets))->default_value(L"", "")->value_name("encoder"), "List presets available for encoder")
		("listprofiles", po::wvalue<std::wstring>(&(cli_options.list_profiles))->default_value(L"", "")->value_name("encoder"), "List profiles available for encoder")

		("monitor,m", po::value<int>(&cli_options.monitor_to_record), "set monitor to be recorded")
		("output,o", po::wvalue<std::vector<std::wstring>>(&cli_options.outputs_paths), "set file destination, can be set multiple times for multiple outputs")
		("audio,a", po::wvalue<std::wstring>(&cli_options.audio_device)->default_value(L"", "")->implicit_value(L"default", ""), "set audio to be recorded (default to mic) -a\"device_name\" ")
		("encoder,e", po::wvalue<std::wstring>(&cli_options.encoder)->default_value(CliOptions::default_encoder, wstring_to_utf8(CliOptions::default_encoder)), "set encoder")
		("ratecontrol", po::wvalue<std::wstring>(&cli_options.rate_control)->default_value(CliOptions::default_rate_control, wstring_to_utf8(CliOptions::default_rate_control)), "set rate control.")
		("bitrate", po::value<int>(&cli_options.video_bitrate)->default_value(CliOptions::default_video_bitrate), "set video bitrate for rate controls that need it (CBR, VBR). suggested values for HD: 1200 for low, 2500 for medium, 5000 for high")
		("cqp", po::value<int>(&cli_options.video_cqp)->default_value(CliOptions::default_video_cqp), "set video cqp parameter for CQP rate control.")
		("fps", po::value<int>(&cli_options.fps)->default_value(CliOptions::default_fps), "set capture fps.")
		("preset", po::wvalue<std::wstring>(&cli_options.preset)->default_value(CliOptions::default_preset, wstring_to_utf8(CliOptions::default_preset)), "set encoder preset.")
		("profile", po::wvalue<std::wstring>(&cli_options.profile)->default_value(CliOptions::default_profile, wstring_to_utf8(CliOptions::default_profile)), "set encoder profile.")
		;

	try {

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, all_options), vm);

		po::notify(vm);

		if (cli_options.show_help) {
			std::cout << all_options << "\n";
			return Ret::success;
		}

		if (should_print_lists())
			// will be properly handled later.
			return Ret::success;

		// Check required options.
		// These are checked manually to not interfere with help and list* options.
		if (vm.count("monitor") == 0) {
			throw po::required_option("monitor");
		}
		if (vm.count("output") == 0) {
			throw po::required_option("output");
		}
	}
	catch (po::error& e) {
		std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
		std::cerr << all_options << std::endl;
		return Ret::error_in_command_line;
	}

	return Ret::success;
}

void start_output_callback(void * /*data*/, calldata_t *params)
{
	// auto loop = static_cast<EventLoop*>(data);
	auto output = static_cast<obs_output_t*>(calldata_ptr(params, "output"));
	blog(LOG_INFO, "Output '%s' started.", obs_output_get_name(output));
}

void stop_output_callback(void *data, calldata_t *params)
{
	auto loop = static_cast<EventLoop*>(data);
	auto output = static_cast<obs_output_t*>(calldata_ptr(params, "output"));
	long long code = calldata_int(params, "code");

	blog(LOG_INFO, "Output '%s' stopped with code %d.", obs_output_get_name(output), code);
	// as soon as *any* output is stopped, we have to ensure that the
	// program stops.
	loop->stop();
}

int main(int argc, char **argv)
{
	try {
		int ret = parse_args(argc, argv);
		if (ret != Ret::success)
			return ret;

		if (cli_options.show_help) {
			// We've already printed help. Exit.
			return Ret::success;
		}

		// manages object context so that we don't have to call
		// obs_startup and obs_shutdown.
		OBSContext ObsScope("en-US", nullptr, nullptr);

		if (!obs_initialized()) {
			std::cerr << "Obs initialization failed." << std::endl;
			return Ret::error_obs;
		}

		// must be called before reset_video
		if (!detect_monitors()){
			std::cerr << "No monitors found!" << std::endl;
			return Ret::success;
		}

		// resets must be called before loading modules.
		reset_video(cli_options.monitor_to_record, cli_options.fps);
		reset_audio();
		obs_load_all_modules();

		// can only be called after loading modules and detecting monitors.
		if (do_print_lists())
			return Ret::success;

		// declared in "main" scope, so that they are not disposed too soon.
		OBSSource video_source, audio_source;

		MonitorInfo monitor = monitor_at_index(cli_options.monitor_to_record);
		video_source = setup_video_input(monitor);
		if (!cli_options.audio_device.empty()) {
			audio_source = setup_audio_input(cli_options.audio_device);
			if (!audio_source){
				std::wcout << L"failed to find audio device " << cli_options.audio_device << L"." << std::endl;
			}
		}
		// Also declared in "main" scope. While the outputs are kept in scope, we will continue recording.
		Outputs output = setup_outputs(cli_options.encoder, cli_options.rate_control,
			cli_options.preset, cli_options.profile,
			cli_options.video_bitrate, cli_options.video_cqp,
			cli_options.outputs_paths);

		EventLoop loop;

		// connect signal events.
		OBSSignal output_start, output_stop;
		for (auto o : output.outputs) {
			output_start.Connect(obs_output_get_signal_handler(o), "start", start_output_callback, &loop);
			output_stop.Connect(obs_output_get_signal_handler(o), "stop", stop_output_callback, &loop);
		}

		bool success = start_recording(output.outputs);
		if (!success)
			return Ret::error_obs;

		loop.run();
		stop_recording(output.outputs);

		// Known issue: obs_qsv11 is not properly releasing some locks before shutdown.

		obs_set_output_source(0, nullptr);
		obs_set_output_source(1, nullptr);
		return Ret::success;
	}
	catch (std::exception& e) {
		std::cerr << "Unhandled Exception reached the top of main: "
			<< e.what() << ", application will now exit" << std::endl;
		return Ret::error_unhandled_exception;
	}
}
