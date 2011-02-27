/*
 *  Console.cpp - console utilities for Aleph One

 Copyright (C) 2005 and beyond by Gregory Smith
 and the "Aleph One" developers.
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 This license is contained in the file "COPYING",
 which is included with this source code; it is available online at
 http://www.gnu.org/licenses/gpl.html

*/

#include "cseries.h"
#include "Console.h"
#include "Logging.h"

#include <string>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "network.h"

// for carnage reporting:
#include "player.h"
#include "projectiles.h"
#include "shell.h"

// for saving
#include "FileHandler.h"
#include "game_wad.h"

#include <boost/algorithm/string/predicate.hpp>

using namespace std;

extern bool game_is_networked;

Console *Console::m_instance = NULL;

Console::Console() : m_active(false), m_carnage_messages_exist(false), m_use_lua_console(false)
{
	m_carnage_messages.resize(NUMBER_OF_PROJECTILE_TYPES);
	register_save_commands();
}

Console *Console::instance() {
	if (!m_instance) {
		m_instance = new Console;
	}
	return m_instance;
}

static inline void lowercase(string& s)
{
	transform(s.begin(), s.end(), s.begin(), ::tolower);
}

static pair<string, string> split(string buffer)
{
	string command;
	string remainder;

	string::size_type pos = buffer.find(' ');
	if (pos != string::npos && pos < buffer.size())
	{
		remainder = buffer.substr(pos + 1);
		command = buffer.substr(0, pos);
	}
	else
	{
		command = buffer;
	}

	return pair<string, string>(command, remainder);
}

void CommandParser::register_command(string command, boost::function<void(const string&)> f)
{
	lowercase(command);
	m_commands[command] = f;
}

void CommandParser::register_command(string command, const CommandParser& command_parser)
{
	lowercase(command);
	m_commands[command] = boost::bind(&CommandParser::parse_and_execute, command_parser, _1);
}

void CommandParser::unregister_command(string command)
{
	lowercase(command);
	m_commands.erase(command);
}

void CommandParser::parse_and_execute(const std::string& command_string)
{
	pair<string, string> cr = split(command_string);

	string command = cr.first;
	string remainder = cr.second;

	lowercase(command);
	
	command_map::iterator it = m_commands.find(command);
	if (it != m_commands.end())
	{
		it->second(remainder);
	}
}


void Console::enter() {
	// macros are processed first
	if (m_buffer[0] == '.')
	{
		pair<string, string> mr = split(m_buffer.substr(1));

		string input = mr.first;
		string output = mr.second;

		lowercase(input);

		std::map<string, string>::iterator it = m_macros.find(input);
		if (it != m_macros.end())
		{
			if (output != "")
				m_buffer = it->second + " " + output;
			else
				m_buffer = it->second;
		}
	}

	// commands are processed before callbacks
	if (m_buffer[0] == '.')
	{
		parse_and_execute(m_buffer.substr(1));
	} else if (!m_callback) {
		logAnomaly("console enter activated, but no callback set");
	} else {
		m_callback(m_buffer);
	}
	
	m_callback.clear();
	m_buffer.clear();
	m_displayBuffer.clear();
	m_active = false;
#if defined(SDL)
	SDL_EnableKeyRepeat(0, 0);
	SDL_EnableUNICODE(0);
#endif
}

void Console::abort() {
	m_buffer.clear();
	m_displayBuffer.clear();
	if (!m_callback) {
		logAnomaly("console abort activated, but no callback set");
	} else {
		m_callback(m_buffer);
	}

	m_callback.clear();
	m_active = false;
#if defined(SDL)
	SDL_EnableKeyRepeat(0, 0);
	SDL_EnableUNICODE(0);
#endif
}

void Console::backspace() {
	if (!m_buffer.empty()) {
		m_buffer.erase(m_buffer.size() - 1);
		m_displayBuffer.erase(m_displayBuffer.size() - 2);
		m_displayBuffer += "_";
	}
}

void Console::clear() {
	m_buffer.clear();
	m_displayBuffer = m_prompt + " _";
}

void Console::key(const char c) {
	m_buffer += c;
	m_displayBuffer.erase(m_displayBuffer.size() - 1);
	m_displayBuffer += c;
	m_displayBuffer += "_";
}

void Console::activate_input(boost::function<void (const std::string&)> callback,
			     const std::string& prompt)
{
	assert(!m_active);
	m_callback = callback;
	m_buffer.clear();
	m_displayBuffer = m_prompt = prompt;
	m_displayBuffer += " _";
	m_active = true;

#if defined(SDL)
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_EnableUNICODE(1);
#endif
}

void Console::deactivate_input() {
	m_buffer.clear();
	m_displayBuffer.clear();
	
	m_callback.clear();
	m_active = false;
#if defined(SDL)
	SDL_EnableKeyRepeat(0, 0);
	SDL_EnableUNICODE(0);
#endif
}

void Console::register_macro(string input, string output)
{
	lowercase(input);
	m_macros[input] = output;
}

void Console::unregister_macro(string input)
{
	lowercase(input);
	m_macros.erase(input);
}

void Console::set_carnage_message(int16 projectile_type, const std::string& on_kill, const std::string& on_suicide)
{
	m_carnage_messages_exist = true;
	m_carnage_messages[projectile_type] = std::pair<std::string, std::string>(on_kill, on_suicide);
}

