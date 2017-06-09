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

/* Helpers to enumerate input/encoder/output ids/names. */

#pragma once

#include<string>

void print_obs_enum_input_types();

void print_obs_enum_encoder_types();

void print_obs_enum_output_types();

void print_obs_enum_audio_types();

void print_obs_enum_presets(const std::wstring& encoder);
void print_obs_enum_profiles(const std::wstring& encoder);
