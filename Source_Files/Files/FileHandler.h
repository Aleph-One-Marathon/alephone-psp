#ifndef _FILE_HANDLER_
#define _FILE_HANDLER_
/*

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
	
	File-handler classes
	by Loren Petrich,
	August 11, 2000

	These are designed to provide some abstract interfaces to file and directory objects.
	
	Most of these routines return whether they had succeeded;
	more detailed error codes are API-specific.
	Attempted to support stdio I/O directly, but on my Macintosh, at least,
	the performance was much poorer. This is possibly due to "fseek" having to
	actually read the file or something.
	
	Merged all the Macintosh-specific code into these base classes, so that
	it will be selected with a preprocessor statement when more than one file-I/O
	API is supported.

Dec 7, 2000 (Loren Petrich):
	Added a MacOS-specific file-creation function that allows direct specification
	of type and creator codes

Jan 25, 2002 (Br'fin (Jeremy Parsons)):
	Added TARGET_API_MAC_CARBON for Carbon.h
	Rearranged initializers in DirectorySpecifier constructor to appease compiler warnings

March 18, 2002 (Br'fin (Jeremy Parsons)):
	Added FileSpecifier::SetParentToResources for Carbon
*/
#ifdef SDL_RFORK_HACK
#undef SDL
#define mac
#endif

#ifdef mac
#if defined(EXPLICIT_CARBON_HEADER)
    #include <Carbon/Carbon.h>
/*
#else
#include <Files.h>
#include <Resources.h>
#endif
*/
#endif
#endif

// For the filetypes
#include "tags.h"

#include <stddef.h>	// For size_t
#include <time.h>	// For time_t
#include <vector>
#include <SDL.h>
using namespace std;

#ifdef SDL
#include <errno.h>
#include <string>
#define fnfErr ENOENT
#ifndef NO_STD_NAMESPACE
using std::string;
using std::vector;
#endif
#endif

#ifdef __WIN32__
#include <windows.h>
#ifdef GetFreeSpace
#undef GetFreeSpace
#endif
#ifdef CreateDirectory
#undef CreateDirectory
#endif
#endif


// Symbolic constant for a closed file's reference number (refnum) (MacOS only)
const short RefNum_Closed = -1;


/*
	Abstraction for opened files; it does reading, writing, and closing of such files,
	without doing anything to the files' specifications
*/
class OpenedFile
{
	// This class will need to set the refnum and error value appropriately 
	friend class FileSpecifier;
	
public:
	bool IsOpen();
	bool Close();
	
	bool GetPosition(long& Position);
	bool SetPosition(long Position);
	
	bool GetLength(long& Length);
	bool SetLength(long Length);
	
	bool Read(long Count, void *Buffer);
	bool Write(long Count, void *Buffer);
		
	OpenedFile();
	~OpenedFile() {Close();}	// Auto-close when destroying

	// Platform-specific members
#if defined(mac)

	short GetRefNum() {return RefNum;}
	OSErr GetError() {return Err;}

private:
	short RefNum;	// File reference number
	OSErr Err;		// Error code

#elif defined(SDL)

	int GetError() {return err;}
	SDL_RWops *GetRWops() {return f;}
	SDL_RWops *TakeRWops();		// Hand over SDL_RWops

private:
	SDL_RWops *f;	// File handle
	int err;		// Error code
#endif
	bool is_forked;
	long fork_offset, fork_length;
};


/*
	Abstraction for loaded resources;
	this object will release that resource when it finishes.
	MacOS resource handles will be assumed to be locked.
*/
class LoadedResource
{
	// This class grabs a resource to be loaded into here
	friend class OpenedResourceFile;
	
public:
	// Resource loaded?
	bool IsLoaded();
	
	// Unloads the resource
	void Unload();
	
	// Get size of loaded resource
	size_t GetLength();
	
	// Get pointer (always present)
	void *GetPointer(bool DoDetach = false);

	// Make resource from raw resource data; the caller gives up ownership
	// of the pointed to memory block
	void SetData(void *data, size_t length);
	