static std::string replace_first(std::string &result, const std::string& from, const std::string& to)
{
	const int pos = result.find(from);
	if (pos != string::npos)
	{
		result.replace(pos, from.size(), to);
	}
	return result;
}

void Console::report_kill(int16 player_index, int16 aggressor_player_index, int16 projectile_index)
{
	if (!game_is_networked || !NetAllowCarnageMessages() || !m_carnage_messages_exist) return;

	// do some lookups
	projectile_data *projectile = get_projectile_data(projectile_index);

	const std::string player_key = "%player%";
	const std::string aggressor_key = "%aggressor%";
	if (projectile)
	{
		std::string display_string;
		std::string player_name = get_player_data(player_index)->name;
		if (player_index != aggressor_player_index)
		{
			display_string = m_carnage_messages[projectile->type].first;
			if (display_string == "") return;
			std::string aggressor_player_name = get_player_data(aggressor_player_index)->name;

			const int ppos = display_string.find(player_key);
			const int apos = display_string.find(aggressor_key);
			if (ppos == string::npos || apos == string::npos || ppos > apos)
			{
				replace_first(display_string, player_key, player_name);
				replace_first(display_string, aggressor_key, aggressor_player_name);
			}
			else
			{
				replace_first(display_string, aggressor_key, aggressor_player_name);
				replace_first(display_string, player_key, player_name);
			}
			
		}
		else
		{
			display_string = m_carnage_messages[projectile->type].second;
			if (display_string == "") return;
			replace_first(display_string, player_key, player_name);
		}

		screen_printf("%s", display_string.c_str());
	}
}

static std::string last_level;

struct save_level
{
	void operator() (const std::string& arg) const {
		if (!NetAllowSavingLevel())
		{
			screen_printf("Level saving disabled");
			return;
		}

		std::string filename = arg;
		if (filename == "")
		{
			if (last_level != "")
				filename = last_level;
			else
			{
				filename = mac_roman_to_utf8(static_world->level_name);
				if (!boost::algorithm::ends_with(filename, ".sceA"))
					filename += ".sceA";
			}
		}
		else
		{
			if (!boost::algorithm::ends_with(filename, ".sceA"))
				filename += ".sceA";	
		}

		last_level = filename;
		FileSpecifier fs;
		fs.SetToLocalDataDir();
		fs += filename;
		if (export_level(fs))
			screen_printf("Saved %s", utf8_to_mac_roman(fs.GetPath()).c_str());
		else
			screen_printf("An error occurred while saving the level");
	}
};

void Console::register_save_commands()
{
	CommandParser saveParser;
	saveParser.register_command("level", save_level());
	register_command("save", saveParser);
}
	
void Console::clear_saves()
{
	last_level.clear();
}

class XML_CarnageMessageParser : public XML_ElementParser
{
	int16 projectile_type;
	string on_kill;
	string on_suicide;
public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();

	XML_CarnageMessageParser() : XML_ElementParser("carnage_message") {}
	
};

bool XML_CarnageMessageParser::Start()
{
	on_kill.clear();
	on_suicide.clear();
	projectile_type = NONE;
	return true;
}

bool XML_CarnageMessageParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag, "on_kill"))
	{
		on_kill = Value;
		return true;
	}
	else if (StringsEqual(Tag, "on_suicide"))
	{
		on_suicide = Value;
		return true;
	}
	else if (StringsEqual(Tag, "projectile_type"))
	{
		return ReadBoundedInt16Value(Value, projectile_type, 0, NUMBER_OF_PROJECTILE_TYPES);
	}

	UnrecognizedTag();
	return false;
}

bool XML_CarnageMessageParser::AttributesDone()
{
	if (projectile_type == NONE)
	{
		AttribsMissing();
		return false;
	}
	
	Console::instance()->set_carnage_message(projectile_type, on_kill, on_suicide);
	return true;
}		

static XML_CarnageMessageParser CarnageMessageParser;

class XML_MacroParser : public XML_ElementParser
{
	string input;
	string output;
public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();

	XML_MacroParser() : XML_ElementParser("macro") {}
};

bool XML_MacroParser::Start()
{
	input.clear();
	output.clear();
	return true;
}

bool XML_MacroParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag, "input"))
	{
		input = Value;
		return true;
	}
	else if (StringsEqual(Tag, "output"))
	{
		output = Value;
		return true;
	}

	UnrecognizedTag();
	return false;
}

bool XML_MacroParser::AttributesDone()
{
	if (input == "")
	{
		AttribsMissing();
		return false;
	}

	Console::instance()->register_macro(input, output);
	return true;
}

static XML_MacroParser MacroParser;

class XML_ConsoleParser : public XML_ElementParser
{
public:
	bool HandleAttribute(const char *Tag, const char *Value);

	XML_ConsoleParser() : XML_ElementParser("console") {} 
};

bool XML_ConsoleParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag, "lua"))
	{
		bool use_lua_console;
		if (ReadBooleanValueAsBool(Value, use_lua_console))
		{
			Console::instance()->use_lua_console(use_lua_console);
			return true;
		}
		else
		{
			return false;
		}
	}
	
	UnrecognizedTag();
	return false;
}


static XML_ConsoleParser ConsoleParser;

XML_ElementParser *Console_GetParser()
{
	ConsoleParser.AddChild(&MacroParser);
	ConsoleParser.AddChild(&CarnageMessageParser);

	return &ConsoleParser;
}

