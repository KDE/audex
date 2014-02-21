/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2013 Marco Nelles (audex@maniatek.com)
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

#include "encoderassistant.h"

long makeVersionNumber(int major, int minor, int patch)
{
    long versionNumber=   ((major & 0xFF) << 16)
                        | ((minor & 0xFF) << 8)
                        | (patch & 0xFF);
    return versionNumber;
}

const QString EncoderAssistant::name(const EncoderAssistant::Encoder encoder) {

  switch (encoder) {

    case EncoderAssistant::LAME : return ENCODER_LAME_NAME;
    case EncoderAssistant::OGGENC : return ENCODER_OGGENC_NAME;
    case EncoderAssistant::FLAC : return ENCODER_FLAC_NAME;
    case EncoderAssistant::FAAC : return ENCODER_FAAC_NAME;
    case EncoderAssistant::WAVE : return ENCODER_WAVE_NAME;
    case EncoderAssistant::CUSTOM : return ENCODER_CUSTOM_NAME;
    default : return "";

  }

  return "";

}

const QString EncoderAssistant::encoderName(const Encoder encoder) {

  switch (encoder) {

    case EncoderAssistant::LAME : return ENCODER_LAME_ENCODER_NAME;
    case EncoderAssistant::OGGENC : return ENCODER_OGGENC_ENCODER_NAME;
    case EncoderAssistant::FLAC : return ENCODER_FLAC_ENCODER_NAME;
    case EncoderAssistant::FAAC : return ENCODER_FAAC_ENCODER_NAME;
    case EncoderAssistant::WAVE : return ENCODER_WAVE_ENCODER_NAME;
    case EncoderAssistant::CUSTOM : return ENCODER_CUSTOM_ENCODER_NAME;
    default : return "";

  }

  return "";

}

const QString EncoderAssistant::icon(const EncoderAssistant::Encoder encoder) {

  switch (encoder) {

    case EncoderAssistant::LAME : return ENCODER_LAME_ICON;
    case EncoderAssistant::OGGENC : return ENCODER_OGGENC_ICON;
    case EncoderAssistant::FLAC : return ENCODER_FLAC_ICON;
    case EncoderAssistant::FAAC : return ENCODER_FAAC_ICON;
    case EncoderAssistant::WAVE : return ENCODER_WAVE_ICON;
    case EncoderAssistant::CUSTOM : return ENCODER_CUSTOM_ICON;
    default : return "";

  }

  return "";

}

bool EncoderAssistant::available(const EncoderAssistant::Encoder encoder) {

  switch (encoder) {

    case EncoderAssistant::LAME : return (KProcess::execute(ENCODER_LAME_BIN, QStringList() << ENCODER_LAME_VERSION_PARA)==0);
    case EncoderAssistant::OGGENC : return (KProcess::execute(ENCODER_OGGENC_BIN, QStringList() << ENCODER_OGGENC_VERSION_PARA)==0);
    case EncoderAssistant::FLAC : return (KProcess::execute(ENCODER_FLAC_BIN, QStringList() << ENCODER_FLAC_VERSION_PARA)==0);
    case EncoderAssistant::FAAC : return (KProcess::execute(ENCODER_FAAC_BIN, QStringList() << ENCODER_FAAC_VERSION_PARA)==1);
    case EncoderAssistant::WAVE : return (KProcess::execute(ENCODER_WAVE_BIN, QStringList() << ENCODER_WAVE_VERSION_PARA)==0);
    case EncoderAssistant::CUSTOM : return TRUE;
    default : return FALSE;

  }

  return FALSE;

}

bool EncoderAssistant::canEmbedCover(const Encoder encoder, int *maxCoverSize) {

  switch (encoder) {

    case EncoderAssistant::LAME : if (maxCoverSize) *maxCoverSize = ENCODER_LAME_MAX_EMBED_COVER_SIZE; return TRUE;
    case EncoderAssistant::OGGENC :
    case EncoderAssistant::FLAC : return TRUE;
    case EncoderAssistant::FAAC :
    case EncoderAssistant::WAVE :
    case EncoderAssistant::CUSTOM :
    default : break;

  }

  if (maxCoverSize) *maxCoverSize = 0;

  return FALSE;

}

