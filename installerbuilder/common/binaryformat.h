/**************************************************************************
**
** This file is part of Qt SDK**
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).*
**
** Contact:  Nokia Corporation qt-info@nokia.com**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception version
** 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you are unsure which license is appropriate for your use, please contact
** (qt-info@nokia.com).
**
**************************************************************************/
#ifndef BINARYFORMAT_H
#define BINARYFORMAT_H

#include "range.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QHash>
#include <QIODevice>
#include <QStack>
#include <QString>
#include <QSharedPointer>

#include "binaryformatenginehandler.h"
#include "installer_global.h"

class KD7zEngineHandler;

namespace KDUpdater
{
    class UpdateOperation;
}

namespace QInstaller
{
    qint64 INSTALLER_EXPORT findMagicCookie( QFile* file );

    void INSTALLER_EXPORT appendFileData(QIODevice *out, QIODevice *in);
    void INSTALLER_EXPORT appendInt64( QIODevice* out, qint64 n );
    void INSTALLER_EXPORT appendInt64Range( QIODevice* out, const Range<qint64>& r );
    void INSTALLER_EXPORT appendData(QIODevice *out, QIODevice *in, qint64 size);
    void INSTALLER_EXPORT appendByteArray(QIODevice *out, const QByteArray &ba);
    void INSTALLER_EXPORT appendString(QIODevice *out, const QString &str);
    void INSTALLER_EXPORT appendStringList(QIODevice *out, const QStringList &list);
    void INSTALLER_EXPORT appendDictionary(QIODevice *out, const QHash<QString,QString>& dict);

    void INSTALLER_EXPORT retrieveFileData(QIODevice *out, QIODevice *in);
    qint64 INSTALLER_EXPORT retrieveInt64( QIODevice* in );
    Range<qint64> INSTALLER_EXPORT retrieveInt64Range( QIODevice* in );
    QByteArray INSTALLER_EXPORT retrieveByteArray(QIODevice *in);
    QString INSTALLER_EXPORT retrieveString(QIODevice *in);
    QStringList INSTALLER_EXPORT retrieveStringList(QIODevice *in);
    QHash<QString,QString> INSTALLER_EXPORT retrieveDictionary(QIODevice *in);

    static const qint64 MagicInstallerMarker      =  0x12023233UL;
    static const qint64 MagicUninstallerMarker    =  0x12023234UL;

    static const qint64 MagicUpdaterMarker        =  0x12023235UL;
    static const qint64 MagicPackageManagerMarker =  0x12023236UL;

    // this cookie is put at the end of the file to determine wheter we have data
    static const quint64 MagicCookie = 0xc2630a1c99d668f8LL;

}

namespace QInstallerCreator 
{
    class Component;

    class INSTALLER_EXPORT Archive : public QIODevice
    {
        Q_OBJECT
    public:
        explicit Archive( const QString& path );
        Archive( const QByteArray& name, const QByteArray& data );
        Archive( const QByteArray& name, QIODevice* device, const Range< qint64 >& segment );
        ~Archive();

        bool open( OpenMode mode );
        void close();

        bool seek( qint64 pos );
        qint64 size() const;

        bool createZippedFile();
        bool isZippedDirectory() const;
        bool copy( const QString& name );

        QByteArray name() const;
        void setName( const QByteArray& name );

    protected:
        qint64 readData( char* data, qint64 maxSize );
        qint64 writeData( const char* data, qint64 maxSize );

        Range< qint64 > binarySegment() const;

    private:
        //used when when reading from the installer
        QIODevice* const m_device;
        const Range<qint64> m_segment;

         //used when creating the installer, archive input file
        QFile m_inputFile;
        const bool m_isTempFile;
        const QString m_path;
        QByteArray m_name;
    };

    class INSTALLER_EXPORT Component 
    {
        Q_DECLARE_TR_FUNCTIONS(Component);
    public:
        virtual ~Component();

        static Component readFromIndexEntry( QIODevice* dev, qint64 offset );
        void writeIndexEntry( QIODevice* dev, qint64 offset ) const;

        void writeData( QIODevice* dev, qint64 positionOffset ) const;
        void readData( QIODevice* dev, qint64 offset );

        QByteArray name() const;
        void setName( const QByteArray& ba );

        QString dataDirectory() const;
        void setDataDirectory( const QString& path );

        Range<qint64> binarySegment() const;
        void setBinarySegment( const Range<qint64>& r );

        void appendArchive( const QSharedPointer<Archive>& archive );
        QSharedPointer<Archive> archiveByName( const QByteArray& name ) const;
        QVector< QSharedPointer<Archive> > archives() const;

        bool operator<( const Component& other ) const;
        bool operator==( const Component& other ) const;

    private:
        QByteArray m_name;
        QVector< QSharedPointer< Archive > > m_archives;
        mutable Range<qint64> m_binarySegment;
        QString m_dataDirectory;
    };


    class INSTALLER_EXPORT ComponentIndex 
    {
    public:
        ComponentIndex();
        static ComponentIndex read( QIODevice* dev, qint64 offset );
        void writeIndex( QIODevice* dev, qint64 offset ) const;
        void writeComponentData( QIODevice* dev, qint64 offset ) const;
        Component componentByName( const QByteArray& ) const;
        void insertComponent( const Component& );
        void removeComponent( const QByteArray& name );
        QVector<Component> components() const;
        int componentCount() const;

    protected:
        static KD7zEngineHandler* zipHandler;

    private:
        QHash<QByteArray,Component> m_components;
    };
}

namespace QInstaller
{
    class INSTALLER_EXPORT BinaryContent
    {
        explicit BinaryContent( const QString& path  );

    public:
        virtual ~BinaryContent();

        static BinaryContent readFromApplicationFile();
        static BinaryContent readFromBinary( const QString& path );

        int registerEmbeddedQResources();

        QSharedPointer<QFile> file;
        QInstallerCreator::ComponentIndex components;
        QInstallerCreator::BinaryFormatEngineHandler handler;
        QVector< Range< qint64 > > metadataResourceSegments;
        QVector< const uchar* > mappings;
        qint64 magicmaker;
        qint64 dataBlockStart;
        QStack<KDUpdater::UpdateOperation*> performedOperations;
    };
}

#endif // BINARYFORMAT_H
