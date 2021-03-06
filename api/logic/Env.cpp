#include "Env.h"
#include "net/HttpMetaCache.h"
#include "BaseVersion.h"
#include "BaseVersionList.h"
#include <QDir>
#include <QCoreApplication>
#include <QNetworkProxy>
#include <QNetworkAccessManager>
#include <QDebug>
#include "tasks/Task.h"
#include "meta/Index.h"
#include "FileSystem.h"
#include <QDebug>


class Env::Private
{
public:
	QNetworkAccessManager m_qnam;
	shared_qobject_ptr<HttpMetaCache> m_metacache;
	std::shared_ptr<IIconList> m_iconlist;
	QMap<QString, std::shared_ptr<BaseVersionList>> m_versionLists;
	shared_qobject_ptr<Meta::Index> m_metadataIndex;
	QString m_jarsPath;
};

static Env * instance;

/*
 * The *NEW* global rat nest of an object. Handle with care.
 */

Env::Env()
{
	d = new Private();
}

Env::~Env()
{
	delete d;
}

Env& Env::Env::getInstance()
{
	if(!instance)
	{
		instance = new Env();
	}
	return *instance;
}

void Env::dispose()
{
	delete instance;
	instance = nullptr;
}

shared_qobject_ptr< HttpMetaCache > Env::metacache()
{
	return d->m_metacache;
}

QNetworkAccessManager& Env::qnam() const
{
	return d->m_qnam;
}

std::shared_ptr<IIconList> Env::icons()
{
	return d->m_iconlist;
}

void Env::registerIconList(std::shared_ptr<IIconList> iconlist)
{
	d->m_iconlist = iconlist;
}

BaseVersionPtr Env::getVersion(QString component, QString version)
{
	auto list = getVersionList(component);
	if(!list)
	{
		return nullptr;
	}
	return list->findVersion(version);
}

std::shared_ptr< BaseVersionList > Env::getVersionList(QString component)
{
	auto iter = d->m_versionLists.find(component);
	if(iter != d->m_versionLists.end())
	{
		return *iter;
	}
	//return std::make_shared<NullVersionList>();
	return nullptr;
}

void Env::registerVersionList(QString name, std::shared_ptr< BaseVersionList > vlist)
{
	d->m_versionLists[name] = vlist;
}

shared_qobject_ptr<Meta::Index> Env::metadataIndex()
{
	if (!d->m_metadataIndex)
	{
		d->m_metadataIndex.reset(new Meta::Index());
	}
	return d->m_metadataIndex;
}


void Env::initHttpMetaCache()
{
	auto &m_metacache = d->m_metacache;
	m_metacache.reset(new HttpMetaCache("metacache"));
	m_metacache->addBase("asset_indexes", QDir("assets/indexes").absolutePath());
	m_metacache->addBase("asset_objects", QDir("assets/objects").absolutePath());
	m_metacache->addBase("versions", QDir("versions").absolutePath());
	m_metacache->addBase("libraries", QDir("libraries").absolutePath());
	m_metacache->addBase("minecraftforge", QDir("mods/minecraftforge").absolutePath());
	m_metacache->addBase("fmllibs", QDir("mods/minecraftforge/libs").absolutePath());
	m_metacache->addBase("liteloader", QDir("mods/liteloader").absolutePath());
	m_metacache->addBase("general", QDir("cache").absolutePath());
	m_metacache->addBase("skins", QDir("accounts/skins").absolutePath());
	m_metacache->addBase("root", QDir::currentPath());
	m_metacache->addBase("translations", QDir("translations").absolutePath());
	m_metacache->addBase("icons", QDir("cache/icons").absolutePath());
	m_metacache->addBase("meta", QDir("meta").absolutePath());
	m_metacache->Load();
}

void Env::updateProxySettings(QString proxyTypeStr, QString addr, int port, QString user, QString password)
{
	// Set the application proxy settings.
	if (proxyTypeStr == "SOCKS5")
	{
		QNetworkProxy::setApplicationProxy(
			QNetworkProxy(QNetworkProxy::Socks5Proxy, addr, port, user, password));
	}
	else if (proxyTypeStr == "HTTP")
	{
		QNetworkProxy::setApplicationProxy(
			QNetworkProxy(QNetworkProxy::HttpProxy, addr, port, user, password));
	}
	else if (proxyTypeStr == "None")
	{
		// If we have no proxy set, set no proxy and return.
		QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
	}
	else
	{
		// If we have "Default" selected, set Qt to use the system proxy settings.
		QNetworkProxyFactory::setUseSystemConfiguration(true);
	}

	qDebug() << "Detecting proxy settings...";
	QNetworkProxy proxy = QNetworkProxy::applicationProxy();
	d->m_qnam.setProxy(proxy);
	QString proxyDesc;
	if (proxy.type() == QNetworkProxy::NoProxy)
	{
		qDebug() << "Using no proxy is an option!";
		return;
	}
	switch (proxy.type())
	{
	case QNetworkProxy::DefaultProxy:
		proxyDesc = "Default proxy: ";
		break;
	case QNetworkProxy::Socks5Proxy:
		proxyDesc = "Socks5 proxy: ";
		break;
	case QNetworkProxy::HttpProxy:
		proxyDesc = "HTTP proxy: ";
		break;
	case QNetworkProxy::HttpCachingProxy:
		proxyDesc = "HTTP caching: ";
		break;
	case QNetworkProxy::FtpCachingProxy:
		proxyDesc = "FTP caching: ";
		break;
	default:
		proxyDesc = "DERP proxy: ";
		break;
	}
	proxyDesc += QString("%3@%1:%2 pass %4")
					 .arg(proxy.hostName())
					 .arg(proxy.port())
					 .arg(proxy.user())
					 .arg(proxy.password());
	qDebug() << proxyDesc;
}

QString Env::getJarsPath()
{
	if(d->m_jarsPath.isEmpty())
	{
		return FS::PathCombine(QCoreApplication::applicationDirPath(), "jars");
	}
	return d->m_jarsPath;
}

void Env::setJarsPath(const QString& path)
{
	d->m_jarsPath = path;
}

#include "Env.moc"