const QString EncoderAssistant::version(const EncoderAssistant::Encoder encoder) {

  KProcess process;
  process.setOutputChannelMode(KProcess::SeparateChannels);
  process.setReadChannel(KProcess::StandardError);
  switch (encoder) {
    case EncoderAssistant::LAME : process.setShellCommand(QString(ENCODER_LAME_BIN)+" "+QString(ENCODER_LAME_VERSION_PARA)); break;
    case EncoderAssistant::OGGENC : process.setShellCommand(QString(ENCODER_OGGENC_BIN)+" "+QString(ENCODER_OGGENC_VERSION_PARA)); break;
    case EncoderAssistant::FLAC : process.setShellCommand(QString(ENCODER_FLAC_BIN)+" "+QString(ENCODER_FLAC_VERSION_PARA)); break;
    case EncoderAssistant::FAAC : process.setShellCommand(QString(ENCODER_FAAC_BIN)+" "+QString(ENCODER_FAAC_VERSION_PARA)); break;
    case EncoderAssistant::WAVE : return "";
    case EncoderAssistant::CUSTOM : return "";
    default : return "";
  }
  process.start();
  if (!process.waitForFinished()) return "";
  QByteArray rawoutput = process.readAllStandardError();
  if (rawoutput.size() == 0) rawoutput = process.readAllStandardOutput();
  QString output(rawoutput);
  QStringList list = output.trimmed().split("\n");
  if (list.count()==0) return "";
  QStringList words = list[0].split(" ");
  if (words.count()==0) return "";

  switch (encoder) {

    case EncoderAssistant::LAME :
      if ((words.contains("version")) && (words.indexOf("version")+1<words.count())) return words[words.indexOf("version")+1];
      if (words.count()<2) return "";
      return words[words.count()-2];

    case EncoderAssistant::OGGENC :

    case EncoderAssistant::FLAC :
      return words.last();

    case EncoderAssistant::FAAC :
      if (list.count()<2) return "";
      words = list[1].split(" ");
      if (words.count()<2) return "";
      if ((words.contains("FAAC")) && (words.indexOf("FAAC")+1<words.count())) return words[words.indexOf("FAAC")+1];
      return words[1];

    case EncoderAssistant::WAVE :

    case EncoderAssistant::CUSTOM :

    default : ;

  }

  return "";

}

long EncoderAssistant::versionNumber(const EncoderAssistant::Encoder encoder) {

    QString v = version(encoder);
    long versionNumber = 0;

    switch (encoder) {

    case EncoderAssistant::LAME :

    case EncoderAssistant::OGGENC :

    case EncoderAssistant::FLAC :

    case EncoderAssistant::FAAC:
    {
        // At present all encoders seem to use 2 or 3 version number items
        // separated by . so we use the same code for all
        // convert to a number for easy version comparison.
        // Each of the 3 version components must be < 255
        QStringList version = v.split(".");
        int major = 0;
        int minor = 0;
        int patch = 0;


        if (version.count() > 0) {
            major = version[0].toUInt();
            versionNumber = (major & 0xFF) << 16;
        }
        if (version.count() > 1) {
           minor = version[1].toUInt();
           versionNumber = versionNumber | (minor & 0xFF) << 8;
        }
        if (version.count() > 2) {
            patch = version[2].toUInt();
            versionNumber = versionNumber | (patch & 0xFF);
        }
    }
    break;

    case EncoderAssistant::WAVE :

    case EncoderAssistant::CUSTOM :

    default : ;

    }
    return versionNumber;
}

