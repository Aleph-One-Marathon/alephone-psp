/*
SOUND.C

	Copyright (C) 1991-2001 and beyond by Bungie Studios, Inc.
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

#include "SoundManager.h"
#include "ReplacementSounds.h"
#include "sound_definitions.h"
#include "Mixer.h"

#define SLOT_IS_USED(o) ((o)->flags&(uint16)0x8000)
#define SLOT_IS_FREE(o) (!SLOT_IS_USED(o))
#define MARK_SLOT_AS_FREE(o) ((o)->flags&=(uint16)~0x8000)
#define MARK_SLOT_AS_USED(o) ((o)->flags|=(uint16)0x8000)

static int16 real_number_of_sound_definitions;

SoundManager *SoundManager::m_instance = 0;

static void Shutdown()
{
	SoundManager::instance()->Shutdown();
}

// From FileSpecifier_SDL.cpp
extern void get_default_sounds_spec(FileSpecifier &file);

void SoundManager::Initialize(const Parameters& new_parameters)
{
	loaded_sounds_size = 0;
	total_channel_count = 0;

	FileSpecifier InitialSoundFile;
	get_default_sounds_spec(InitialSoundFile);
	if (OpenSoundFile(InitialSoundFile))
	{
		atexit(::Shutdown);

		parameters.flags = 0;
		initialized = true;
		active = false;
		SetParameters(new_parameters);
		SetStatus(true);
	}
}

void SoundManager::SetParameters(const Parameters& parameters)
{
	if (initialized)
	{
		bool initial_state = active;

		// If it was initially on, turn off the sound manager
		if (initial_state)
			SetStatus(false);

		// We need to get rid of the sounds we have in memory
		UnloadAllSounds();

		// Stuff in our new parameters
		this->parameters = parameters;
		this->parameters.Verify();

		// If it was initially on, turn the sound manager back on
		if (initial_state && parameters.volume)
			SetStatus(true);		
	}

}

void SoundManager::Shutdown()
{
	instance()->SetStatus(false);
	instance()->CloseSoundFile();
}

bool SoundManager::OpenSoundFile(FileSpecifier& File)
{
	StopAllSounds();
	if (!sound_file.Open(File)) return false;
	real_number_of_sound_definitions = number_of_sound_definitions = sound_file.sound_count;	

	sound_source = (parameters.flags & _16bit_sound_flag) ? _16bit_22k_source : _8bit_22k_source;
	if (sound_file.source_count == 1)
		sound_source = _8bit_22k_source;

	return true;
}

void SoundManager::CloseSoundFile()
{
	StopAllSounds();
	sound_file.Close();
}

bool SoundManager::AdjustVolumeUp(short sound_index)
{
	if (active && parameters.volume < NUMBER_OF_SOUND_VOLUME_LEVELS)
	{
		parameters.volume++;
		Mixer::instance()->SetVolume(parameters.volume * SOUND_VOLUME_DELTA);
		PlaySound(sound_index, 0, NONE);
		return true;
	}
	return false;
}

bool SoundManager::AdjustVolumeDown(short sound_index)
{
	if (active && parameters.volume > 0)
	{
		parameters.volume--;
		Mixer::instance()->SetVolume(parameters.volume * SOUND_VOLUME_DELTA);
		PlaySound(sound_index, 0, NONE);
		return true;
	}
	return false;
}

void SoundManager::TestVolume(short volume, short sound_index)
{
	if (active)
	{
		if ((volume = PIN(volume, 0, NUMBER_OF_SOUND_VOLUME_LEVELS)) != 0)
		{
			PlaySound(sound_index, 0, NONE);
			Mixer::instance()->SetVolume(volume * SOUND_VOLUME_DELTA);
			while (SoundIsPlaying(sound_index))
				SDL_Delay(10);
			Mixer::instance()->SetVolume(parameters.volume * SOUND_VOLUME_DELTA);
		}
	}
}

int SoundManager::NewCustomSoundDefinition() {
  int ret = sound_file.NewCustomSoundDefinition();
  if(ret >= 0)
    ++number_of_sound_definitions;
  return ret;
}

bool SoundManager::AddCustomSoundSlot(int index, const char* file) {
  return sound_file.AddCustomSoundSlot(index, file);
}

void SoundManager::UnloadCustomSounds() {
  sound_file.UnloadCustomSounds();
  number_of_sound_definitions = real_number_of_sound_definitions;
}

bool SoundManager::LoadSound(short sound_index)
{
	if (active)
	{
		SoundDefinition *definition = GetSoundDefinition(sound_index);
		if (!definition) return false;

		// Load all the external-file sounds for each index; fill the slots appropriately.
		int NumSlots= (parameters.flags & _more_sounds_flag) ? definition->permutations : 1;
		for (int k = 0; k < NumSlots; k++)
		{
			SoundOptions *SndOpts = SoundReplacements::instance()->GetSoundOptions(sound_index, k);
			if (!SndOpts) continue;
			FileSpecifier File;
			if (!File.SetNameWithPath(SndOpts->File.c_str())) continue;
			if (!SndOpts->Sound.LoadExternal(File)) continue;
		}

		if (definition->sound_code != NONE &&
		    (parameters.flags & _ambient_sound_flag) || !(definition->flags & _sound_is_ambient))
		{
			if (!definition->LoadedSize())
			{
				definition->Load(*(sound_file.opened_sound_file), parameters.flags & _more_sounds_flag);
				loaded_sounds_size += definition->LoadedSize();
				definition->last_played = machine_tick_count();
				while (loaded_sounds_size > total_buffer_size)
					ReleaseLeastUsefulSound();
			}
			if (definition->LoadedSize())
			{
				definition->permutations_played = 0;
			}
		}
		
		return definition->LoadedSize() ? true : false;
	}	

	return false;
}

void SoundManager::LoadSounds(short *sounds, short count)
{
	for (short i = 0; i < count; i++)
	{
		LoadSound(sounds[i]);
	}
}

void SoundManager::OrphanSound(short identifier)
{
	if (active && total_channel_count > 0)
	{
		for (short i = 0; i < parameters.channel_count; i++)
		{
			Channel *channel = &channels[i];
			if (channel->identifier == identifier || identifier == NONE)
			{
				channel->dynamic_source = 0;
				channel->identifier = NONE;
			}
		}
	}
}

void SoundManager::UnloadAllSounds()
{
	if (active)
	{
		StopSound(NONE, NONE);

		while (ReleaseLeastUsefulSound() != NONE)
			;
	}
}

void SoundManager::PlaySound(short sound_index, 
			     world_location3d *source,
			     short identifier, // NONE is no identifer and the sound is immediately orphaned
			     _fixed pitch) // on top of all existing pitch modifiers
{
	/* don�t do anything if we�re not initialized or active, or our sound_code is NONE,
		or our volume is zero, our we have no sound channels */
	if (sound_index!=NONE && active && sound_index < number_of_sound_definitions && parameters.volume > 0 && total_channel_count > 0)
	{
		Channel::Variables variables;

		CalculateInitialSoundVariables(sound_index, source, variables, pitch);
		
		/* make sure the sound data is in memory */
		if (LoadSound(sound_index))
		{
			Channel *channel = BestChannel(sound_index, variables);;
			/* get the channel, and free it for our new sound */
			if (channel)
			{
				/* set the volume and pitch in this channel */
				InstantiateSoundVariables(variables, *channel, true);
				
				/* initialize the channel */
				channel->flags= 0;
				channel->callback_count= 0; // #MD
				channel->start_tick= machine_tick_count();
				channel->sound_index= sound_index;
				channel->identifier= identifier;
				channel->dynamic_source= (identifier==NONE) ? (world_location3d *) NULL : source;
				MARK_SLOT_AS_USED(channel);
				
				/* start the sound playing */
				BufferSound(*channel, sound_index, pitch);

				/* if we have a valid source, copy it, otherwise remember that we don�t */
				if (source)
				{
					channel->source= *source;
				}
				else
				{
					channel->flags|= _sound_is_local;
				}
			}
		}
	}
}
				