	LoadedResource();
	~LoadedResource() {Unload();}	// Auto-unload when destroying

private:
	// Detaches an allocated resource from this object
	// (keep private to avoid memory leaks)
	void Detach();

public:
	// Platform-specific members
#if defined(mac)

	Handle GetHandle(bool DoDetach = false)
		{Handle ret = RsrcHandle; if (DoDetach) Detach(); return ret;}
#endif
#if defined(mac) && !defined(SDL_RFORK_HACK)
	// Gets some suitable default color table
	void GetSystemColorTable()
		{Unload(); RsrcHandle = Handle(GetCTable(8));}
#endif
#if defined(mac)
private:
	Handle RsrcHandle;	
#elif defined(SDL)

	void *p;		// Pointer to resource data (malloc()ed)
	size_t size;	// Size of data
#endif
};


/*
	Abstraction for opened resource files:
	it does opening, setting, and closing of such files;
	also getting "LoadedResource" objects that return pointers
*/
class OpenedResourceFile
{
	// This class will need to set the refnum and error value appropriately 
	friend class FileSpecifier;
	
public:
	
	// Pushing and popping the current file -- necessary in the MacOS version,
	// since resource forks are globally open with one of them the current top one.
	// Push() saves the earlier top one makes the current one the top one,
	// while Pop() restores the earlier top one.
	// Will leave SetResLoad in the state of true.
	bool Push();
	bool Pop();

	// Pushing and popping are unnecessary for the MacOS versions of Get() and Check()
	// Check simply checks if a resource is present; returns whether it is or not
	// Get loads a resource; returns whether or not one had been successfully loaded
	// CB: added functions that take 4 characters instead of uint32, which is more portable
	bool Check(uint32 Type, int16 ID);
	bool Check(uint8 t1, uint8 t2, uint8 t3, uint8 t4, int16 ID) {return Check(FOUR_CHARS_TO_INT(t1, t2, t3, t4), ID);}
	bool Get(uint32 Type, int16 ID, LoadedResource& Rsrc);
	bool Get(uint8 t1, uint8 t2, uint8 t3, uint8 t4, int16 ID, LoadedResource& Rsrc) {return Get(FOUR_CHARS_TO_INT(t1, t2, t3, t4), ID, Rsrc);}

	// Pushing and popping are unnecessary for the MacOS versions of these functions.
	// All these functions return a list of what they found as an STL vector
	// of the appropriate type; if they fail, they set the vector to zero length and return "false".
#if 0
	bool GetTypeList(vector<uint32>& TypeList);
	bool GetIDList(uint32 Type, vector<int16>& IDList);
	bool GetIDList(uint8 t1, uint8 t2, uint8 t3, uint8 t4, vector<int16>& IDList) {return GetIDList(FOUR_CHARS_TO_INT(t1, t2, t3, t4), IDList);}
#endif

	bool IsOpen();
	bool Close();
	
	OpenedResourceFile();
	~OpenedResourceFile() {Close();}	// Auto-close when destroying

	// Platform-specific members
#if defined(mac)

	short GetRefNum() {return RefNum;}
	OSErr GetError() {return Err;}

private:
	short RefNum;	// File reference number
	OSErr Err;		// Error code
	
	short SavedRefNum;
	bool isSavedRefRWOps;
	static bool isCurrentRefRWOps;

#elif defined(SDL)

	int GetError() {return err;}

private:
	int err;		// Error code
#endif
	SDL_RWops *f, *saved_f;
};


#ifdef mac
/*
	Abstraction for directory specifications;
	designed to encapsulate both directly-specified paths
	and MacOS volume/directory ID's.
*/
class DirectorySpecifier
{
	// This class needs to see directory info 
	friend class FileSpecifier;
	
	// MacOS directory specification
	long parID;
	short vRefNum;
	
	OSErr Err;
public:

	short Get_vRefNum() {return vRefNum;}
	long Get_parID() {return parID;}
	void Set_vRefNum(short _vRefNum) {vRefNum = _vRefNum;}
	void Set_parID(long _parID) {parID = _parID;}
	