const QString EncoderAssistant::pattern(const EncoderAssistant::Encoder encoder, const Parameters& parameters) {

  switch (encoder) {

    case EncoderAssistant::LAME : {
      int preset = parameters.valueToInt(ENCODER_LAME_PRESET_KEY, ENCODER_LAME_PRESET);
      bool cbr = parameters.valueToBool(ENCODER_LAME_CBR_KEY);
      int bitrate = parameters.valueToInt(ENCODER_LAME_BITRATE_KEY, ENCODER_LAME_BITRATE);
      bool embed_cover = parameters.valueToBool(ENCODER_LAME_EMBED_COVER_KEY);
      QString cmd = ENCODER_LAME_BIN;
      switch (preset) {
        case 0 : cmd += " --preset medium"; break;
        case 1 : cmd += " --preset standard"; break;
        case 2 : cmd += " --preset extreme"; break;
        case 3 : cmd += " --preset insane"; break;
        case 4 : cmd += QString(" --preset")+(cbr?QString(" cbr"):QString())+QString(" %1").arg(bitrate); break;
        default : cmd += " --preset standard";
      }
      QString v = EncoderAssistant::version(EncoderAssistant::LAME);
      if ((v.startsWith("3.95")) || (v.startsWith("3.96")) || (v.startsWith("3.97"))) {
        cmd += QString(" --vbr-new");
      }

      //if we have eyeD3, use this for tagging as lame can't handle unicode
      if (KProcess::execute(ENCODER_LAME_HELPER_TAG, QStringList() << ENCODER_LAME_HELPER_TAG_VERSION_PARA) == 0) {

	cmd += QString::fromUtf8(" $"VAR_INPUT_FILE" $"VAR_OUTPUT_FILE);
	cmd += QString::fromUtf8(" && eyeD3 -t \"$"VAR_TRACK_TITLE"\" -a \"$"VAR_TRACK_ARTIST"\" --set-text-frame=TPE2:\"$"VAR_ALBUM_ARTIST"\" -A \"$"VAR_ALBUM_TITLE \
	                         "\" -Y \"$"VAR_DATE"\" -n $"VAR_TRACK_NO" -N $"VAR_NO_OF_TRACKS" --set-text-frame=\"TCON:$"VAR_GENRE"\" "\
	                         "--set-text-frame=TSSE:\"$"VAR_AUDEX" / Encoder $"VAR_ENCODER"\" ${"VAR_CD_NO" pre=\"--set-text-frame=TPOS:\"}");
        if (embed_cover) {
          cmd += QString::fromUtf8(" ${"VAR_COVER_FILE" pre=\"--add-image=\" post=\":FRONT_COVER\"}");
        }
	cmd += QString::fromUtf8(" --set-encoding=latin1 $"VAR_OUTPUT_FILE" && eyeD3 --to-v2.3 $"VAR_OUTPUT_FILE);

      } else {

        if (embed_cover) {
          cmd += QString::fromUtf8(" ${"VAR_COVER_FILE" pre=\"--ti \"}");
        }
        cmd += QString::fromUtf8(" --add-id3v2 --id3v2-only --ignore-tag-errors --tt \"$"VAR_TRACK_TITLE"\" --ta \"$"VAR_TRACK_ARTIST \
                             "\" --tl \"$"VAR_ALBUM_TITLE"\" --ty \"$"VAR_DATE"\" --tn \"$"VAR_TRACK_NO"/$"VAR_NO_OF_TRACKS"\" --tc \"$"VAR_AUDEX" / Encoder $"VAR_ENCODER"\" "\
                             "--tg \"$"VAR_GENRE"\" ${"VAR_CD_NO" pre=\"--tv TPOS=\"}"\
                             " $"VAR_INPUT_FILE" $"VAR_OUTPUT_FILE);

      }

      return cmd;
    }

    case EncoderAssistant::OGGENC : {
      double quality = parameters.valueToDouble(ENCODER_OGGENC_QUALITY_KEY, ENCODER_OGGENC_QUALITY);
      bool min_bitrate = parameters.valueToBool(ENCODER_OGGENC_MINBITRATE_KEY);
      int min_bitrate_value = parameters.valueToInt(ENCODER_OGGENC_MINBITRATE_VALUE_KEY, ENCODER_OGGENC_MINBITRATE_VALUE);
      bool max_bitrate = parameters.valueToBool(ENCODER_OGGENC_MAXBITRATE_KEY);
      int max_bitrate_value = parameters.valueToInt(ENCODER_OGGENC_MAXBITRATE_VALUE_KEY, ENCODER_OGGENC_MAXBITRATE_VALUE);
      QString cmd = ENCODER_OGGENC_BIN;
      cmd += QString(" -q %1").arg(quality, 0, 'f', 2);

      if (min_bitrate) {
        cmd += QString(" -m %1").arg(min_bitrate_value);
      }

      if (max_bitrate) {
        cmd += QString(" -M %1").arg(max_bitrate_value);
      }

      cmd += QString::fromUtf8(" -c \"Artist=$"VAR_TRACK_ARTIST"\" -c \"Title=$"VAR_TRACK_TITLE"\" -c \"Album=$"VAR_ALBUM_TITLE\
                               "\" -c \"Date=$"VAR_DATE"\" -c \"Tracknumber=$"VAR_TRACK_NO"\" -c \"Genre=$"VAR_GENRE\
			       "\" ${"VAR_CD_NO" pre=\"-c Discnumber=\"}"\
                               " -o $"VAR_OUTPUT_FILE" $"VAR_INPUT_FILE);

      return cmd;
    }

    case EncoderAssistant::FLAC : {
      int compression = parameters.valueToInt(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION);
      bool embed_cover = parameters.valueToBool(ENCODER_FLAC_EMBED_COVER_KEY);
      QString cmd = ENCODER_FLAC_BIN;

      if (embed_cover) {
          long versionNumber=EncoderAssistant::versionNumber(EncoderAssistant::FLAC);

          if (versionNumber >= makeVersionNumber(1,1,3) ) {
              cmd += QString::fromUtf8(" --picture=\\|\\|\\|\\|$"VAR_COVER_FILE);
          }
      }
      cmd += QString(" -%1").arg(compression);
      cmd += QString::fromUtf8(" -T Artist=\"$"VAR_TRACK_ARTIST"\" -T Title=\"$"VAR_TRACK_TITLE"\" -T Album=\"$"VAR_ALBUM_TITLE\
                               "\" -T Date=\"$"VAR_DATE"\" -T Tracknumber=\"$"VAR_TRACK_NO"\" -T Genre=\"$"VAR_GENRE\
                               "\" -o $"VAR_OUTPUT_FILE" $"VAR_INPUT_FILE);
      return cmd;
    }

    case EncoderAssistant::FAAC : {
      int quality = parameters.valueToInt(ENCODER_FAAC_QUALITY_KEY, ENCODER_FAAC_QUALITY);
      QString cmd = ENCODER_FAAC_BIN;
      cmd += QString(" -q %1").arg(quality);
      cmd += QString::fromUtf8(" --title \"$"VAR_TRACK_TITLE"\" --artist \"$"VAR_TRACK_ARTIST"\" --album \"$"VAR_ALBUM_TITLE\
                               "\" --year \"$"VAR_DATE"\" --track $"VAR_TRACK_NO" ${"VAR_CD_NO" pre=\"--disc \"} --genre \"$"VAR_GENRE\
                               "\" -o $"VAR_OUTPUT_FILE" $"VAR_INPUT_FILE);

      return cmd;
    }

    case EncoderAssistant::WAVE : {
      return QString(ENCODER_WAVE_BIN)+" $"VAR_INPUT_FILE" $"VAR_OUTPUT_FILE;
    }

    case EncoderAssistant::CUSTOM : {
      return parameters.value(ENCODER_CUSTOM_COMMAND_PATTERN_KEY, ENCODER_CUSTOM_COMMAND_PATTERN);
    }

    default : ;

  }

  return "";

}