void SoundManager::DirectPlaySound(short sound_index, angle direction, short volume, _fixed pitch)
{
	/* don�t do anything if we�re not initialized or active, or our sound_code is NONE,
	   or our volume is zero, our we have no sound channels */

	if (sound_index != NONE && active && sound_index < number_of_sound_definitions && parameters.volume > 0 && total_channel_count > 0)
	{
		if (LoadSound(sound_index))
		{
			Channel::Variables variables;
			Channel *channel = BestChannel(sound_index, variables);
			if (channel)
			{
				world_location3d *listener = _sound_listener_proc();

				variables.priority = 0;
				variables.volume = volume;

				if (direction = NONE || !listener)
				{
					variables.left_volume = variables.right_volume = volume;
				}
				else
				{
					AngleAndVolumeToStereoVolume(direction - listener->yaw, volume, &variables.right_volume, &variables.left_volume);
				}

				InstantiateSoundVariables(variables, *channel, true);
				/* initialize the channel */
				channel->flags = _sound_is_local; // but possibly being played in stereo
				channel->callback_count = 0;
				channel->start_tick = machine_tick_count();
				channel->sound_index = sound_index;
				channel->dynamic_source = 0;
				MARK_SLOT_AS_USED(channel);

				/* start the sound playing */
				BufferSound(*channel, sound_index, pitch);
			}
		}
	}
}

bool SoundManager::SoundIsPlaying(short sound_index)
{
	bool sound_playing = false;

	if (active && total_channel_count > 0)
	{
		for (short i = 0; i < total_channel_count; i++)
		{
			if (SLOT_IS_USED(&channels[i]) && channels[i].sound_index == sound_index)
			{
				sound_playing = true;
			}
		}

		UnlockLockedSounds();
	}

	return sound_playing;
}

void SoundManager::StopSound(short identifier, short sound_index)
{
	if (active && total_channel_count > 0)
	{
		// if we're stopping everything...
		if (identifier == NONE && sound_index == NONE)
		{
			// can't fade to silence here

			// stop the ambient sound channels, too
			if (parameters.flags & _ambient_sound_flag)
			{
				for (short i = 0; i < MAXIMUM_AMBIENT_SOUND_CHANNELS; i++)
				{
					FreeChannel(channels[total_channel_count - i - 1]);
				}
			}
		}

		for (short i = 0; i < total_channel_count; i++)
		{
			if (SLOT_IS_USED(&channels[i]) && (channels[i].identifier == identifier || identifier == NONE) && (channels[i].sound_index == sound_index || sound_index == NONE))
			{
				FreeChannel(channels[i]);
			}
		}
	}

	return;
}

int SoundManager::NumberOfSoundDefinitions()
{
	return number_of_sound_definitions;
}

void SoundManager::Idle()
{
	if (active && total_channel_count > 0)
	{
		UnlockLockedSounds();
		TrackStereoSounds();
		CauseAmbientSoundSourceUpdate();
	}
}

void SoundManager::CauseAmbientSoundSourceUpdate()
{
	if (active && parameters.volume > 0 && total_channel_count > 0)
	{
		if (parameters.flags & _ambient_sound_flag)
		{
			UpdateAmbientSoundSources();
		}
	}
}

struct ambient_sound_data
{
	uint16 flags;
	short sound_index;

	SoundManager::Channel::Variables variables;
	
	struct channel_data *channel;
};

static sound_behavior_definition *get_sound_behavior_definition(
	const short sound_behavior_index)
{
	return GetMemberWithBounds(sound_behavior_definitions,sound_behavior_index,NUMBER_OF_SOUND_BEHAVIOR_DEFINITIONS);
}

