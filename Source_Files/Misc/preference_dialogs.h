/*

	Copyright (C) 2006 and beyond by Bungie Studios, Inc.
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

#ifndef PREFERENCE_DIALOGS_H
#define PREFERENCE_DIALOGS_H

#include "shared_widgets.h"
#include "OGL_Setup.h"

class OpenGLDialog
{
public:
	// Abstract factory; concrete type determined at link-time
	static std::auto_ptr<OpenGLDialog> Create();

	void OpenGLPrefsByRunning ();

	virtual ~OpenGLDialog ();
protected:
	OpenGLDialog ();
	
	virtual bool Run () = 0;
	virtual void Stop (bool result) = 0;
	
	ButtonWidget*		m_cancelWidget;
	ButtonWidget*		m_okWidget;
	
	ToggleWidget*		m_zBufferWidget;
	ToggleWidget*		m_fogWidget;
	ToggleWidget*		m_staticEffectWidget;
	ToggleWidget*		m_colourEffectsWidget;
	ToggleWidget*		m_transparentLiquidsWidget;
	ToggleWidget*		m_3DmodelsWidget;
	
	ToggleWidget*		m_colourTheVoidWidget;
	ColourPickerWidget*	m_voidColourWidget;
	
	SelectorWidget*		m_fsaaWidget;
	
	SelectorWidget*		m_anisotropicWidget;
	
	SelectorWidget* m_textureQualityWidget [OGL_NUMBER_OF_TEXTURE_TYPES];
	SelectorWidget* m_modelQualityWidget;
	
	ToggleWidget*		m_geForceFixWidget;
	SelectSelectorWidget*		m_wallsFilterWidget;
	SelectSelectorWidget*		m_spritesFilterWidget;
	
	SelectorWidget* m_textureResolutionWidget [OGL_NUMBER_OF_TEXTURE_TYPES];
	SelectorWidget* m_textureDepthWidget [OGL_NUMBER_OF_TEXTURE_TYPES];
};

#endif