Parameters EncoderAssistant::stdParameters(const Encoder encoder, const Quality quality) {

  Parameters parameters;

  switch (encoder) {

    case EncoderAssistant::LAME :

      switch (quality) {

        case NORMAL :
          parameters.setValue(ENCODER_LAME_PRESET_KEY, ENCODER_LAME_PRESET);
          parameters.setValue(ENCODER_LAME_EMBED_COVER_KEY, ENCODER_LAME_EMBED_COVER);
          parameters.setValue(ENCODER_LAME_BITRATE_KEY, ENCODER_LAME_BITRATE);
          parameters.setValue(ENCODER_LAME_EMBED_COVER_KEY, ENCODER_LAME_EMBED_COVER);
	  break;

        case MOBILE :
          parameters.setValue(ENCODER_LAME_PRESET_KEY, ENCODER_LAME_PRESET_M);
          parameters.setValue(ENCODER_LAME_EMBED_COVER_KEY, ENCODER_LAME_EMBED_COVER_M);
          parameters.setValue(ENCODER_LAME_BITRATE_KEY, ENCODER_LAME_BITRATE_M);
          parameters.setValue(ENCODER_LAME_EMBED_COVER_KEY, ENCODER_LAME_EMBED_COVER_M);
	  break;

	case EXTREME :
          parameters.setValue(ENCODER_LAME_PRESET_KEY, ENCODER_LAME_PRESET_X);
          parameters.setValue(ENCODER_LAME_EMBED_COVER_KEY, ENCODER_LAME_EMBED_COVER_X);
          parameters.setValue(ENCODER_LAME_BITRATE_KEY, ENCODER_LAME_BITRATE_X);
          parameters.setValue(ENCODER_LAME_EMBED_COVER_KEY, ENCODER_LAME_EMBED_COVER_X);
	  break;

      }

      break;

    case EncoderAssistant::OGGENC :

      switch (quality) {

        case NORMAL :
          parameters.setValue(ENCODER_OGGENC_QUALITY_KEY, ENCODER_OGGENC_QUALITY);
          parameters.setValue(ENCODER_OGGENC_MINBITRATE_KEY, ENCODER_OGGENC_MINBITRATE);
          parameters.setValue(ENCODER_OGGENC_MINBITRATE_VALUE_KEY, ENCODER_OGGENC_MINBITRATE_VALUE);
          parameters.setValue(ENCODER_OGGENC_MAXBITRATE_KEY, ENCODER_OGGENC_MAXBITRATE);
          parameters.setValue(ENCODER_OGGENC_MAXBITRATE_VALUE_KEY, ENCODER_OGGENC_MAXBITRATE_VALUE);
	  break;

        case MOBILE :
          parameters.setValue(ENCODER_OGGENC_QUALITY_KEY, ENCODER_OGGENC_QUALITY_M);
          parameters.setValue(ENCODER_OGGENC_MINBITRATE_KEY, ENCODER_OGGENC_MINBITRATE_M);
          parameters.setValue(ENCODER_OGGENC_MINBITRATE_VALUE_KEY, ENCODER_OGGENC_MINBITRATE_VALUE_M);
          parameters.setValue(ENCODER_OGGENC_MAXBITRATE_KEY, ENCODER_OGGENC_MAXBITRATE_M);
          parameters.setValue(ENCODER_OGGENC_MAXBITRATE_VALUE_KEY, ENCODER_OGGENC_MAXBITRATE_VALUE_M);
	  break;

	case EXTREME :
	  parameters.setValue(ENCODER_OGGENC_QUALITY_KEY, ENCODER_OGGENC_QUALITY_X);
          parameters.setValue(ENCODER_OGGENC_MINBITRATE_KEY, ENCODER_OGGENC_MINBITRATE_X);
          parameters.setValue(ENCODER_OGGENC_MINBITRATE_VALUE_KEY, ENCODER_OGGENC_MINBITRATE_VALUE_X);
          parameters.setValue(ENCODER_OGGENC_MAXBITRATE_KEY, ENCODER_OGGENC_MAXBITRATE_X);
          parameters.setValue(ENCODER_OGGENC_MAXBITRATE_VALUE_KEY, ENCODER_OGGENC_MAXBITRATE_VALUE_X);
	  break;

      }

      break;

    case EncoderAssistant::FLAC :

      parameters.setValue(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION);
      parameters.setValue(ENCODER_FLAC_EMBED_COVER_KEY, ENCODER_FLAC_EMBED_COVER);

      break;

    case EncoderAssistant::FAAC :

      switch (quality) {

        case NORMAL :
	  parameters.setValue(ENCODER_FAAC_QUALITY_KEY, ENCODER_FAAC_QUALITY);
	  break;

        case MOBILE :
	  parameters.setValue(ENCODER_FAAC_QUALITY_KEY, ENCODER_FAAC_QUALITY_M);
	  break;

	case EXTREME :
	  parameters.setValue(ENCODER_FAAC_QUALITY_KEY, ENCODER_FAAC_QUALITY_X);
	  break;

      }

      break;

    case EncoderAssistant::WAVE :

    case EncoderAssistant::CUSTOM :

    default : ;

  }

  return parameters;

}