	// Starts with the current root directory
	// Can go to subdirectory of subdirectory;
	// uses Unix-style path syntax (/ instead of : as the separator)
	bool SetToSubdirectory(const char *NameWithPath);
	
	// Set special directories:
	bool SetToAppParent();
	bool SetToPreferencesParent();
	
	DirectorySpecifier& operator=(DirectorySpecifier& D)
		{vRefNum = D.vRefNum; parID = D.parID; return *this;}
		
	OSErr GetError() {return Err;}
	
	DirectorySpecifier(): parID(0), vRefNum(0), Err(noErr) {}
	DirectorySpecifier(DirectorySpecifier& D) {*this = D;}
};
#else
// Directories are treated like files
#define DirectorySpecifier FileSpecifier
#endif


#ifdef SDL
// Directory entry, returned by FileSpecifier::ReadDirectory()
struct dir_entry {
	dir_entry() {}
	dir_entry(const string &n, long s, bool is_dir, bool is_vol = false)
		: name(n), size(s), is_directory(is_dir), is_volume(is_vol) {}
	dir_entry(const dir_entry &other)
		: name(other.name), size(other.size), is_directory(other.is_directory), is_volume(other.is_volume) {}
	~dir_entry() {}

	const dir_entry &operator=(const dir_entry &other)
	{
		if (this != &other) {
			name = other.name;
			size = other.size;
			is_directory = other.is_directory;
			is_volume = other.is_volume;
		}
		return *this;
	}

	bool operator<(const dir_entry &other) const
	{
		if (is_directory == other.is_directory)
			return name < other.name;
		else	// Sort directories before files
			return is_directory > other.is_directory;
	}

	string name;		// Entry name
	long size;			// File size (only valid if !is_directory)
	bool is_directory;	// Entry is a directory (plain file otherwise)
	bool is_volume;		// Entry is a volume (for platforms that have volumes, is_directory must also be set)
};
#endif


// Root-directory stuff; unnecessary for SDL?
#ifdef mac

// Useful for scenarios -- submit the subdirectory string.
// Originally the app's parent directory. To revert to that, submit an empty string.
// Returns the level of success.
bool Files_SetRootDirectory(const char *NameWithPath);

void Files_GetRootDirectory(DirectorySpecifier& Dir);

#endif


/*
	Abstraction for file specifications;
	designed to encapsulate both directly-specified paths
	and MacOS FSSpecs
*/
class FileSpecifier
{	
public:
	// The typecodes here are the symbolic constants defined in tags.h (_typecode_creator, etc.)
	
	// Get the name (final path element) as a C string:
	// assumes enough space to hold it if getting (max. 256 bytes)
	void GetName(char *Name) const;
	
	// MacOS:
	//   Sets the file specifier to the current root directory +
	//   the directories and name in "NameWithPath".
	//   Formerly used the object's own parent directory.
	// SDL:
	//   Looks in all directories in the current data search
	//   path for a file with the relative path "NameWithPath" and
	//   sets the file specifier to the full path of the first file
	//   found.
	// "NameWithPath" follows Unix-like syntax: <dirname>/<dirname>/<dirname>/filename
	// A ":" will be translated into a "/" in the MacOS.
	// Returns whether or not the setting was successful
	bool SetNameWithPath(const char *NameWithPath);

	// Move the directory specification
	void ToDirectory(DirectorySpecifier& Dir);
	void FromDirectory(DirectorySpecifier& Dir);

	// These functions take an appropriate one of the typecodes used earlier;
	// this is to try to cover the cases of both typecode attributes
	// and typecode suffixes.
	bool Create(Typecode Type);
	
	// Opens a file:
	bool Open(OpenedFile& OFile, bool Writable=false);
	
	// Opens either a MacOS resource fork or some imitation of it:
	bool Open(OpenedResourceFile& OFile, bool Writable=false);
	
	// These calls are for creating dialog boxes to set the filespec
	// A null pointer means an empty string
	bool ReadDialog(Typecode Type, const char *Prompt=NULL);
	bool WriteDialog(Typecode Type, const char *Prompt=NULL, const char *DefaultName=NULL);
	
