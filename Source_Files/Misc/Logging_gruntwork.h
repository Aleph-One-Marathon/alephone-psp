// This file is automatically generated by aleph/tools/gen-Logging_gruntwork.csh
// Make changes to that script, not to this file.
 
#if 0
// Straight main-thread logging convenience macros
#define logFatal(message)	(GetCurrentLogger()->logMessage(logDomain, logFatalLevel, __FILE__, __LINE__, (message)))
#define logError(message)	(GetCurrentLogger()->logMessage(logDomain, logErrorLevel, __FILE__, __LINE__, (message)))
#define logWarning(message)	(GetCurrentLogger()->logMessage(logDomain, logWarningLevel, __FILE__, __LINE__, (message)))
#define logAnomaly(message)	(GetCurrentLogger()->logMessage(logDomain, logAnomalyLevel, __FILE__, __LINE__, (message)))
#define logNote(message)	(GetCurrentLogger()->logMessage(logDomain, logNoteLevel, __FILE__, __LINE__, (message)))
#define logSummary(message)	(GetCurrentLogger()->logMessage(logDomain, logSummaryLevel, __FILE__, __LINE__, (message)))
#define logTrace(message)	(GetCurrentLogger()->logMessage(logDomain, logTraceLevel, __FILE__, __LINE__, (message)))
#define logDump(message)	(GetCurrentLogger()->logMessage(logDomain, logDumpLevel, __FILE__, __LINE__, (message)))
#else
// this obsoletes this whole file, pretty much - since macros can support var args and stuff :P
#define logFatal(...)    (GetCurrentLogger()->logMessage(logDomain, logFatalLevel, __FILE__, __LINE__, __VA_ARGS__))
#define logError(...)    (GetCurrentLogger()->logMessage(logDomain, logErrorLevel, __FILE__, __LINE__, __VA_ARGS__))
#define logWarning(...)  (GetCurrentLogger()->logMessage(logDomain, logWarningLevel, __FILE__, __LINE__, __VA_ARGS__))
#define logAnomaly(...)  (GetCurrentLogger()->logMessage(logDomain, logAnomalyLevel, __FILE__, __LINE__, __VA_ARGS__))
#define logNote(...)     (GetCurrentLogger()->logMessage(logDomain, logNoteLevel, __FILE__, __LINE__, __VA_ARGS__))
#define logSummary(...)  (GetCurrentLogger()->logMessage(logDomain, logSummaryLevel, __FILE__, __LINE__, __VA_ARGS__))
#define logTrace(...)    (GetCurrentLogger()->logMessage(logDomain, logTraceLevel, __FILE__, __LINE__, __VA_ARGS__))
#define logDump(...)     (GetCurrentLogger()->logMessage(logDomain, logDumpLevel, __FILE__, __LINE__, __VA_ARGS__))
#endif


#define logFatal1(message, arg1)	(GetCurrentLogger()->logMessage(logDomain, logFatalLevel, __FILE__, __LINE__, (message), (arg1)))
#define logError1(message, arg1)	(GetCurrentLogger()->logMessage(logDomain, logErrorLevel, __FILE__, __LINE__, (message), (arg1)))
#define logWarning1(message, arg1)	(GetCurrentLogger()->logMessage(logDomain, logWarningLevel, __FILE__, __LINE__, (message), (arg1)))
#define logAnomaly1(message, arg1)	(GetCurrentLogger()->logMessage(logDomain, logAnomalyLevel, __FILE__, __LINE__, (message), (arg1)))
#define logNote1(message, arg1)	(GetCurrentLogger()->logMessage(logDomain, logNoteLevel, __FILE__, __LINE__, (message), (arg1)))
#define logSummary1(message, arg1)	(GetCurrentLogger()->logMessage(logDomain, logSummaryLevel, __FILE__, __LINE__, (message), (arg1)))
#define logTrace1(message, arg1)	(GetCurrentLogger()->logMessage(logDomain, logTraceLevel, __FILE__, __LINE__, (message), (arg1)))
#define logDump1(message, arg1)	(GetCurrentLogger()->logMessage(logDomain, logDumpLevel, __FILE__, __LINE__, (message), (arg1)))

#define logFatal2(message, arg1, arg2)	(GetCurrentLogger()->logMessage(logDomain, logFatalLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))
#define logError2(message, arg1, arg2)	(GetCurrentLogger()->logMessage(logDomain, logErrorLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))
#define logWarning2(message, arg1, arg2)	(GetCurrentLogger()->logMessage(logDomain, logWarningLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))
#define logAnomaly2(message, arg1, arg2)	(GetCurrentLogger()->logMessage(logDomain, logAnomalyLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))
#define logNote2(message, arg1, arg2)	(GetCurrentLogger()->logMessage(logDomain, logNoteLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))
#define logSummary2(message, arg1, arg2)	(GetCurrentLogger()->logMessage(logDomain, logSummaryLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))
#define logTrace2(message, arg1, arg2)	(GetCurrentLogger()->logMessage(logDomain, logTraceLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))
#define logDump2(message, arg1, arg2)	(GetCurrentLogger()->logMessage(logDomain, logDumpLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))

