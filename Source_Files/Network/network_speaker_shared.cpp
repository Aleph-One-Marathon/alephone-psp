#include <cstdlib>
/*
 *  network_speaker_shared.cpp
 *  created for Marathon: Aleph One <http://source.bungie.org/>

	Copyright (C) 2002 and beyond by Woody Zenfell, III
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

 *  The code in this file is licensed to you under the GNU GPL.  As the copyright holder,
 *  however, I reserve the right to use this code as I see fit, without being bound by the
 *  GPL's terms.  This exemption is not intended to apply to modified versions of this file -
 *  if I were to use a modified version, I would be a licensee of whomever modified it, and
 *  thus must observe the GPL terms.
 *
 *  Network speaker-related code usable by multiple platforms.
 *
 *  Created by woody Feb 1, 2003, largely from stuff in network_speaker_sdl.cpp.
 *
 *  May 28, 2003 (Gregory Smith):
 *	Speex audio decompression 
 */

#if !defined(DISABLE_NETWORKING)

#include "cseries.h"
#include "network_sound.h"
#include "network_data_formats.h"
#include "network_audio_shared.h"
#include "player.h"

#ifdef SPEEX
#include "speex.h"
#include "network_speex.h"
#endif

// This is what the network distribution system calls when audio is received.
void
received_network_audio_proc(void *buffer, short buffer_size, short player_index) {
    network_audio_header_NET* theHeader_NET = (network_audio_header_NET*) buffer;

    network_audio_header    theHeader;

    netcpy(&theHeader, theHeader_NET);
    
    byte* theSoundData = ((byte*)buffer) + sizeof(network_audio_header_NET);

    // 0 if using uncompressed audio, 1 if using speex
    if(!(theHeader.mFlags & kNetworkAudioForTeammatesOnlyFlag) || (local_player->team == get_player_data(player_index)->team)) {
        if (theHeader.mReserved == 0) {
            queue_network_speaker_data(theSoundData, buffer_size - sizeof(network_audio_header_NET));
        } 
#ifdef SPEEX
        else if (theHeader.mReserved == 1) {

            // decode the data
            float frame[160];
            char cbits[200];
            int nbytes;
            int i;
            byte *newBuffer = NULL;
            int totalBytes = 0;
            
            // allocate buffer space
            int numFrames = 0;
            while (theSoundData < static_cast<byte*>(buffer) + buffer_size) {
                nbytes = *theSoundData++;
                theSoundData += nbytes;
                numFrames++;
            }
            newBuffer = (byte *) malloc (sizeof(char) * 160 * numFrames);
            theSoundData = ((byte*)buffer) + sizeof(network_audio_header_NET);
            
            while (theSoundData < static_cast<byte*>(buffer) + buffer_size) {
                // copy a frame into the decoder
                nbytes = *theSoundData++;
                for (i = 0; i < nbytes; i++) {
                    cbits[i] = *theSoundData++;
                }
                speex_bits_read_from(&gDecoderBits, cbits, nbytes);
                speex_decode(gDecoderState, &gDecoderBits, frame);
                for (i = 0; i < 160; i++) {
                    int16 framedata = static_cast<int16>(frame[i]);
                    newBuffer[totalBytes++] = 128 + (framedata >> 8);
                }
            }
            
            queue_network_speaker_data(newBuffer, 160 * numFrames);
            free(newBuffer);
        }
#endif
    }
}

#endif // !defined(DISABLE_NETWORKING)