static short distance_to_volume(
	SoundDefinition *definition,
	world_distance distance,
	uint16 flags)
{
	struct sound_behavior_definition *behavior= get_sound_behavior_definition(definition->behavior_index);
	// LP change: idiot-proofing
	if (!behavior) return 0; // Silence
	
	struct depth_curve_definition *depth_curve;
	short volume;

	if (((flags&_sound_was_obstructed) && !(definition->flags&_sound_cannot_be_obstructed)) ||
		((flags&_sound_was_media_obstructed) && !(definition->flags&_sound_cannot_be_media_obstructed)))
	{
		depth_curve= &behavior->obstructed_curve;
	}
	else
	{
		depth_curve= &behavior->unobstructed_curve;
	}
	
	if (distance<=depth_curve->maximum_volume_distance)
	{
		volume= depth_curve->maximum_volume;
	}
	else
	{
		if (distance>depth_curve->minimum_volume_distance)
		{
			volume= depth_curve->minimum_volume;
		}
		else
		{
			volume= depth_curve->minimum_volume - ((depth_curve->minimum_volume-depth_curve->maximum_volume)*(depth_curve->minimum_volume_distance-distance)) /
				(depth_curve->minimum_volume_distance-depth_curve->maximum_volume_distance);
		}
	}
	
	if ((flags&_sound_was_media_muffled) && !(definition->flags&_sound_cannot_be_media_obstructed))
	{
		volume>>= 1;
	}
	
	return volume;
}

static ambient_sound_definition *get_ambient_sound_definition(
	const short ambient_sound_index)
{
	return GetMemberWithBounds(ambient_sound_definitions,ambient_sound_index,NUMBER_OF_AMBIENT_SOUND_DEFINITIONS);
}

void SoundManager::AddOneAmbientSoundSource(ambient_sound_data *ambient_sounds, world_location3d *source, world_location3d *listener, short ambient_sound_index, short absolute_volume)
{
	if (ambient_sound_index!=NONE)
	{
		// LP change; make NONE in case this sound definition is invalid
		struct ambient_sound_definition *SoundDef = get_ambient_sound_definition(ambient_sound_index);
		short sound_index = (SoundDef) ? SoundDef->sound_index : NONE;
		
		if (sound_index!=NONE)
		{
			SoundDefinition *definition = SoundManager::instance()->GetSoundDefinition(sound_index);
			
			// LP change: idiot-proofing
			if (definition)
			{
				if (definition->sound_code!=NONE)
				{
					struct sound_behavior_definition *behavior= get_sound_behavior_definition(definition->behavior_index);
					// LP change: idiot-proofing
					if (!behavior) return; // Silence
					
					struct ambient_sound_data *ambient;
					short distance = 0;
					short i;
					
					if (source)
					{
						distance= distance3d(&listener->point, &source->point);
					}
					
					for (i= 0, ambient= ambient_sounds;
					     i<MAXIMUM_PROCESSED_AMBIENT_SOUNDS;
					     ++i, ++ambient)
					{
						if (SLOT_IS_USED(ambient))
						{
							if (ambient->sound_index==sound_index) break;
						}
						else
						{
							MARK_SLOT_AS_USED(ambient);
							
							ambient->sound_index= sound_index;
							
							ambient->variables.priority= definition->behavior_index;
							ambient->variables.volume= ambient->variables.left_volume= ambient->variables.right_volume= 0;
							
							break;
						}
					}
					
					if (i!=MAXIMUM_PROCESSED_AMBIENT_SOUNDS)
					{
						if (!source || distance<behavior->unobstructed_curve.minimum_volume_distance)
						{
							short volume, left_volume, right_volume;
							
							if (source)
							{
								// LP change: made this long-distance friendly
								long dx= long(listener->point.x) - long(source->point.x);
								long dy= long(listener->point.y) - long(source->point.y);
								
								volume= distance_to_volume(definition, distance, _sound_obstructed_proc(source));
								volume= (absolute_volume*volume)>>MAXIMUM_SOUND_VOLUME_BITS;
								
								if (dx || dy)
								{
									AngleAndVolumeToStereoVolume(arctangent(dx, dy) - listener->yaw, volume, &right_volume, &left_volume);
								}
								else
								{
									left_volume= right_volume= volume;
								}
							}
							else
							{
								volume= left_volume= right_volume= absolute_volume;
							}
							
							{
								short maximum_volume= MAX(MAXIMUM_AMBIENT_SOUND_VOLUME, volume);
								short maximum_left_volume= MAX(MAXIMUM_AMBIENT_SOUND_VOLUME, left_volume);
								short maximum_right_volume= MAX(MAXIMUM_AMBIENT_SOUND_VOLUME, right_volume);
								
								ambient->variables.volume= CEILING(ambient->variables.volume+volume, maximum_volume);
								ambient->variables.left_volume= CEILING(ambient->variables.left_volume+left_volume, maximum_left_volume);
								ambient->variables.right_volume= CEILING(ambient->variables.right_volume+right_volume, maximum_right_volume);
							}
						}
					}
					else
					{
//					dprintf("warning: ambient sound buffer full;g;");
					}
				}
			}
		}
	}
}

struct random_sound_definition *get_random_sound_definition(
	const short random_sound_index)
{
	return GetMemberWithBounds(random_sound_definitions,random_sound_index,NUMBER_OF_RANDOM_SOUND_DEFINITIONS);
}

short SoundManager::RandomSoundIndexToSoundIndex(short random_sound_index)
{
	random_sound_definition *definition = get_random_sound_definition(random_sound_index);

	if (definition) 
		return definition->sound_index;
	else
		return NONE;
}

SoundManager::Parameters::Parameters() :
	channel_count(MAXIMUM_SOUND_CHANNELS),
	volume(DEFAULT_SOUND_LEVEL),
	flags(_more_sounds_flag | _stereo_flag | _dynamic_tracking_flag | _ambient_sound_flag | _16bit_sound_flag),
	rate(DEFAULT_RATE),
	samples(DEFAULT_SAMPLES),
	music(DEFAULT_MUSIC_LEVEL),
	volume_while_speaking(DEFAULT_VOLUME_WHILE_SPEAKING),
	mute_while_transmitting(true)
{
}

bool SoundManager::Parameters::Verify()
{
	channel_count = PIN(channel_count, 0, MAXIMUM_SOUND_CHANNELS);
	volume = PIN(volume, 0, NUMBER_OF_SOUND_VOLUME_LEVELS);
	
	return true;
}

SoundManager::SoundManager() : active(false), initialized(false) 
{ 
	channels.resize(MAXIMUM_SOUND_CHANNELS + MAXIMUM_AMBIENT_SOUND_CHANNELS);
};

