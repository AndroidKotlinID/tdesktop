/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "base/timer.h"

class PeerData;

class ApiWrap;

namespace Data {

struct LastSpokeTimes {
	crl::time anything = 0;
	crl::time voice = 0;
};

struct GroupCallParticipant {
	not_null<PeerData*> peer;
	TimeId date = 0;
	TimeId lastActive = 0;
	uint64 raisedHandRating = 0;
	uint32 ssrc = 0;
	int volume = 0;
	bool applyVolumeFromMin = true;
	bool sounding = false;
	bool speaking = false;
	bool muted = false;
	bool mutedByMe = false;
	bool canSelfUnmute = false;
	bool onlyMinLoaded = false;
};

class GroupCall final {
public:
	GroupCall(not_null<PeerData*> peer, uint64 id, uint64 accessHash);
	~GroupCall();

	[[nodiscard]] uint64 id() const;
	[[nodiscard]] not_null<PeerData*> peer() const;
	[[nodiscard]] MTPInputGroupCall input() const;
	[[nodiscard]] QString title() const {
		return _title.current();
	}
	[[nodiscard]] rpl::producer<QString> titleValue() const {
		return _title.value();
	}
	void setTitle(const QString &title) {
		_title = title;
	}
	[[nodiscard]] TimeId recordStartDate() const {
		return _recordStartDate.current();
	}
	[[nodiscard]] rpl::producer<TimeId> recordStartDateValue() const {
		return _recordStartDate.value();
	}
	[[nodiscard]] rpl::producer<TimeId> recordStartDateChanges() const {
		return _recordStartDate.changes();
	}

	void setPeer(not_null<PeerData*> peer);

	using Participant = GroupCallParticipant;
	struct ParticipantUpdate {
		std::optional<Participant> was;
		std::optional<Participant> now;
	};

	static constexpr auto kSoundStatusKeptFor = crl::time(350);

	[[nodiscard]] auto participants() const
		-> const std::vector<Participant> &;
	void requestParticipants();
	[[nodiscard]] bool participantsLoaded() const;
	[[nodiscard]] PeerData *participantPeerBySsrc(uint32 ssrc) const;

	[[nodiscard]] rpl::producer<> participantsSliceAdded();
	[[nodiscard]] rpl::producer<ParticipantUpdate> participantUpdated() const;

	void applyUpdate(const MTPGroupCall &update);
	void applyUpdate(const MTPDupdateGroupCallParticipants &update);
	void applyUpdateChecked(
		const MTPDupdateGroupCallParticipants &update);
	void applyLastSpoke(uint32 ssrc, LastSpokeTimes when, crl::time now);
	void applyActiveUpdate(
		PeerId participantPeerId,
		LastSpokeTimes when,
		PeerData *participantPeerLoaded);

	void resolveParticipants(const base::flat_set<uint32> &ssrcs);

	[[nodiscard]] int fullCount() const;
	[[nodiscard]] rpl::producer<int> fullCountValue() const;

	void setInCall();
	void reload();

	void setJoinMutedLocally(bool muted);
	[[nodiscard]] bool joinMuted() const;
	[[nodiscard]] bool canChangeJoinMuted() const;

private:
	enum class ApplySliceSource {
		SliceLoaded,
		UnknownLoaded,
		UpdateReceived,
	};
	[[nodiscard]] ApiWrap &api() const;

	[[nodiscard]] bool inCall() const;
	void applyCall(const MTPGroupCall &call, bool force);
	void applyParticipantsSlice(
		const QVector<MTPGroupCallParticipant> &list,
		ApplySliceSource sliceSource);
	void requestUnknownParticipants();
	void changePeerEmptyCallFlag();
	void checkFinishSpeakingByActive();

	const uint64 _id = 0;
	const uint64 _accessHash = 0;

	not_null<PeerData*> _peer;
	int _version = 0;
	mtpRequestId _participantsRequestId = 0;
	mtpRequestId _reloadRequestId = 0;
	rpl::variable<QString> _title;

	std::vector<Participant> _participants;
	base::flat_map<uint32, not_null<PeerData*>> _participantPeerBySsrc;
	base::flat_map<not_null<PeerData*>, crl::time> _speakingByActiveFinishes;
	base::Timer _speakingByActiveFinishTimer;
	QString _nextOffset;
	rpl::variable<int> _fullCount = 0;
	rpl::variable<TimeId> _recordStartDate = 0;

	base::flat_map<uint32, LastSpokeTimes> _unknownSpokenSsrcs;
	base::flat_map<PeerId, LastSpokeTimes> _unknownSpokenPeerIds;
	mtpRequestId _unknownParticipantPeersRequestId = 0;

	rpl::event_stream<ParticipantUpdate> _participantUpdates;
	rpl::event_stream<> _participantsSliceAdded;

	bool _joinMuted = false;
	bool _canChangeJoinMuted = true;
	bool _allReceived = false;

};

} // namespace Data