#define logFatal3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessage(logDomain, logFatalLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))
#define logError3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessage(logDomain, logErrorLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))
#define logWarning3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessage(logDomain, logWarningLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))
#define logAnomaly3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessage(logDomain, logAnomalyLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))
#define logNote3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessage(logDomain, logNoteLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))
#define logSummary3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessage(logDomain, logSummaryLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))
#define logTrace3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessage(logDomain, logTraceLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))
#define logDump3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessage(logDomain, logDumpLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))

#define logFatal4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessage(logDomain, logFatalLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))
#define logError4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessage(logDomain, logErrorLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))
#define logWarning4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessage(logDomain, logWarningLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))
#define logAnomaly4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessage(logDomain, logAnomalyLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))
#define logNote4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessage(logDomain, logNoteLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))
#define logSummary4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessage(logDomain, logSummaryLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))
#define logTrace4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessage(logDomain, logTraceLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))
#define logDump4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessage(logDomain, logDumpLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))

#define logFatal5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessage(logDomain, logFatalLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))
#define logError5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessage(logDomain, logErrorLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))
#define logWarning5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessage(logDomain, logWarningLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))
#define logAnomaly5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessage(logDomain, logAnomalyLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))
#define logNote5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessage(logDomain, logNoteLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))
#define logSummary5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessage(logDomain, logSummaryLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))
#define logTrace5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessage(logDomain, logTraceLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))
#define logDump5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessage(logDomain, logDumpLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))

 
// Analogues to the above, but use these when you might be running in a non-main thread
#define logFatalNMT(message)	(GetCurrentLogger()->logMessageNMT(logDomain, logFatalLevel, __FILE__, __LINE__, (message)))
#define logErrorNMT(message)	(GetCurrentLogger()->logMessageNMT(logDomain, logErrorLevel, __FILE__, __LINE__, (message)))
#define logWarningNMT(message)	(GetCurrentLogger()->logMessageNMT(logDomain, logWarningLevel, __FILE__, __LINE__, (message)))
#define logAnomalyNMT(message)	(GetCurrentLogger()->logMessageNMT(logDomain, logAnomalyLevel, __FILE__, __LINE__, (message)))
#define logNoteNMT(message)	(GetCurrentLogger()->logMessageNMT(logDomain, logNoteLevel, __FILE__, __LINE__, (message)))
#define logSummaryNMT(message)	(GetCurrentLogger()->logMessageNMT(logDomain, logSummaryLevel, __FILE__, __LINE__, (message)))
#define logTraceNMT(message)	(GetCurrentLogger()->logMessageNMT(logDomain, logTraceLevel, __FILE__, __LINE__, (message)))
#define logDumpNMT(message)	(GetCurrentLogger()->logMessageNMT(logDomain, logDumpLevel, __FILE__, __LINE__, (message)))

#define logFatalNMT1(message, arg1)	(GetCurrentLogger()->logMessageNMT(logDomain, logFatalLevel, __FILE__, __LINE__, (message), (arg1)))
#define logErrorNMT1(message, arg1)	(GetCurrentLogger()->logMessageNMT(logDomain, logErrorLevel, __FILE__, __LINE__, (message), (arg1)))
#define logWarningNMT1(message, arg1)	(GetCurrentLogger()->logMessageNMT(logDomain, logWarningLevel, __FILE__, __LINE__, (message), (arg1)))
#define logAnomalyNMT1(message, arg1)	(GetCurrentLogger()->logMessageNMT(logDomain, logAnomalyLevel, __FILE__, __LINE__, (message), (arg1)))
#define logNoteNMT1(message, arg1)	(GetCurrentLogger()->logMessageNMT(logDomain, logNoteLevel, __FILE__, __LINE__, (message), (arg1)))
#define logSummaryNMT1(message, arg1)	(GetCurrentLogger()->logMessageNMT(logDomain, logSummaryLevel, __FILE__, __LINE__, (message), (arg1)))
#define logTraceNMT1(message, arg1)	(GetCurrentLogger()->logMessageNMT(logDomain, logTraceLevel, __FILE__, __LINE__, (message), (arg1)))
#define logDumpNMT1(message, arg1)	(GetCurrentLogger()->logMessageNMT(logDomain, logDumpLevel, __FILE__, __LINE__, (message), (arg1)))

#define logFatalNMT2(message, arg1, arg2)	(GetCurrentLogger()->logMessageNMT(logDomain, logFatalLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))
#define logErrorNMT2(message, arg1, arg2)	(GetCurrentLogger()->logMessageNMT(logDomain, logErrorLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))
#define logWarningNMT2(message, arg1, arg2)	(GetCurrentLogger()->logMessageNMT(logDomain, logWarningLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))
#define logAnomalyNMT2(message, arg1, arg2)	(GetCurrentLogger()->logMessageNMT(logDomain, logAnomalyLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))
#define logNoteNMT2(message, arg1, arg2)	(GetCurrentLogger()->logMessageNMT(logDomain, logNoteLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))
#define logSummaryNMT2(message, arg1, arg2)	(GetCurrentLogger()->logMessageNMT(logDomain, logSummaryLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))
#define logTraceNMT2(message, arg1, arg2)	(GetCurrentLogger()->logMessageNMT(logDomain, logTraceLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))
#define logDumpNMT2(message, arg1, arg2)	(GetCurrentLogger()->logMessageNMT(logDomain, logDumpLevel, __FILE__, __LINE__, (message), (arg1), (arg2)))

