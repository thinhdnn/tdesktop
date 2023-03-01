﻿/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "core/changelogs.h"

#include "lang/lang_keys.h"
#include "core/application.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "storage/storage_domain.h"
#include "data/data_session.h"
#include "base/qt/qt_common_adapters.h"
#include "mainwindow.h"
#include "apiwrap.h"

namespace Core {
namespace {

std::map<int, const char*> BetaLogs() {
	return {
	{
		1000064,
		"- Allow wide range of interface scale options.\n"

		"- Show opened chat name in the window title.\n"

		"- Bug fixes and other minor improvements.\n"

		"- Fix updating on macOS older than 10.14.\n"

		"- Try enabling non-fractional scale "
		"High DPI support on Windows and Linux.\n"

		"- Experimental setting for fractional scale "
		"High DPI support on Windows and Linux.\n"

		"- Fix navigation to bottom problems in groups you didn't join.\n"

		"- Fix a crash in chat export settings changes.\n"

		"- Fix a crash in sending some of JPEG images.\n"

		"- Fix CJK fonts on Windows.\n"

		"- Fix glitches after moving window to another screen.\n",
	},
	{
		1000065,
		"- Allow opening another account in a new window "
		"(see Settings > Advanced > Experimental Settings).\n"

		"- A lot of bugfixes for working with more than one window.\n"
	},
	{
		1000071,
		"- Allow media viewer to exit fullscreen and become a normal window."
	},
	{
		4006006,
		"- Confirmation window before starting a call.\n"

		"- New \"Battery and Animations\" settings section.\n"

		"- \n""Save Power on Low Battery\n"" option for laptops.\n"

		"- Improved windowed mode support for media viewer.\n"

		"- Hardware accelerated video playback fix on macOS.\n"
		
		"- New application icon on macOS following the system guidelines.\n"
	}
	};
};

} // namespace

Changelogs::Changelogs(not_null<Main::Session*> session, int oldVersion)
: _session(session)
, _oldVersion(oldVersion) {
	_session->data().chatsListChanges(
	) | rpl::filter([](Data::Folder *folder) {
		return !folder;
	}) | rpl::start_with_next([=] {
		requestCloudLogs();
	}, _chatsSubscription);
}

std::unique_ptr<Changelogs> Changelogs::Create(
		not_null<Main::Session*> session) {
	auto &local = Core::App().domain().local();
	const auto oldVersion = local.oldVersion();
	local.clearOldVersion();
	return (oldVersion > 0 && oldVersion < AppVersion)
		? std::make_unique<Changelogs>(session, oldVersion)
		: nullptr;
}

void Changelogs::requestCloudLogs() {
	_chatsSubscription.destroy();
//
//	const auto callback = [this](const MTPUpdates &result) {
//		_session->api().applyUpdates(result);
//
//		auto resultEmpty = true;
//		switch (result.type()) {
//		case mtpc_updateShortMessage:
//		case mtpc_updateShortChatMessage:
//		case mtpc_updateShort:
//			resultEmpty = false;
//			break;
//		case mtpc_updatesCombined:
//			resultEmpty = result.c_updatesCombined().vupdates().v.isEmpty();
//			break;
//		case mtpc_updates:
//			resultEmpty = result.c_updates().vupdates().v.isEmpty();
//			break;
//		case mtpc_updatesTooLong:
//		case mtpc_updateShortSentMessage:
//			LOG(("API Error: Bad updates type in app changelog."));
//			break;
//		}
//		if (resultEmpty) {
//			addLocalLogs();
//		}
//	};
//	_session->api().requestChangelog(
//		FormatVersionPrecise(_oldVersion),
//		crl::guard(this, callback));
    addLocalLogs();
}

void Changelogs::addLocalLogs() {
	//if (AppBetaVersion || cAlphaVersion()) {
	//	addBetaLogs();
	//}
	if (!_addedSomeLocal) {
		//const auto text = tr::lng_new_version_wrap2(
		//	tr::now,
		//	lt_version,
		//	QString::fromLatin1(AppVersionStr),
		//	lt_changes,
		//	tr::lng_new_version_minor(tr::now),
		//	lt_link,
		//	Core::App().changelogLink());
		//addLocalLog(text.trimmed());
		addBetaLogs();
	}
}

void Changelogs::addLocalLog(const QString &text) {
	auto textWithEntities = TextWithEntities{ text };
	TextUtilities::ParseEntities(textWithEntities, TextParseLinks);
	_session->data().serviceNotification(textWithEntities);
	_addedSomeLocal = true;
};

void Changelogs::addBetaLogs() {
	for (const auto &[version, changes] : BetaLogs()) {
		addBetaLog(version, changes);
	}
}

void Changelogs::addBetaLog(int changeVersion, const char *changes) {
	if (_oldVersion >= changeVersion) {
		return;
	}
	const auto text = [&] {
		static const auto simple = u"\n- "_q;
		static const auto separator = QString::fromUtf8("\n\xE2\x80\xA2 ");
		auto result = QString::fromUtf8(changes).trimmed();
		if (result.startsWith(base::StringViewMid(simple, 1))) {
			result = separator.mid(1) + result.mid(simple.size() - 1);
		}
		return result.replace(simple, separator);
	}();
	const auto version = FormatVersionDisplay(changeVersion);
	const auto log = u"New in version %1%2:\n\n"_q.arg(version).arg(AppBetaVersion ? " beta" : "") + text;
	addLocalLog(log);
}

QString FormatVersionDisplay(int version) {
	return QString::number(version / 1000000)
		+ '.' + QString::number((version % 1000000) / 1000)
		+ ((version % 1000)
			? ('.' + QString::number(version % 1000))
			: QString());
}

QString FormatVersionPrecise(int version) {
	return QString::number(version / 1000000)
		+ '.' + QString::number((version % 1000000) / 1000)
		+ '.' + QString::number(version % 1000);
}

} // namespace Core