void SoundManager::SetStatus(bool active)
{
	if (initialized)
	{
		if (active != this->active)
		{
			if (active) 
			{
				total_channel_count = parameters.channel_count;
				if (parameters.flags & _ambient_sound_flag)
					total_channel_count += MAXIMUM_AMBIENT_SOUND_CHANNELS;
				int32 samples = parameters.samples;
				if (parameters.flags & _more_sounds_flag)
					total_buffer_size = MORE_SOUND_BUFFER_SIZE;
				else
					total_buffer_size = MINIMUM_SOUND_BUFFER_SIZE;
				if (parameters.flags & _ambient_sound_flag)
					total_buffer_size += AMBIENT_SOUND_BUFFER_SIZE;
				if (parameters.flags & _16bit_sound_flag)
				{
					total_buffer_size *= 2;
					samples *= 2;
				}
				if (parameters.flags & _extra_memory_flag)
					total_buffer_size *= 2;
				if (parameters.flags & _stereo_flag)
					samples *= 2;
				sound_source = (parameters.flags & _16bit_sound_flag) ? _16bit_22k_source : _8bit_22k_source;
				for (int i = 0; i < channels.size(); i++)
				{
					channels[i].mixer_channel = i;
				}

				samples = samples * parameters.rate / Parameters::DEFAULT_RATE;

				Mixer::instance()->Start(parameters.rate, parameters.flags & _16bit_sound_flag, parameters.flags & _stereo_flag, MAXIMUM_SOUND_CHANNELS + MAXIMUM_AMBIENT_SOUND_CHANNELS, parameters.volume * SOUND_VOLUME_DELTA, samples);

				if (Mixer::instance()->SoundChannelCount() == 0)
				{
					total_channel_count = 0;
					active = this->active = initialized = false;
				}
				else
				{
					total_channel_count = Mixer::instance()->SoundChannelCount();
				}
			}
			else
			{
				StopAllSounds();
				Mixer::instance()->Stop();
				total_channel_count = 0;
			}
			this->active = active;
		}
	}
}

SoundDefinition* SoundManager::GetSoundDefinition(short sound_index)
{
	SoundDefinition* sound_definition = sound_file.GetSoundDefinition(sound_source, sound_index);
	if (sound_source == _16bit_22k_source && sound_definition && sound_definition->permutations == 0)
	{
		sound_definition = sound_file.GetSoundDefinition(_8bit_22k_source, sound_index);
	}

	return sound_definition;
}

void SoundManager::BufferSound(Channel &channel, short sound_index, _fixed pitch, bool ext_play_immed)
{
	SoundDefinition *definition = GetSoundDefinition(sound_index);
	if (!definition || !definition->permutations)
		return;
	assert(definition->LoadedSize());

	int permutation = GetRandomSoundPermutation(sound_index);

	assert(permutation >= 0 && permutation < definition->permutations);

	Mixer::Header header;

	SoundOptions *SndOpts = SoundReplacements::instance()->GetSoundOptions(sound_index, permutation);
	if (SndOpts && SndOpts->Sound.Length())
	{
		header = SndOpts->Sound;
	}
	else
	{
		header = definition->sounds[permutation];
	}
	
	Mixer::instance()->BufferSound(channel.mixer_channel, header, CalculatePitchModifier(sound_index, pitch));
}

SoundManager::Channel *SoundManager::BestChannel(short sound_index, Channel::Variables &variables)
{
	Channel *best_channel;

	SoundDefinition *definition = GetSoundDefinition(sound_index);
	if (!definition) return 0;

	best_channel = 0;
	if (!definition->chance || (local_random() > definition->chance))
	{
		for (short i = 0; i < parameters.channel_count; i++)
		{
			Channel *channel = &channels[i];
			if (SLOT_IS_USED(channel) && Mixer::instance()->ChannelBusy(channel->mixer_channel))
			{
				/* if this channel is at a lower volume than the sound we are trying to play and is at the same
				   priority, or the channel is at a lower priority, then we can abort it */
				if ((channel->variables.volume <= variables.volume + ABORT_AMPLITUDE_THRESHHOLD && channel->variables.priority == variables.priority) ||
				    channel->variables.priority < variables.priority)
				{
					// if this channel is already playing our sound, 
					// this is our channel (for better or for worse)
					if (channel->sound_index == sound_index)
					{
						if (definition->flags & _sound_cannot_be_restarted)
						{
							best_channel = 0;
							break;
						}
						
						if (!(definition->flags & _sound_does_not_self_abort))
						{
							if ((parameters.flags & _zero_restart_delay) || channel->start_tick + MINIMUM_RESTART_TICKS < machine_tick_count())
								best_channel = channel;
							else
								best_channel = 0;
							break;
						}
					}
					
					/* if we haven�t found an alternative channel or this channel is at a lower
					   volume than our previously best channel (which isn�t an unused channel),
					   then we�ve found a new best channel */
					if (!best_channel ||
					    (SLOT_IS_USED(best_channel) && best_channel->variables.volume > channel->variables.volume) ||
					    (SLOT_IS_USED(best_channel) && best_channel->variables.priority < channel->variables.priority))
					{
						best_channel = channel;
					}
				}
				else
				{
					if (channel->sound_index == sound_index && !(definition->flags & _sound_does_not_self_abort))
					{
						/* if we�re already playing this sound at a higher volume, don�t abort it */
						best_channel = 0;
						break;
					}
				}
				
			}
			else
			{
				/* unused channel (we won�t get much better than this!) */
				if (SLOT_IS_USED(channel))
					FreeChannel(*channel);
				best_channel = channel;
			}
		}
	}

	if (best_channel)
	{
		
		/* stop whatever sound is playing and unlock the old handle if necessary */
		FreeChannel(*best_channel);
	}

	return best_channel;
}

void SoundManager::FreeChannel(Channel &channel)
{
	if (SLOT_IS_USED(&channel))
	{
		short sound_index = channel.sound_index;
		Mixer::instance()->QuietChannel(channel.mixer_channel);

		assert(sound_index != NONE);
		channel.sound_index = NONE;
		MARK_SLOT_AS_FREE(&channel);

		// if anybody else is playing this sound_index, we can't unlock the handle
		if (!SoundIsPlaying(sound_index)) UnlockSound(sound_index);
	}
}

