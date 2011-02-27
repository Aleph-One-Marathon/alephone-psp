#ifndef __PSP_SDL_PROFILERMG__
#define __PSP_SDL_PROFILERMG__

#include <vector>
#include "psp_sdl_profiler.h"

#ifdef PSP
#define PSPROF_MARK_BEGIN(x, m) 	if (x) (x)->createProfiler(m)
#define PSPROF_MARK_END(x, m) 		if (x) (x)->end(m)
#define PSPROF_BEGIN(x) 			PSPROF_MARK_BEGIN(x, __FUNCTION__)
#define PSPROF_END(x) 				PSPROF_MARK_END(x, __FUNCTION__)
#else
#define PSPROF_MARK_BEGIN(x, m)
#define PSPROF_MARK_END(x, m)
#define PSPROF_BEGIN(x)
#define PSPROF_END(x)
#endif

class PSPSDLProfilerMg {
	
public:

	PSPSDLProfilerMg();
	~PSPSDLProfilerMg();

	PSPSDLProfiler*			createProfiler(const char* functionName, 
											bool begin = true);
											
	PSPSDLProfiler*			getProfiler(const char* functionName);
	
	void					begin(PSPSDLProfiler* p);
	
	void					end(PSPSDLProfiler* p);
	void					end(const char* functionName);
	
	void					dump(const char* fileName);

private:
	
	std::vector<PSPSDLProfiler*>	profilers;
	PSPSDLProfiler*					lastCalled;
	
};


#endif /* __PSP_SDL_PROFILERMG__ */
