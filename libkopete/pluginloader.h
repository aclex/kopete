/*
    pluginloader.h - Kopete Plugin Loader

    Copyright (c) 2002 by Duncan Mac-Vicar Prett       <duncan@kde.org>

    Portions of this code based in Noatun plugin code:
    Copyright (c) 2000-2002 The Noatun Developers

    Kopete    (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#include <qdict.h>
#include <qmap.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qvaluelist.h>

#include <klibloader.h>

class KopeteProtocol;
class KopetePlugin;

struct KopeteLibraryInfo
{
	QString specfile;
	QString filename;
	QString author;
	QString license;
	QString type;
	QString site;
	QString email;
	QString name;
	QString comment;
};

bool operator ==(const KopeteLibraryInfo &, const KopeteLibraryInfo &);


/**
 * @author Duncan Mac-Vicar P. <duncan@kde.org>
 */
class LibraryLoader : public QObject
{
	Q_OBJECT

private:
	struct PluginLibrary
	{
		KopetePlugin *plugin;
		KLibrary *library;
	};

public:
	/**
	 * Retrieve the plugin loader instance.
	 */
	static LibraryLoader* pluginLoader();

	~LibraryLoader();

	/**
	 * FIXME: These 6 methods are only used internally and by
	 *        pluginconfig. Fix that code and remove them or
	 *        make them private.
	 *
	 * This is needed for the Plugin-List-View
	 * to see what plugins are required to show
	 * (when required by another noatun-plugin)
	 */
	KopeteLibraryInfo getInfo(const QString &spec) const;
	QValueList<KopeteLibraryInfo> available() const;
	QValueList<KopeteLibraryInfo> loaded() const;
	void add(const QString &spec);
	bool isLoaded(const QString &spec) const;
	void setModules(const QStringList &mods);

	/**
	 * Search by name
	 * ex: "ICQ"
	 */
	KopetePlugin *searchByName(const QString&);

	/**
	 * loads all the enabled plugins
	 */
	bool loadAll();

	/**
	 * unload the plugin specified by spec
	 */
	bool remove(const QString &spec);
	/**
	 * unload the plugin that is plugin
	 */
	bool remove(const LibraryLoader::PluginLibrary *plugin);
	bool remove(const KopetePlugin *plugin);

	QPtrList<KopetePlugin> plugins() const;

	/**
	 * Return all registered address book fields for a given plugin.
	 *
	 * Returns an empty QStringList if the plugin is invalid.
	 */
	QStringList addressBookFields( KopetePlugin *p ) const;

signals:
	void pluginLoaded(KopetePlugin *);

private slots:
	/**
	 * Cleanup some references if the plugin is destroyed
	 */
	void slotPluginDestroyed( QObject *o );

private:
	LibraryLoader();

	bool loadSO(const QString &spec);
	void removeNow(const QString &spec);

	QDict<LibraryLoader::PluginLibrary> mLibHash;

	/**
	 * The list of all address book keys used by each plugin
	 */
	QMap<KopetePlugin *, QStringList> m_addressBookFields;

	/**
	 * A cache for plugin info, to avoid reparsing (and hence mutable)
	 */
	mutable QMap<QString, KopeteLibraryInfo> m_cachedInfo;

	static LibraryLoader *s_pluginLoader;
};

#endif

// vim: set noet ts=4 sts=4 sw=4:

