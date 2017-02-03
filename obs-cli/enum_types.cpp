#include "enum_types.hpp"

#include<obs.hpp>
#include<iostream>

void print_obs_enum_input_types() {
	const char *type;
	bool foundValues = false;
	size_t idx = 0;

	std::cout << "Input types:" << std::endl;

	while (obs_enum_input_types(idx++, &type)) {
		const char *name = obs_source_get_display_name(type);

		std::cout << type << "\t" << name << std::endl;
		foundValues = true;
	}

	if (!foundValues) {
		std::cout << "Not Found!" << std::endl;
	}
	std::cout << "#############" << std::endl;
}

void print_obs_enum_encoder_types() {
	const char *type;
	bool foundValues = false;
	size_t idx = 0;

	std::cout << "Encoder types:" << std::endl;

	while (obs_enum_encoder_types(idx++, &type)) {
		const char *name = obs_encoder_get_display_name(type);
		const char *codec = obs_get_encoder_codec(type);

		std::cout << type << "\t" << name << "\t" << codec << std::endl;
		foundValues = true;
	}

	if (!foundValues) {
		std::cout << "Not Found!" << std::endl;
	}
	std::cout << "#############" << std::endl;
}

void print_obs_enum_output_types() {
	const char *type;
	bool foundValues = false;
	size_t idx = 0;

	std::cout << "Output types:" << std::endl;

	while (obs_enum_output_types(idx++, &type)) {
		const char *name = obs_output_get_display_name(type);

		std::cout << type << "\t" << name << std::endl;
		foundValues = true;
	}

	if (!foundValues) {
		std::cout << "Not Found!" << std::endl;
	}
	std::cout << "#############" << std::endl;
}
#ifdef __APPLE__
#define INPUT_AUDIO_SOURCE  "coreaudio_input_capture"
#define OUTPUT_AUDIO_SOURCE "coreaudio_output_capture"
#elif _WIN32
#define INPUT_AUDIO_SOURCE  "wasapi_input_capture"
#define OUTPUT_AUDIO_SOURCE "wasapi_output_capture"
#else
#define INPUT_AUDIO_SOURCE  "pulse_input_capture"
#define OUTPUT_AUDIO_SOURCE "pulse_output_capture"
#endif


void print_obs_enum_audio_type(obs_properties_t* props){
	obs_property_t *property = obs_properties_first(props);
	bool hasNoProperties = !property;
	while (property){
		const char        *name = obs_property_name(property);
		//only check device_id properties
		if (strcmp(name, "device_id") != 0)
			break;

		obs_property_type type = obs_property_get_type(property);
		//std::cout << "name:" << name << std::endl;

		obs_combo_type   ctype = obs_property_list_type(property);
		obs_combo_format cformat = obs_property_list_format(property);
		size_t           ccount = obs_property_list_item_count(property);
		int              cidx = -1;

		switch (type)
		{
		case OBS_PROPERTY_LIST:
			//const char       *name = obs_property_name(property);

			for (size_t cidx = 0; cidx < ccount; cidx++)
			{
				const char *nameListItem = obs_property_list_item_name(property, cidx);
				if (cformat == OBS_COMBO_FORMAT_STRING){
					std::cout << cidx << ": " << nameListItem << "  |  " << obs_property_list_item_string(property, cidx) << std::endl;
				}
			}

			break;
		default:
			break;
		}
		obs_property_next(&property);
	}
}

void print_obs_enum_audio_types(){

	std::cout << "Audio Input Captures:" << std::endl;

	obs_properties_t* props = obs_get_source_properties(INPUT_AUDIO_SOURCE);
	print_obs_enum_audio_type(props);

	std::cout << "Audio Output Captures:" << std::endl;
	props = obs_get_source_properties(OUTPUT_AUDIO_SOURCE);
	print_obs_enum_audio_type(props);

	std::cout << "#############" << std::endl;
}

void print_obs_enum_presets(const std::string & encoder) {
	std::cout << "Presets for '" << encoder << "':" << std::endl;

	obs_properties_t *props = obs_get_encoder_properties(encoder.c_str());

	obs_property_t *p = obs_properties_get(props, "preset");
	size_t num = obs_property_list_item_count(p);
	for (size_t i = 0; i < num; i++) {
		const char *name = obs_property_list_item_name(p, i);
		const char *val  = obs_property_list_item_string(p, i);

		std::cout << "\t" << val << ": " << name << std::endl;
	}

	obs_properties_destroy(props);
}

void print_obs_enum_profiles(const std::string & encoder) {
	std::cout << "Profiles for '" << encoder << "':" << std::endl;

	obs_properties_t *props = obs_get_encoder_properties(encoder.c_str());

	obs_property_t *p = obs_properties_get(props, "profile");
	size_t num = obs_property_list_item_count(p);
	for (size_t i = 0; i < num; i++) {
		const char *name = obs_property_list_item_name(p, i);
		const char *val  = obs_property_list_item_string(p, i);

		std::cout << "\t" << val << ": " << name << std::endl;
	}

	obs_properties_destroy(props);
}