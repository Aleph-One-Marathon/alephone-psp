#include <cstdio>
#include <cstring>
#include <SDL.h>

#include "psp_sdl_profiler.h"

PSPSDLProfiler::PSPSDLProfiler(const char* functionName)
{
	setFunctionName(functionName);
	avarageTime = 0.0f;
	totalCalls = 0;
	lastTicks = 0;
}
	
void PSPSDLProfiler::setFunctionName(const char* functionName)
{
	strncpy(this->functionName, functionName, MAX_FUNC_NAME);
}

void PSPSDLProfiler::setParent(PSPSDLProfiler* p)
{
	parent = p;
}

void PSPSDLProfiler::addChild(PSPSDLProfiler* p)
{
	if (!p) return;
	
	for (unsigned int i=0; i < getChildsCount(); i++)
		if (getChild(i) == p) return;
		
	childs.push_back(p);
}
	
void PSPSDLProfiler::begin(void)
{
	totalCalls++;
	lastTicks = SDL_GetTicks();
}

void PSPSDLProfiler::end(void)
{
	unsigned long delta = SDL_GetTicks() - lastTicks;
	
	totalTime += delta;
	
	avarageTime += delta;
	
	if (totalCalls > 1)
		avarageTime /= 2.0f;
	
}

void PSPSDLProfiler::dump(FILE* f, unsigned int level)
{
	if (!f) return;
	
	// Indent the entry according to its level
	for (unsigned int j=0; j < level; j++)
		fprintf(f, "\t");
		
	fprintf(f,"<%s caller=\"%s\" called=\"%ld\" avr-time=\"%f\" tot-time=\"%f\">\n",
			getFunctionName(),
			(parent ? parent->getFunctionName() : "none"),
			getTotalCalls(),
			getAvarageTime(),
			getTotalTime());
			
	for (unsigned int i=0; i < getChildsCount(); i++)
		getChild(i)->dump(f, level+1);
	
	// Indent the entry according to its level
	for (unsigned int j=0; j < level; j++)
		fprintf(f, "\t");
	fprintf(f,"</%s>\n",getFunctionName());
}
