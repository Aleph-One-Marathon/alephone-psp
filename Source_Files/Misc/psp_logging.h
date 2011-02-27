#ifndef __PSP_LOGGING_H__
#define __PSP_LOGGING_H__

#define STRINGIFY(x) #x
#define STRINGIFY_MACRO(x) STRINGIFY(x)

#ifdef PSP
#define PSP_STRLOG(str) \
	printf("PSP: %s - %s:%s\n", str, __FILE__, STRINGIFY_MACRO(__LINE__))
#else
#define PSP_STRLOG(str)
#endif

#endif /* __PSP_LOGGING_H__ */
