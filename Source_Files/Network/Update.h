#ifndef __UPDATE_H
#define __UPDATE_H

/*

	Copyright (C) 2007 Gregory Smith.
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

	Checks for updates online

*/

#include <string>
#ifdef __MWERKS__
#include <SDL_thread.h>
#else
#include <SDL/SDL_thread.h>
#endif
#include "cseries.h"

class Update
{
public:
	static Update *instance() { if (!m_instance) m_instance = new Update(); return m_instance; }

	enum Status
	{
		CheckingForUpdate,
		UpdateCheckFailed,
		UpdateAvailable,
		NoUpdateAvailable
	};

	Status GetStatus() { return m_status; }
	std::string NewDisplayVersion() { assert(m_status == UpdateAvailable); return m_new_display_version; }

private:
	static Update *m_instance;
	Update();
	~Update();

	void StartUpdateCheck();
	static int update_thread(void *);
	int Thread();

	Status m_status;
	std::string m_new_date_version;
	std::string m_new_display_version;
	SDL_Thread *m_thread;

};

#endif