void SoundManager::UnlockLockedSounds()
{
	if (active && total_channel_count > 0)
	{
		for (short i = 0; i < parameters.channel_count; i++)
		{
			Channel *channel = &channels[i];
			if (SLOT_IS_USED(channel) && !Mixer::instance()->ChannelBusy(channel->mixer_channel))
			{
				FreeChannel(*channel);
			}
		}
	}
}

short SoundManager::ReleaseLeastUsefulSound()
{
	short sound_index, least_used_sound_index = NONE;
	SoundDefinition *least_used_definition = 0;

	for (sound_index = 0; sound_index < number_of_sound_definitions; ++sound_index)
	{
		SoundDefinition *definition = GetSoundDefinition(sound_index);

		if (definition->LoadedSize() && (!least_used_definition || least_used_definition->last_played > definition->last_played))
		{
			least_used_sound_index = sound_index;
			least_used_definition = definition;
		}
	}

	if (least_used_sound_index != NONE)
	{
		StopSound(NONE, least_used_sound_index);
		DisposeSound(least_used_sound_index);
	}

	return least_used_sound_index;
}

void SoundManager::DisposeSound(short sound_index)
{
	SoundDefinition *definition = GetSoundDefinition(sound_index);

	// unload replacement sounds
	int NumSlots = (parameters.flags & _more_sounds_flag) ? definition->permutations : 1;
	for (int k = 0; k < NumSlots; k++)
	{
		SoundOptions *SndOpts = SoundReplacements::instance()->GetSoundOptions(sound_index, k);
		if (SndOpts) SndOpts->Sound.Clear();
	}

	if (!definition) return;
	loaded_sounds_size -= definition->LoadedSize();
	definition->Unload();

}

void SoundManager::CalculateSoundVariables(short sound_index, world_location3d *source, Channel::Variables& variables)
{
	SoundDefinition *definition = GetSoundDefinition(sound_index);
	if (!definition) return;

	world_location3d *listener = _sound_listener_proc();

	if (source && listener)
	{
		world_distance distance = distance3d(&source->point, &listener->point);
		
		// LP change: made this long-distance friendly
		long dx = long(listener->point.x) - long(source->point.x);
		long dy = long(listener->point.y) - long(source->point.y);

		// for now, a sound's priority is its behavior index
		variables.priority = definition->behavior_index;

		// calculate the relative volume due to the given depth curve
		variables.volume = distance_to_volume(definition, distance, _sound_obstructed_proc(source));

		if (dx || dy)
		{

			// set volume, left_volume, right_volume
			AngleAndVolumeToStereoVolume(arctangent(dx, dy) - listener->yaw, variables.volume, &variables.right_volume, &variables.left_volume);
		}
		else
		{
			variables.left_volume = variables.right_volume = variables.volume;
		}
	}

}

void SoundManager::CalculateInitialSoundVariables(short sound_index, world_location3d *source, Channel::Variables& variables, _fixed)
{
	SoundDefinition *definition = GetSoundDefinition(sound_index);
	if (!definition) return;

	if (!source)
	{
		variables.volume = variables.left_volume = variables.right_volume = MAXIMUM_SOUND_VOLUME;
		// ghs: is this what the priority should be if there's no source?
		variables.priority = definition->behavior_index;
	}

	// and finally, do all the stuff we regularly do ...
	CalculateSoundVariables(sound_index, source, variables);
}

void SoundManager::InstantiateSoundVariables(Channel::Variables& variables, Channel& channel, bool first_time)
{
	if (first_time || variables.right_volume != channel.variables.right_volume || variables.left_volume != channel.variables.left_volume)
	{
		Mixer::instance()->SetChannelVolumes(channel.mixer_channel, variables.left_volume, variables.right_volume);
	}
	channel.variables = variables;
}

_fixed SoundManager::CalculatePitchModifier(short sound_index, _fixed pitch_modifier)
{
	SoundDefinition *definition = GetSoundDefinition(sound_index);
	if (!definition) return FIXED_ONE;

	if (!(definition->flags & _sound_cannot_change_pitch))
	{
		if (!(definition->flags & _sound_resists_pitch_changes))
		{
			pitch_modifier += ((FIXED_ONE-pitch_modifier)>>1);
		}
	}
	else
	{
		pitch_modifier= FIXED_ONE;
	}

	return pitch_modifier;
}

void SoundManager::AngleAndVolumeToStereoVolume(angle delta, short volume, short *right_volume, short *left_volume)
{
	if (parameters.flags & _stereo_flag)
	{
		short fraction = delta & ((1<<(ANGULAR_BITS-2))-1);
		short maximum_volume = volume + (volume >> 1);
		short minimum_volume = volume >> 2;
		short middle_volume = volume - minimum_volume;

		switch (NORMALIZE_ANGLE(delta)>>(ANGULAR_BITS-2))
		{
		case 0: // rear right quarter [v,vmax] [v,vmin]
			*left_volume= middle_volume + ((fraction*(maximum_volume-middle_volume))>>(ANGULAR_BITS-2));
			*right_volume= middle_volume + ((fraction*(minimum_volume-middle_volume))>>(ANGULAR_BITS-2));
			break;
			
		case 1: // front right quarter [vmax,vmid] [vmin,vmid]
			*left_volume= maximum_volume + ((fraction*(volume-maximum_volume))>>(ANGULAR_BITS-2));
			*right_volume= minimum_volume + ((fraction*(volume-minimum_volume))>>(ANGULAR_BITS-2));
			break;
			
		case 2: // front left quarter [vmid,vmin] [vmid,vmax]
			*left_volume= volume + ((fraction*(minimum_volume-volume))>>(ANGULAR_BITS-2));
			*right_volume= volume + ((fraction*(maximum_volume-volume))>>(ANGULAR_BITS-2));
			break;
			
		case 3: // rear left quarter [vmin,v] [vmax,v]
			*left_volume= minimum_volume + ((fraction*(middle_volume-minimum_volume))>>(ANGULAR_BITS-2));
			*right_volume= maximum_volume + ((fraction*(middle_volume-maximum_volume))>>(ANGULAR_BITS-2));
			break;
			
		default:
			assert(false);
			break;

		}
	}
	else
	{
		*left_volume = *right_volume = volume;
	}
}