#define logFatalNMT3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessageNMT(logDomain, logFatalLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))
#define logErrorNMT3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessageNMT(logDomain, logErrorLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))
#define logWarningNMT3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessageNMT(logDomain, logWarningLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))
#define logAnomalyNMT3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessageNMT(logDomain, logAnomalyLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))
#define logNoteNMT3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessageNMT(logDomain, logNoteLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))
#define logSummaryNMT3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessageNMT(logDomain, logSummaryLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))
#define logTraceNMT3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessageNMT(logDomain, logTraceLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))
#define logDumpNMT3(message, arg1, arg2, arg3)	(GetCurrentLogger()->logMessageNMT(logDomain, logDumpLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3)))

#define logFatalNMT4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessageNMT(logDomain, logFatalLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))
#define logErrorNMT4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessageNMT(logDomain, logErrorLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))
#define logWarningNMT4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessageNMT(logDomain, logWarningLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))
#define logAnomalyNMT4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessageNMT(logDomain, logAnomalyLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))
#define logNoteNMT4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessageNMT(logDomain, logNoteLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))
#define logSummaryNMT4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessageNMT(logDomain, logSummaryLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))
#define logTraceNMT4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessageNMT(logDomain, logTraceLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))
#define logDumpNMT4(message, arg1, arg2, arg3, arg4)	(GetCurrentLogger()->logMessageNMT(logDomain, logDumpLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4)))

#define logFatalNMT5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessageNMT(logDomain, logFatalLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))
#define logErrorNMT5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessageNMT(logDomain, logErrorLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))
#define logWarningNMT5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessageNMT(logDomain, logWarningLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))
#define logAnomalyNMT5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessageNMT(logDomain, logAnomalyLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))
#define logNoteNMT5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessageNMT(logDomain, logNoteLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))
#define logSummaryNMT5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessageNMT(logDomain, logSummaryLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))
#define logTraceNMT5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessageNMT(logDomain, logTraceLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))
#define logDumpNMT5(message, arg1, arg2, arg3, arg4, arg5)	(GetCurrentLogger()->logMessageNMT(logDomain, logDumpLevel, __FILE__, __LINE__, (message), (arg1), (arg2), (arg3), (arg4), (arg5)))

 
// Main-thread log context convenience macros
#define logContext(context)	LogContext makeUniqueIdentifier(_theLogContext,__LINE__)(false, __FILE__, __LINE__, (context))
#define logContext1(context, arg1)	LogContext makeUniqueIdentifier(_theLogContext,__LINE__)(false, __FILE__, __LINE__, (context), (arg1))
#define logContext2(context, arg1, arg2)	LogContext makeUniqueIdentifier(_theLogContext,__LINE__)(false, __FILE__, __LINE__, (context), (arg1), (arg2))
#define logContext3(context, arg1, arg2, arg3)	LogContext makeUniqueIdentifier(_theLogContext,__LINE__)(false, __FILE__, __LINE__, (context), (arg1), (arg2), (arg3))
#define logContext4(context, arg1, arg2, arg3, arg4)	LogContext makeUniqueIdentifier(_theLogContext,__LINE__)(false, __FILE__, __LINE__, (context), (arg1), (arg2), (arg3), (arg4))
#define logContext5(context, arg1, arg2, arg3, arg4, arg5)	LogContext makeUniqueIdentifier(_theLogContext,__LINE__)(false, __FILE__, __LINE__, (context), (arg1), (arg2), (arg3), (arg4), (arg5))
 
// Non-main-thread analogues to the above
#define logContextNMT(context)	LogContext makeUniqueIdentifier(_theLogContext,__LINE__)(true, __FILE__, __LINE__, (context))
#define logContextNMT1(context, arg1)	LogContext makeUniqueIdentifier(_theLogContext,__LINE__)(true, __FILE__, __LINE__, (context), (arg1))
#define logContextNMT2(context, arg1, arg2)	LogContext makeUniqueIdentifier(_theLogContext,__LINE__)(true, __FILE__, __LINE__, (context), (arg1), (arg2))
#define logContextNMT3(context, arg1, arg2, arg3)	LogContext makeUniqueIdentifier(_theLogContext,__LINE__)(true, __FILE__, __LINE__, (context), (arg1), (arg2), (arg3))
#define logContextNMT4(context, arg1, arg2, arg3, arg4)	LogContext makeUniqueIdentifier(_theLogContext,__LINE__)(true, __FILE__, __LINE__, (context), (arg1), (arg2), (arg3), (arg4))
#define logContextNMT5(context, arg1, arg2, arg3, arg4, arg5)	LogContext makeUniqueIdentifier(_theLogContext,__LINE__)(true, __FILE__, __LINE__, (context), (arg1), (arg2), (arg3), (arg4), (arg5))
 