const QMap<int,QString> EncoderAssistant::encoderList() {

  QMap<int,QString> encoders;

  encoders[(int)EncoderAssistant::LAME] = ENCODER_LAME_NAME;
  encoders[(int)EncoderAssistant::OGGENC] = ENCODER_OGGENC_NAME;
  encoders[(int)EncoderAssistant::FLAC] = ENCODER_FLAC_NAME;
  encoders[(int)EncoderAssistant::FAAC] = ENCODER_FAAC_NAME;
  encoders[(int)EncoderAssistant::WAVE] = ENCODER_WAVE_NAME;
  encoders[(int)EncoderAssistant::CUSTOM] = ENCODER_CUSTOM_NAME;

  return encoders;

}

const QMap<int,QString> EncoderAssistant::availableEncoderNameList() {

  QMap<int,QString> encoders;

  if (EncoderAssistant::available(EncoderAssistant::LAME)) encoders[(int)EncoderAssistant::LAME] = ENCODER_LAME_NAME;
  if (EncoderAssistant::available(EncoderAssistant::OGGENC)) encoders[(int)EncoderAssistant::OGGENC] = ENCODER_OGGENC_NAME;
  if (EncoderAssistant::available(EncoderAssistant::FLAC)) encoders[(int)EncoderAssistant::FLAC] = ENCODER_FLAC_NAME;
  if (EncoderAssistant::available(EncoderAssistant::FAAC)) encoders[(int)EncoderAssistant::FAAC] = ENCODER_FAAC_NAME;
  if (EncoderAssistant::available(EncoderAssistant::WAVE)) encoders[(int)EncoderAssistant::WAVE] = ENCODER_WAVE_NAME;
  if (EncoderAssistant::available(EncoderAssistant::CUSTOM)) encoders[(int)EncoderAssistant::CUSTOM] = ENCODER_CUSTOM_NAME;

  return encoders;

}