short SoundManager::GetRandomSoundPermutation(short sound_index)
{
	SoundDefinition *definition = GetSoundDefinition(sound_index);
	if (!definition) return 0;

	short permutation;

	if (!(definition->permutations > 0)) return 0;

	if (parameters.flags & _more_sounds_flag)
	{
		if ((definition->permutations_played & ((1<<definition->permutations)-1))==((1<<definition->permutations)-1)) 
			definition->permutations_played = 0;
		permutation = local_random() % definition->permutations;
		while (definition->permutations_played & (1 << permutation)) 
			if ((permutation += 1) >= definition->permutations) 
				permutation = 0;
		definition->permutations_played |= 1 << permutation;
	}
	else
	{
		permutation = 0;
	}

	definition->last_played = machine_tick_count();

	return permutation;
}

void SoundManager::TrackStereoSounds()
{
	if (active && total_channel_count > 0 && (parameters.flags & _dynamic_tracking_flag))
	{
		for (int i = 0; i < parameters.channel_count; i++)
		{
			Channel *channel = &channels[i];
			if (SLOT_IS_USED(channel) && !Mixer::instance()->ChannelBusy(channel->mixer_channel) && !(channel->flags & _sound_is_local))
			{
				Channel::Variables variables = channel->variables;
				if (channel->dynamic_source) 
					channel->source = *channel->dynamic_source;
				CalculateSoundVariables(channel->sound_index, &channel->source, variables);
				InstantiateSoundVariables(variables, *channel, false);
			}
		}
	}
}

static void add_one_ambient_sound_source(struct ambient_sound_data *ambient_sounds,
					 world_location3d *source, world_location3d *listener, short sound_index,
					 short absolute_volume)
{
	SoundManager::instance()->AddOneAmbientSoundSource(ambient_sounds, source, listener, sound_index, absolute_volume);
}

void SoundManager::UpdateAmbientSoundSources()
{
	ambient_sound_data ambient_sounds[MAXIMUM_PROCESSED_AMBIENT_SOUNDS];

	bool channel_used[MAXIMUM_AMBIENT_SOUND_CHANNELS];
	bool sound_handled[MAXIMUM_PROCESSED_AMBIENT_SOUNDS];
	
	// reset all local copies
	for (short i = 0; i < MAXIMUM_PROCESSED_AMBIENT_SOUNDS; i++)
	{
		ambient_sounds[i].flags = 0;
		ambient_sounds[i].sound_index = NONE;

		sound_handled[i] = false;
	}
	
	for (short i = 0; i < MAXIMUM_AMBIENT_SOUND_CHANNELS; i++)
	{
		channel_used[i] = false;
	}

	// accumulate up to MAXIMUM_PROCESSED_AMBIENT_SOUNDS worth of sounds
	_sound_add_ambient_sources_proc(&ambient_sounds, add_one_ambient_sound_source);

	// remove all zero volume sounds
	for (short i = 0; i < MAXIMUM_PROCESSED_AMBIENT_SOUNDS; i++)
	{
		ambient_sound_data *ambient = &ambient_sounds[i];
		if (SLOT_IS_USED(ambient) && !ambient->variables.volume) 
			MARK_SLOT_AS_FREE(ambient);
	}

	{
		ambient_sound_data *lowest_priority;
		short count;

		do
		{
			lowest_priority = 0;
			count = 0;

			for (short i = 0; i < MAXIMUM_PROCESSED_AMBIENT_SOUNDS; i++)
			{
				ambient_sound_data *ambient = &ambient_sounds[i];
				if (SLOT_IS_USED(ambient))
				{
					if (!lowest_priority || lowest_priority->variables.volume > ambient->variables.volume + ABORT_AMPLITUDE_THRESHHOLD)
					{
						lowest_priority = ambient;
					}

					count++;
				}
			}

			if (count >  MAXIMUM_AMBIENT_SOUND_CHANNELS)
			{
				assert(lowest_priority);
				MARK_SLOT_AS_FREE(lowest_priority);
				count--;
			}
		}
		while (count > MAXIMUM_AMBIENT_SOUND_CHANNELS);

	}

	// update .variables of those sounds which we are already playing
	for (short i = 0; i < MAXIMUM_PROCESSED_AMBIENT_SOUNDS; i++)
	{
		ambient_sound_data *ambient = &ambient_sounds[i];
		if (SLOT_IS_USED(ambient))
		{
			for (short j = 0; j < MAXIMUM_AMBIENT_SOUND_CHANNELS; j++)
			{
				Channel *channel = &channels[parameters.channel_count + j];
				if (SLOT_IS_USED(channel) && channel->sound_index == ambient->sound_index)
				{
					InstantiateSoundVariables(ambient->variables, *channel, false);
					sound_handled[i] = channel_used[j] = true;
					break;
				}
			}
		}
	}

	// allocate a channel for a sound we just started playing
	for (short i = 0; i < MAXIMUM_PROCESSED_AMBIENT_SOUNDS; i++)
	{
		ambient_sound_data *ambient = &ambient_sounds[i];

		if (SLOT_IS_USED(ambient) && !sound_handled[i])
		{
			for (short j = 0; j < MAXIMUM_AMBIENT_SOUND_CHANNELS; j++)
			{
				Channel *channel = &channels[j + parameters.channel_count];
				if (!channel_used[j])
				{
					if (SLOT_IS_USED(channel)) FreeChannel(*channel);
					channel->flags = 0;
					channel->callback_count = 2; // #MD as if two sounds had just stopped playing
					channel->sound_index = ambient->sound_index;
					MARK_SLOT_AS_USED(channel);
					
					channel_used[j] = true;
					
					InstantiateSoundVariables(ambient->variables, *channel, true);
					break;
				}
			}
		}
	}

	// remove those sounds which are no longer being played
	for (short i = 0; i < MAXIMUM_AMBIENT_SOUND_CHANNELS; i++)
	{
		Channel *channel = &channels[i + parameters.channel_count];
		if (SLOT_IS_USED(channel) && !channel_used[i])
		{
			FreeChannel(*channel);
		}
	}

	// unlock those handles which need to be unlocked and buffer new sounds if necessary
	for (short i = 0; i < MAXIMUM_AMBIENT_SOUND_CHANNELS; i++)
	{
		Channel *channel = &channels[i + parameters.channel_count];
		if (SLOT_IS_USED(channel))
		{
			if (LoadSound(channel->sound_index))
			{
				while (channel->callback_count)
				{
					BufferSound(*channel, channel->sound_index, FIXED_ONE, false);
					channel->callback_count--;
				}
			}
		}
	}
}

