/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <http://kde.maniatek.com/audex>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "playlist.h"

Playlist::Playlist()
{

}

Playlist::Playlist(const QByteArray& playlist)
{

  addPlaylist(playlist);

}

Playlist::~Playlist() {

}

void Playlist::addPlaylist(const QByteArray& playlist)
{

  QString format = p_playlist_format(playlist);

  if (format == "m3u")
  {
    p_add_M3U(playlist);
  }
  else if (format == "pls")
  {
    p_add_PLS(playlist);
  }
  else if (format == "xspf")
  {
    p_add_XSPF(playlist);
  }

}

void Playlist::clear()
{

  p_playlist.clear();

}

void Playlist::appendItem(const PlaylistItem& item)
{

  p_playlist.append(item);

}

QByteArray Playlist::toM3U(const QString& playlistPath, const bool utf8) const {

  QStringList playlist;
  playlist.append("#EXTM3U");

  for (int i = 0; i < p_playlist.count(); ++i)
  {

    PlaylistItem pi = p_playlist[i];
    if (pi.filename().isEmpty()) continue;

    if (!pi.artist().isEmpty())
    {
      playlist.append(QString("#EXTINF:%1,%2 - %3").arg(pi.length()).arg(pi.artist()).arg(pi.title()));
    }
    else
    {
      playlist.append(QString("#EXTINF:%1,%2").arg(pi.length()).arg(pi.title()));
    }
    if (!playlistPath.isEmpty())
    {
      QDir dir(playlistPath);
      playlist.append(dir.relativeFilePath(pi.filename()));
    }
    else
    {
      playlist.append(pi.filename());
    }

  }

  if (utf8)
    return playlist.join("\n").append("\n").toUtf8();
  else
    return playlist.join("\n").append("\n").toLatin1();

}

QByteArray Playlist::toPLS(const QString& playlistPath, const bool utf8) const
{

  QStringList playlist;
  playlist.append("[Playlist]");

  int j = 0;
  for (int i = 0; i < p_playlist.count(); ++i)
  {
    PlaylistItem pi = p_playlist[i];

    if (pi.filename().isEmpty()) continue;
    ++j;

    if (!playlistPath.isEmpty())
    {
      QDir dir(playlistPath);
      playlist.append(QString("File%1=%2").arg(i+1).arg(dir.relativeFilePath(pi.filename())));
    }
    else
    {
      playlist.append(QString("File%1=%2").arg(i+1).arg(pi.filename()));
    }

    if (!pi.artist().isEmpty())
    {
      playlist.append(QString("Title%1=%2 - %3").arg(i+1).arg(pi.artist()).arg(pi.title()));
    }
    else
    {
      playlist.append(QString("Title%1=%2").arg(i+1).arg(pi.title()));
    }

    playlist.append(QString("Length%1=%2").arg(i+1).arg(pi.length()));

  }

  playlist.append(QString("NumberOfEntries=%1").arg(j));
  playlist.append(QString("Version=2"));

  if (utf8)
    return playlist.join("\n").append("\n").toUtf8();
  else
    return playlist.join("\n").append("\n").toLatin1();

}

