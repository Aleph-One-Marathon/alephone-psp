#ifndef __PSP_SDL_PROFILER__
#define __PSP_SDL_PROFILER__

#include <cstdio>
#include <vector>

class PSPSDLProfiler {
	
private:

	static const int MAX_FUNC_NAME = 1024;
	
public:

	PSPSDLProfiler(const char* functionName);
	
	void				setFunctionName(const char* functionName);
	const char*			getFunctionName(void) { return functionName; }
	
	void				begin(void);
	void				end(void);
	
	float				getAvarageTime(void) { return avarageTime; }
	float				getTotalTime(void) { return totalTime; }
	unsigned long		getTotalCalls(void) { return totalCalls; }
	
	void				setParent(PSPSDLProfiler* p);
	PSPSDLProfiler*		getParent(void) { return parent; }
	
	void				addChild(PSPSDLProfiler* p);
							
	PSPSDLProfiler*		getChild(unsigned int num)
							{ return (num < childs.size() ? 
								childs[num] : NULL); }
								
	unsigned int		getChildsCount(void) { return childs.size(); }
	
	void				dump(FILE* f, unsigned int level = 0);

private:
	char				functionName[MAX_FUNC_NAME];
	float				avarageTime;
	float				totalTime;
	unsigned long		totalCalls;
	unsigned long		lastTicks;
	
	PSPSDLProfiler*					parent;
	std::vector<PSPSDLProfiler*>	childs;
};

#endif /* __PSP_SDL_PROFILER__ */