// List of sounds

// Extra formerly-hardcoded sounds and their accessors; this is done for M1 compatibility:
// Added Ian-Rickard-style interface commands for button actions (map resizing, resolution changes, ...)

static short _Sound_TerminalLogon = _snd_computer_interface_logon;
static short _Sound_TerminalLogoff = _snd_computer_interface_logout;
static short _Sound_TerminalPage = _snd_computer_interface_page;

static short _Sound_TeleportIn = _snd_teleport_in;
static short _Sound_TeleportOut = _snd_teleport_out;

static short _Sound_GotPowerup = _snd_got_powerup;
static short _Sound_GotItem = _snd_got_item;

static short _Sound_Crunched = _snd_body_being_crunched;
static short _Sound_Exploding = _snd_juggernaut_exploding;

static short _Sound_Breathing = _snd_breathing;
static short _Sound_OxygenWarning = _snd_oxygen_warning;

static short _Sound_AdjustVolume = _snd_adjust_volume;

static short _Sound_ButtonSuccess = _snd_computer_interface_page;
static short _Sound_ButtonFailure = _snd_absorbed;
static short _Sound_ButtonInoperative = _snd_cant_toggle_switch;
static short _Sound_OGL_Reset = _snd_juggernaut_exploding;


short Sound_TerminalLogon() {return _Sound_TerminalLogon;}
short Sound_TerminalLogoff() {return _Sound_TerminalLogoff;}
short Sound_TerminalPage() {return _Sound_TerminalPage;}

short Sound_TeleportIn() {return _Sound_TeleportIn;}
short Sound_TeleportOut() {return _Sound_TeleportOut;}

short Sound_GotPowerup() {return _Sound_GotPowerup;}
short Sound_GotItem() {return _Sound_GotItem;}

short Sound_Crunched() {return _Sound_Crunched;}
short Sound_Exploding() {return _Sound_Exploding;}

short Sound_Breathing() {return _Sound_Breathing;}
short Sound_OxygenWarning() {return _Sound_OxygenWarning;}

short Sound_AdjustVolume() {return _Sound_AdjustVolume;}

short Sound_ButtonSuccess() {return _Sound_ButtonSuccess;}
short Sound_ButtonFailure() {return _Sound_ButtonFailure;}
short Sound_ButtonInoperative() {return _Sound_ButtonInoperative;}
short Sound_OGL_Reset() {return _Sound_OGL_Reset;}

// XML elements for parsing sound specifications;
// this is currently what ambient and random sounds are to be used

struct ambient_sound_definition *original_ambient_sound_definitions = NULL;
struct random_sound_definition *original_random_sound_definitions = NULL;

class XML_AmbientRandomAssignParser: public XML_ElementParser
{
	int Type;
	bool IsPresent[2];
	short Index, SoundIndex;
	
public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();
	bool ResetValues();

	enum {
		Ambient,
		Random
	};
	
	XML_AmbientRandomAssignParser(const char *_Name, int _Type): XML_ElementParser(_Name), Type(_Type) {}
};

bool XML_AmbientRandomAssignParser::Start()
{
	// back up old values first
	if (Type == Ambient) {
		if (!original_ambient_sound_definitions) {
			original_ambient_sound_definitions = (struct ambient_sound_definition *) malloc(sizeof(struct ambient_sound_definition) * NUMBER_OF_AMBIENT_SOUND_DEFINITIONS);
			assert(original_ambient_sound_definitions);
			for (int i = 0; i < NUMBER_OF_AMBIENT_SOUND_DEFINITIONS; i++)
				original_ambient_sound_definitions[i] = ambient_sound_definitions[i];
		}
	} else if (Type == Random) {
		if (!original_random_sound_definitions) {
			original_random_sound_definitions = (struct random_sound_definition *) malloc(sizeof(struct random_sound_definition) * NUMBER_OF_RANDOM_SOUND_DEFINITIONS);
			assert(original_random_sound_definitions);
			for (int i = 0; i < NUMBER_OF_RANDOM_SOUND_DEFINITIONS; i++)
				original_random_sound_definitions[i] = random_sound_definitions[i];
		}
	}

	for (int k=0; k<2; k++)
		IsPresent[k] = false;
	return true;
}

bool XML_AmbientRandomAssignParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"index"))
	{
		int ArrayLimit = 0;
		switch(Type)
		{
		case Ambient:
			ArrayLimit = NUMBER_OF_AMBIENT_SOUND_DEFINITIONS;
			break;
		case Random:
			ArrayLimit = NUMBER_OF_RANDOM_SOUND_DEFINITIONS;
			break;
		default:
			vassert(false,csprintf(temporary,"Unrecognized sound-parser class: %d",Type));
		}
		if (ReadBoundedInt16Value(Value,Index,0,ArrayLimit-1))
		{
			IsPresent[0] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"sound"))
	{
		if (ReadBoundedInt16Value(Value,SoundIndex,NONE,SHRT_MAX))
		{
			IsPresent[1] = true;
			return true;
		}
		else return false;
	}
	UnrecognizedTag();
	return false;
}