QByteArray Playlist::toXSPF() const
{

  QDomDocument doc;
  QDomElement root = doc.createElement("playlist");
  root.setAttribute("version", "1");
  root.setAttribute("xmlns", "http://xspf.org/ns/0");

  QDomElement creator = doc.createElement("creator");
  QDomText text = doc.createTextNode("audex");
  creator.appendChild(text);
  root.appendChild(creator);

  QDomElement tracklist = doc.createElement("trackList");

  int j = 0;
  for (int i = 0; i < p_playlist.count(); ++i)
  {

    PlaylistItem pi = p_playlist[i];

    if (pi.filename().isEmpty()) continue;
    ++j;

    QDomElement track = doc.createElement("track");

    QDomElement ch = doc.createElement("location");
    QDomText text = doc.createTextNode(pi.filename());

    ch.appendChild(text);
    track.appendChild(ch);

    if (!pi.artist().isEmpty())
    {
      ch = doc.createElement("creator");
      text = doc.createTextNode(pi.artist());
      ch.appendChild(text);
      track.appendChild(ch);
    }

    ch = doc.createElement("title");
    text = doc.createTextNode(pi.title());
    ch.appendChild(text);
    track.appendChild(ch);

    ch = doc.createElement("trackNum");
    text = doc.createTextNode(QString::number(j));
    ch.appendChild(text);
    track.appendChild(ch);

    if (pi.length() > 0)
    {
      ch = doc.createElement("duration");
      text = doc.createTextNode(QString::number(pi.length()*1000));
      ch.appendChild(text);
      track.appendChild(ch);
    }

    tracklist.appendChild(track);

  }

  root.appendChild(tracklist);
  doc.appendChild(root);
  QByteArray xml_header("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  return doc.toByteArray().prepend(xml_header);

}

const QString Playlist::p_playlist_format(const QByteArray& playlist)
{

  if (playlist.contains("#EXTM3U") || playlist.contains("#EXTINF")) return "m3u";
  if (playlist.toLower().contains("[playlist]")) return "pls";
  if (playlist.contains("<playlist") && playlist.contains("<trackList")) return "xspf";

  return QString();

}

void Playlist::p_add_M3U(const QByteArray& playlist)
{

  QTextStream stream(playlist, QIODevice::ReadOnly);

  QString line = stream.readLine().trimmed();

  bool extended = false;
  if (line.startsWith("#EXTM3U"))
  {
    extended = true;
    line = stream.readLine().trimmed();
  }

  PlaylistItem pi;
  while (!line.isNull())
  {

    if (line.startsWith('#'))
    {
      if (extended && line.startsWith("#EXT"))
      {
        pi = p_parse_m3u_metadata_line(line);
      }
    }
    else if (!line.isEmpty())
    {
      pi.setFilename(line);
      if (!pi.filename().isEmpty())
      {
        p_playlist.append(pi);
        pi.clear();
      }
    }

    line = stream.readLine().trimmed();

  }

}

void Playlist::p_add_PLS(const QByteArray& playlist)
{

  QTextStream stream(playlist, QIODevice::ReadOnly);

  QString line = stream.readLine().trimmed();

  QMap<int, PlaylistItem> items;
  while (!line.isNull())
  {

    int equals = line.indexOf('=');
    QString key = line.left(equals).toLower();
    QString value = line.mid(equals + 1);

    QRegExp n_re("\\d+$");
    n_re.indexIn(key);
    int n = n_re.cap(0).toInt();

    if (key.startsWith("file"))
    {
      items[n].setFilename(value);
    }
    else if (key.startsWith("title"))
    {
      items[n].setTitle(value);
    }
    else if (key.startsWith("length"))
    {
      bool ok;
      int seconds = value.toInt(&ok);
      if (ok) items[n].setLength(seconds);
    }

    line = stream.readLine().trimmed();
  }

  QMap<int, PlaylistItem>::const_iterator i = items.constBegin();
  while (i != items.constEnd())
  {
    p_playlist.append(i.value());
    ++i;
  }

}

void Playlist::p_add_XSPF(const QByteArray& playlist)
{

  QDomDocument doc;
  QString errorMsg;
  int errorCol;
  int errorRow;

  if (!doc.setContent(QString(playlist), &errorMsg, &errorRow, &errorCol)) return;

  QDomElement rootElement = doc.firstChildElement("playlist");
  if (rootElement.isNull()) return;

  QDomElement tracklistElement = rootElement.firstChildElement("trackList");
  if (tracklistElement.isNull()) return;

  QMap<int, PlaylistItem> items;
  QMap<int, PlaylistItem> items_no_tracknum;
  QDomElement child = tracklistElement.firstChildElement("track");
  int m = 0;
  while (!child.isNull())
  {

    bool ok;
    int n = child.firstChildElement("trackNum").text().toInt(&ok);
    if (!ok)
    {
      items_no_tracknum[m].setFilename(child.firstChildElement("location").text());
      items_no_tracknum[m].setArtist(child.firstChildElement("creator").text());
      items_no_tracknum[m].setTitle(child.firstChildElement("title").text());
      items_no_tracknum[m].setLength(child.firstChildElement("duration").text().toInt() / 1000);
      ++m;
    }
    else
    {
      items[n].setFilename(child.firstChildElement("location").text());
      items[n].setArtist(child.firstChildElement("creator").text());
      items[n].setTitle(child.firstChildElement("title").text());
      items[n].setLength(child.firstChildElement("duration").text().toInt() / 1000);
    }

    child = child.nextSiblingElement();
  }

  QMap<int, PlaylistItem>::const_iterator i = items.constBegin();
  while (i != items.constEnd())
  {
    p_playlist.append(i.value());
    ++i;
  }
  QMap<int, PlaylistItem>::const_iterator j = items_no_tracknum.constBegin();
  while (j != items_no_tracknum.constEnd())
  {
    p_playlist.append(j.value());
    ++j;
  }

}

const PlaylistItem Playlist::p_parse_m3u_metadata_line(const QString& line)
{

  PlaylistItem pi;

  QString info = line.section(':', 1);
  QString l = info.section(',', 0, 0);
  bool ok;
  int length = l.toInt(&ok);
  if (!ok) return pi;
  pi.setLength(length);

  QString track_info = info.section(',', 1);
  QStringList list = track_info.split('-');
  if (list.length() <= 1)
  {
    pi.setTitle(track_info);
    return pi;
  }
  pi.setArtist(list[0].trimmed());
  pi.setTitle(list[1].trimmed());
  return pi;

}