const QMap<int,QString> EncoderAssistant::availableEncoderNameListWithVersions() {

  QMap<int,QString> encoders;

  if (EncoderAssistant::available(EncoderAssistant::LAME)) encoders[(int)EncoderAssistant::LAME] = ENCODER_LAME_NAME+" "+version(LAME);
  if (EncoderAssistant::available(EncoderAssistant::OGGENC)) encoders[(int)EncoderAssistant::OGGENC] = ENCODER_OGGENC_NAME+" "+version(OGGENC);
  if (EncoderAssistant::available(EncoderAssistant::FLAC)) encoders[(int)EncoderAssistant::FLAC] = ENCODER_FLAC_NAME+" "+version(FLAC);
  if (EncoderAssistant::available(EncoderAssistant::FAAC)) encoders[(int)EncoderAssistant::FAAC] = ENCODER_FAAC_NAME+" "+version(FAAC);
  if (EncoderAssistant::available(EncoderAssistant::WAVE)) encoders[(int)EncoderAssistant::WAVE] = ENCODER_WAVE_NAME+" "+version(WAVE);
  if (EncoderAssistant::available(EncoderAssistant::CUSTOM)) encoders[(int)EncoderAssistant::CUSTOM] = ENCODER_CUSTOM_NAME+" "+version(CUSTOM);

  return encoders;

}