	// Write dialog box for savegames (must be asynchronous, allowing the sound
	// to continue in the background)
	bool WriteDialogAsync(Typecode Type, char *Prompt=NULL, char *DefaultName=NULL);
	
	// Check on whether a file exists, and its type
	bool Exists();
#ifdef SDL
	bool IsDir();
#endif
	
	// Gets the modification date
	TimeType GetDate();
	
	// Returns _typecode_unknown if the type could not be identified;
	// the types returned are the _typecode_stuff in tags.h
	Typecode GetType();

	// How many bytes are free in the disk that the file lives in?
	bool GetFreeSpace(unsigned long& FreeSpace);
	
	// Copy file contents
	bool CopyContents(FileSpecifier& File);
	
	// Exchange contents with another filespec;
	// good for doing safe saves
	bool Exchange(FileSpecifier& File);
	
	// Delete file
	bool Delete();

	// Copy file specification
	const FileSpecifier &operator=(const FileSpecifier &other);
	
#ifdef SDL
	const char *GetPath(void) const {return name.c_str();}
#else
	const char *GetPath(void);
#endif

	// Platform-specific members
#ifdef mac

	FileSpecifier();
	FileSpecifier(const FileSpecifier& F) {*this = F;}
	bool operator==(const FileSpecifier& F);
	bool operator!=(const FileSpecifier& F) {return !(*this == F);}

	// Filespec management
	void SetSpec(FSSpec& _Spec);
	FSSpec& GetSpec() {return Spec;}
	
	// MacOS-friendly file creation
	bool Create(OSType TypeCode, OSType CreatorCode);
	
	// Replace the file name;
	// The typecode is for automatically adding a suffix;
	// NONE means add none
	void SetName(const char *Name, int Type);
	
	// Set special directories:
	bool SetToApp();
#if defined(TARGET_API_MAC_CARBON)
	bool SetParentToResources();	// OSX Resources directory: AlephOne.app/Contents/Resources
#endif
	bool SetParentToPreferences();

	// The error:
	OSErr GetError() {return Err;}
	
private:
	FSSpec Spec;
	OSErr Err;
	char GetPathStorage[1024];

#elif defined(SDL)

	FileSpecifier();
	FileSpecifier(const string &s) : name(s), err(0) {canonicalize_path();}
	FileSpecifier(const char *s) : name(s), err(0) {canonicalize_path();}
	FileSpecifier(const FileSpecifier &other) : name(other.name), err(other.err) {}

	bool operator==(const FileSpecifier &other) const {return name == other.name;}
	bool operator!=(const FileSpecifier &other) const {return name != other.name;}

	void SetToLocalDataDir();		// Per-user directory (for temporary files)
	void SetToPreferencesDir();		// Directory for preferences (per-user)
	void SetToSavedGamesDir();		// Directory for saved games (per-user)
	void SetToRecordingsDir();		// Directory for recordings (per-user)
	void SetToFirstDataDir();             // best guess at where maps might be

	void AddPart(const string &part);
	FileSpecifier &operator+=(const FileSpecifier &other) {AddPart(other.name); return *this;}
	FileSpecifier &operator+=(const string &part) {AddPart(part); return *this;}
	FileSpecifier &operator+=(const char *part) {AddPart(string(part)); return *this;}
	FileSpecifier operator+(const FileSpecifier &other) const {FileSpecifier a(name); a.AddPart(other.name); return a;}
	FileSpecifier operator+(const string &part) const {FileSpecifier a(name); a.AddPart(part); return a;}
	FileSpecifier operator+(const char *part) const {FileSpecifier a(name); a.AddPart(string(part)); return a;}

	void SplitPath(string &base, string &part) const;
	void SplitPath(DirectorySpecifier &base, string &part) const {string b; SplitPath(b, part); base = b;}

	bool CreateDirectory();
	bool ReadDirectory(vector<dir_entry> &vec);

	int GetError() const {return err;}

private:
	void canonicalize_path(void);

	string name;	// Path name
	int err;

#endif
};

#endif
#ifdef SDL_RFORK_HACK
#define SDL
#undef mac
#endif
