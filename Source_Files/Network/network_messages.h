/*
 *  network_messages.h - TCPMess message types for network game setup

	Copyright (C) 2005 and beyond by Gregory Smith
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

#ifndef NETWORK_MESSAGES_H
#define NETWORK_MESSAGES_H

#include "cseries.h"
#include "AStream.h"
#include "Message.h"

#include "SDL_net.h"

#include "network_capabilities.h"
#include "network_private.h"

enum {
  kHELLO_MESSAGE = 700,
  kJOINER_INFO_MESSAGE,
  kJOIN_PLAYER_MESSAGE,
  kCAPABILITIES_MESSAGE,
  kACCEPT_JOIN_MESSAGE,
  kTOPOLOGY_MESSAGE,
  kMAP_MESSAGE,
  kPHYSICS_MESSAGE,
  kLUA_MESSAGE,
  kCHAT_MESSAGE,
  kUNKNOWN_MESSAGE_MESSAGE,
  kEND_GAME_DATA_MESSAGE,
  kCHANGE_COLORS_MESSAGE,
  kSERVER_WARNING_MESSAGE,
  kCLIENT_INFO_MESSAGE
};

template <MessageTypeID tMessageType, typename tValueType>
class TemplatizedSimpleMessage : public SimpleMessage<tValueType>
{
 public:
  enum { kType = tMessageType };
  
  TemplatizedSimpleMessage() : SimpleMessage<tValueType>(tMessageType) { }

  TemplatizedSimpleMessage(tValueType inValue) : SimpleMessage<tValueType>(tMessageType, inValue) { }

  TemplatizedSimpleMessage<tMessageType, tValueType>* clone () const {
    return new TemplatizedSimpleMessage<tMessageType, tValueType>(*this);
  }
  
  MessageTypeID type() const { return kType; }
};

class AcceptJoinMessage : public SmallMessageHelper
{
 public:
  enum { kType = kACCEPT_JOIN_MESSAGE };
  
  AcceptJoinMessage() : SmallMessageHelper() { }
  
  AcceptJoinMessage(bool accepted, NetPlayer *player) : SmallMessageHelper() {
    mAccepted = accepted;
    mPlayer = *player;
  }

  AcceptJoinMessage *clone() const {
    return new AcceptJoinMessage(*this);
  }

  bool accepted() { return mAccepted; }
  void accepted(bool isAccepted) { mAccepted = isAccepted; }
  NetPlayer *player() { return &mPlayer; }
  void player(const NetPlayer *thePlayer) { mPlayer = *thePlayer; }
 
  MessageTypeID type() const { return kType; }

 protected:
  void reallyDeflateTo(AOStream& outputStream) const;
  bool reallyInflateFrom(AIStream& inputStream);
  
 private:
  
  bool mAccepted;
  NetPlayer mPlayer;
};

class CapabilitiesMessage : public SmallMessageHelper
{
 public:
  enum { kType = kCAPABILITIES_MESSAGE };

  CapabilitiesMessage() : SmallMessageHelper() { }
  
  CapabilitiesMessage(const Capabilities &capabilities) : SmallMessageHelper() {
    mCapabilities = capabilities;
  }
  
  CapabilitiesMessage *clone() const {
    return new CapabilitiesMessage(*this);
  }

  const Capabilities *capabilities() { return &mCapabilities; }

  MessageTypeID type() const { return kType; }

protected:
  void reallyDeflateTo(AOStream& outputStream) const;
  bool reallyInflateFrom(AIStream& inputStream);

private:
  Capabilities mCapabilities;
};
	  
class ChangeColorsMessage : public SmallMessageHelper
{
 public:
  enum { kType = kCHANGE_COLORS_MESSAGE };
  
  ChangeColorsMessage() : SmallMessageHelper() { }

  ChangeColorsMessage(int16 color, int16 team) : SmallMessageHelper() {
    mColor = color;
    mTeam = team;
  }

  ChangeColorsMessage *clone() const {
    return new ChangeColorsMessage(*this);
  }

  int16 color() { return mColor; }
  int16 team() { return mTeam; }

  MessageTypeID type() const { return kType; }

 protected:
  void reallyDeflateTo(AOStream& outputStream) const;
  bool reallyInflateFrom(AIStream& inputStream);

 private:
  
  int16 mColor;
  int16 mTeam;
};

class ClientInfoMessage : public SmallMessageHelper
{
public:
	enum { kType = kCLIENT_INFO_MESSAGE };
	enum { kAdd, kUpdate, kRemove };

	ClientInfoMessage() : SmallMessageHelper() { }

	ClientInfoMessage(int16 stream_id, const ClientChatInfo *clientChatInfo, int16 action ) : mStreamID(stream_id), mAction(action), mClientChatInfo(*clientChatInfo) { }

	ClientInfoMessage *clone() const {
		return new ClientInfoMessage(*this);
	}

	const ClientChatInfo *info() { return &mClientChatInfo; }
	const int16 action() { return mAction; }
	const int16 stream_id() { return mStreamID; }

	MessageTypeID type() const { return kType; }

protected:
	void reallyDeflateTo(AOStream& outputStream) const;
	bool reallyInflateFrom(AIStream& inputStream);

private:
	ClientChatInfo mClientChatInfo;
	int16 mAction;
	int16 mStreamID;
};
  

typedef DatalessMessage<kEND_GAME_DATA_MESSAGE> EndGameDataMessage;

class HelloMessage : public SmallMessageHelper
{
public:
  enum { kType = kHELLO_MESSAGE };
  
  HelloMessage() : SmallMessageHelper() { }

  HelloMessage(const std::string &version) : 
    SmallMessageHelper(), mVersion(version) { }
  
  HelloMessage *clone() const {
    return new HelloMessage(*this);
  }

  std::string version() { return mVersion; }
  void version(const std::string &version) { mVersion = version; }

  MessageTypeID type() const { return kType; }
  
protected:
  void reallyDeflateTo(AOStream& outputStream) const;
  bool reallyInflateFrom(AIStream& inputStream);

private:
  
  std::string mVersion;
};

typedef TemplatizedSimpleMessage<kJOIN_PLAYER_MESSAGE, int16> JoinPlayerMessage;
class JoinerInfoMessage : public SmallMessageHelper
{
public: 
	enum { kType = kJOINER_INFO_MESSAGE };
	
	JoinerInfoMessage() : SmallMessageHelper() { }
	
	JoinerInfoMessage(prospective_joiner_info *info, const std::string &version) : SmallMessageHelper() {
		
		mInfo = *info;
		mVersion = version;
	}
	
	JoinerInfoMessage *clone() const {
		return new JoinerInfoMessage(*this);
	}
	
	prospective_joiner_info *info() { return &mInfo; }
	void info(const prospective_joiner_info *theInfo) { 
		mInfo = *theInfo;
	}
	
	std::string version() { return mVersion; }
	void version(const std::string version) { mVersion = version; }
	
	MessageTypeID type() const { return kType; }
	
protected:
	void reallyDeflateTo(AOStream& outputStream) const;
	bool reallyInflateFrom(AIStream& inputStream);
	
private:
	prospective_joiner_info mInfo;
	std::string mVersion;
};

class LuaMessage : public BigChunkOfDataMessage
{
 public:
  enum { kType = kLUA_MESSAGE };

  LuaMessage(const Uint8* inBuffer = NULL, size_t inLength = 0) :
    BigChunkOfDataMessage(kType, inBuffer, inLength) { };

  LuaMessage(const LuaMessage& other) : BigChunkOfDataMessage(other) { }

  COVARIANT_RETURN(Message *, LuaMessage *) clone () const {
    return new LuaMessage(*this);
  }
};

class MapMessage : public BigChunkOfDataMessage
{
 public:
  enum { kType = kMAP_MESSAGE };
  
  MapMessage(const Uint8* inBuffer = NULL, size_t inLength = 0) : 
    BigChunkOfDataMessage(kType, inBuffer, inLength) { };

  MapMessage(const MapMessage& other) : BigChunkOfDataMessage(other) { }

  COVARIANT_RETURN(Message *, MapMessage *) clone () const {
    return new MapMessage(*this);
  }

};

class NetworkChatMessage : public SmallMessageHelper
{
 public:
  enum { kType = kCHAT_MESSAGE };
  enum { CHAT_MESSAGE_SIZE = 1024 };

  enum { kTargetPlayers = 0,
	 kTargetTeam = 1,
	 kTargetPlayer = 2,
	 kTargetClients = 3,
	 kTargetClient = 4 };

  NetworkChatMessage(const char *chatText = NULL, int16 senderID = 0,
		     int16 target = 0, int16 targetID = -1)
    : mSenderID(senderID), mTarget(target), mTargetID(targetID)
    {
      strncpy(mChatText, (chatText == NULL) ? "" : chatText, CHAT_MESSAGE_SIZE);
      mChatText[CHAT_MESSAGE_SIZE - 1] = '\0';
    }

  NetworkChatMessage * clone() const {
    return new NetworkChatMessage(*this);
  }

  MessageTypeID type() const { return kType; }
  const char *chatText() const { return mChatText; }
  int16 senderID() const { return mSenderID; }
  int16 target() const { return mTarget; }
  int16 targetID() const { return mTargetID; }

 protected:
  void reallyDeflateTo(AOStream& outputStream) const;
  bool reallyInflateFrom(AIStream& includeStream);

 private:
  char mChatText[CHAT_MESSAGE_SIZE];
  int16 mSenderID;
  int16 mTarget;
  int16 mTargetID;
};

class PhysicsMessage : public BigChunkOfDataMessage
{
 public:
  enum { kType = kPHYSICS_MESSAGE };
  
  PhysicsMessage(const Uint8* inBuffer = NULL, size_t inLength = 0) :
    BigChunkOfDataMessage(kType, inBuffer, inLength) { };

  PhysicsMessage(const PhysicsMessage& other) : BigChunkOfDataMessage(other) { }

  COVARIANT_RETURN(Message *, PhysicsMessage *) clone() const {
    return new PhysicsMessage(*this);
  }
  
};

class ServerWarningMessage : public SmallMessageHelper
{
public:
  enum { kType = kSERVER_WARNING_MESSAGE };
  enum { kMaxStringSize = 1024 };

  enum Reason { 
    kNoReason,
    kJoinerUngatherable 
  } ;

  ServerWarningMessage() { }
    
  ServerWarningMessage(std::string s, Reason reason) : 
    mString(s), mReason(reason) { 
    assert(s.length() <  kMaxStringSize); 
  }

  ServerWarningMessage *clone() const {
    return new ServerWarningMessage(*this);
  }

  MessageTypeID type() const { return kType; }
  void string (const std::string s) { 
    assert(s.length() < kMaxStringSize);
    mString = s; 
  }
  const std::string *string() { return &mString; }

protected:
  void reallyDeflateTo(AOStream& outputStream) const;
  bool reallyInflateFrom(AIStream& inputStream);

private:
  std::string mString;
  Reason mReason;
};


class TopologyMessage : public SmallMessageHelper
{
 public: 
  enum { kType = kTOPOLOGY_MESSAGE };
  
  TopologyMessage() : SmallMessageHelper() { }

  TopologyMessage(NetTopology *topology) : SmallMessageHelper() {
    mTopology = *topology;
  }

  TopologyMessage *clone() const {
    return new TopologyMessage(*this);
  }

  NetTopology *topology() { return &mTopology; }
  void topology(const NetTopology *theTopology) { mTopology = *theTopology; }
  
  MessageTypeID type() const { return kType; }

 protected:
  void reallyDeflateTo(AOStream& outputStream) const;
  bool reallyInflateFrom(AIStream& inputStream);
  
 private:
  NetTopology mTopology;
};

struct Client {
	Client(CommunicationsChannel *);
	enum {
		_connecting,
		_connected_but_not_yet_shown,
		_connected,
		_awaiting_capabilities,
		_ungatherable,
		_joiner_didnt_accept,
		_awaiting_accept_join,
		_awaiting_map,
		_ingame,
		_disconnect
	};

	CommunicationsChannel *channel;
	short state;
	uint16 network_version;
	Capabilities capabilities;
	unsigned char name[MAX_NET_PLAYER_NAME_LENGTH];

	static CheckPlayerProcPtr check_player;

	~Client();

	void drop();

	enum {
		_dont_warn_joiner = false,
		_warn_joiner = true
	};
	bool capabilities_indicate_player_is_gatherable(bool warn_joiner);
	bool can_pregame_chat() { return ((state == _connected) || (state == _connected_but_not_yet_shown) || (state == _ungatherable) || (state == _joiner_didnt_accept) || (state == _awaiting_accept_join) || (state == _awaiting_map)); }

	void handleJoinerInfoMessage(JoinerInfoMessage*, CommunicationsChannel*);
	void unexpectedMessageHandler(Message *, CommunicationsChannel*);
	void handleCapabilitiesMessage(CapabilitiesMessage*,CommunicationsChannel*);
	void handleAcceptJoinMessage(AcceptJoinMessage*, CommunicationsChannel*);
	void handleChatMessage(NetworkChatMessage*, CommunicationsChannel*);
	void handleChangeColorsMessage(ChangeColorsMessage*, CommunicationsChannel*);

	std::auto_ptr<MessageDispatcher> mDispatcher;
	std::auto_ptr<MessageHandler> mJoinerInfoMessageHandler;
	std::auto_ptr<MessageHandler> mUnexpectedMessageHandler;
	std::auto_ptr<MessageHandler> mCapabilitiesMessageHandler;
	std::auto_ptr<MessageHandler> mAcceptJoinMessageHandler;
	std::auto_ptr<MessageHandler> mChatMessageHandler;
	std::auto_ptr<MessageHandler> mChangeColorsMessageHandler;
};



#endif // NETWORK_MESSAGES_H