bool XML_AmbientRandomAssignParser::AttributesDone()
{
	// Verify...
	bool AllPresent = true;
	for (int k=0; k<2; k++)
		if (!IsPresent[k]) AllPresent = false;
	
	if (!AllPresent)
	{
		AttribsMissing();
		return false;
	}
	
	// Put into place
	switch(Type)
	{
	case Ambient:
		ambient_sound_definitions[Index].sound_index = SoundIndex;
		break;
	case Random:
		random_sound_definitions[Index].sound_index = SoundIndex;
		break;
	default:
		vassert(false,csprintf(temporary,"Unrecognized sound-parser class: %d",Type));
	}
	return true;
}

bool XML_AmbientRandomAssignParser::ResetValues()
{
	if (Type == Ambient) {
		if (original_ambient_sound_definitions) {
			for (int i = 0; i < NUMBER_OF_AMBIENT_SOUND_DEFINITIONS; i++)
				ambient_sound_definitions[i] = original_ambient_sound_definitions[i];
			free(original_ambient_sound_definitions);
			original_ambient_sound_definitions = NULL;
		}
	} else if (Type == Random) {
		if (original_random_sound_definitions) {
			for (int i = 0; i < NUMBER_OF_RANDOM_SOUND_DEFINITIONS; i++)
				random_sound_definitions[i] = original_random_sound_definitions[i];
			free(original_random_sound_definitions);
			original_random_sound_definitions = NULL;
		}
	}
	return true;
}

static XML_AmbientRandomAssignParser
	AmbientSoundAssignParser("ambient",XML_AmbientRandomAssignParser::Ambient),
	RandomSoundAssignParser("random",XML_AmbientRandomAssignParser::Random);



class XML_SO_ClearParser: public XML_ElementParser
{
public:
	bool Start();

	XML_SO_ClearParser(): XML_ElementParser("sound_clear") {}
};

bool XML_SO_ClearParser::Start()
{
	// Clear the list
	SoundReplacements::instance()->Reset();
	return true;
}

static XML_SO_ClearParser SO_ClearParser;


class XML_SoundOptionsParser: public XML_ElementParser
{
	bool IndexPresent;
	short Index, Slot;
	
	SoundOptions Data;

public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();
	bool ResetValues();

	XML_SoundOptionsParser(): XML_ElementParser("sound") {}
};

bool XML_SoundOptionsParser::Start()
{
	IndexPresent = false;
	Index = NONE;
	Slot = 0;			// Default: first slot
	Data.File.clear();
	
	return true;
}

bool XML_SoundOptionsParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"index"))
	{
		if (ReadInt16Value(Value,Index))
		{
			IndexPresent = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"slot"))
	{
		return ReadBoundedInt16Value(Value,Slot,0,MAXIMUM_PERMUTATIONS_PER_SOUND-1);
	}
	else if (StringsEqual(Tag,"file"))
	{
		size_t nchars = strlen(Value)+1;
		Data.File = Value;
		return true;
	}
	UnrecognizedTag();
	return false;
}

bool XML_SoundOptionsParser::AttributesDone()
{
	// Verify...
	if (!IndexPresent)
	{
		AttribsMissing();
		return false;
	}

	SoundReplacements::instance()->Add(Data, Index, Slot);

	return true;
}

bool XML_SoundOptionsParser::ResetValues()
{
	SoundReplacements::instance()->Reset();
	return true;
}

static XML_SoundOptionsParser SoundOptionsParser;

// Subclassed to set the sounds appropriately
class XML_SoundsParser: public XML_ElementParser
{
public:
	bool HandleAttribute(const char *Tag, const char *Value);
	
	XML_SoundsParser(): XML_ElementParser("sounds") {}
};

bool XML_SoundsParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"terminal_logon"))
	{
		return ReadInt16Value(Value,_Sound_TerminalLogon);
	}
	else if (StringsEqual(Tag,"terminal_logoff"))
	{
		return ReadInt16Value(Value,_Sound_TerminalLogoff);
	}
	else if (StringsEqual(Tag,"terminal_page"))
	{
		return ReadInt16Value(Value,_Sound_TerminalPage);
	}
	else if (StringsEqual(Tag,"teleport_in"))
	{
		return ReadInt16Value(Value,_Sound_TeleportIn);
	}
	else if (StringsEqual(Tag,"teleport_out"))
	{
		return ReadInt16Value(Value,_Sound_TeleportOut);
	}
	else if (StringsEqual(Tag,"got_powerup"))
	{
		return ReadInt16Value(Value,_Sound_GotPowerup);
	}
	else if (StringsEqual(Tag,"got_item"))
	{
		return ReadInt16Value(Value,_Sound_GotItem);
	}
	else if (StringsEqual(Tag,"crunched"))
	{
		return ReadInt16Value(Value,_Sound_Crunched);
	}
	else if (StringsEqual(Tag,"exploding"))
	{
		return ReadInt16Value(Value,_Sound_Exploding);
	}
	else if (StringsEqual(Tag,"breathing"))
	{
		return ReadInt16Value(Value,_Sound_Breathing);
	}
	else if (StringsEqual(Tag,"oxygen_warning"))
	{
		return ReadInt16Value(Value,_Sound_OxygenWarning);
	}
	else if (StringsEqual(Tag,"adjust_volume"))
	{
		return ReadInt16Value(Value,_Sound_AdjustVolume);
	}
	else if (StringsEqual(Tag,"button_success"))
	{
		return ReadInt16Value(Value,_Sound_ButtonSuccess);
	}
	else if (StringsEqual(Tag,"button_failure"))
	{
		return ReadInt16Value(Value,_Sound_ButtonFailure);
	}
	else if (StringsEqual(Tag,"button_inoperative"))
	{
		return ReadInt16Value(Value,_Sound_ButtonInoperative);
	}
	else if (StringsEqual(Tag,"ogl_reset"))
	{
		return ReadInt16Value(Value,_Sound_OGL_Reset);
	}
	UnrecognizedTag();
	return false;
}


static XML_SoundsParser SoundsParser;

// LP change: added infravision-parser export
XML_ElementParser *Sounds_GetParser()
{
	SoundsParser.AddChild(&AmbientSoundAssignParser);
	SoundsParser.AddChild(&RandomSoundAssignParser);
	SoundsParser.AddChild(&SoundOptionsParser);
	SoundsParser.AddChild(&SO_ClearParser);
	
	return &SoundsParser;
}
