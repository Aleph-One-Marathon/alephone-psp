#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "psp_sdl_profilermg.h"
#include "psp_sdl_profiler.h"

PSPSDLProfilerMg::PSPSDLProfilerMg()
{
	lastCalled = NULL;
}

PSPSDLProfilerMg::~PSPSDLProfilerMg()
{
	for (unsigned int i=0; i < profilers.size(); i++)
		if (profilers[i]) delete profilers[i];
}

void PSPSDLProfilerMg::begin(PSPSDLProfiler* p)
{
	if (!p) return;
	
	p->setParent(lastCalled);
	
	if (lastCalled) 
		lastCalled->addChild(p);
	
	p->begin();	
	
	lastCalled = p;
}

void PSPSDLProfilerMg::end(const char* functionName)
{
	end(getProfiler(functionName));
}

void PSPSDLProfilerMg::end(PSPSDLProfiler* p)
{
	if (!p) return;
	p->end();
	
	lastCalled = p->getParent();
}

PSPSDLProfiler* PSPSDLProfilerMg::createProfiler(const char* functionName, 
													bool begin)
{
	// Do not create a new profiler if one already exists
	PSPSDLProfiler* p = getProfiler(functionName);
	
	// If it doesn't exists than create one and push it in the list
	if (!p) {
		p = new PSPSDLProfiler(functionName);
		if (!p) return NULL;
		
		profilers.push_back(p);
	}
	
	// Anyway if instructed to do so, begin.
	if (begin) this->begin(p);
	
	return p;
}

PSPSDLProfiler*	PSPSDLProfilerMg::getProfiler(const char* functionName)
{
	for (unsigned int i=0; i < profilers.size(); i++) {
		PSPSDLProfiler* p = profilers[i];
		if (!p) continue;
		
		if (strcmp(p->getFunctionName(), functionName) == 0)
			return p;
	}
	
	return NULL;
}

void PSPSDLProfilerMg::dump(const char* fileName)
{
	FILE* f = fopen(fileName, "w");
	if (!f) return;
	if (profilers.size() == 0) return;
	
	fprintf(f,"<?xml version=\"1.0\"?>\n");
	
	PSPSDLProfiler* root = profilers[0];
	if (root) root->dump(f,0);
	
	fclose(f);
}
