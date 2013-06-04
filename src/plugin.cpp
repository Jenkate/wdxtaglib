// WDXTagLib is a content plugin for Total Commander which uses TagLib library
// for reading and editing the meta-data of several popular audio formats.
// Copyright (C) 2008-2013 Dmitry Murzaikin (murzzz@gmail.com)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <string>
#include <sstream>

#include <tag.h>
#include <mpegfile.h>
#include <id3v2tag.h>
#include <id3v2header.h>
#include <id3v1tag.h>
#include <apetag.h>
#include <flacfile.h>
#include <oggflacfile.h>
#include <mpcfile.h>
#include <oggfile.h>
#include <trueaudiofile.h>
#include <wavpackfile.h>

#include "plugin.h"
#include "utils.h"
#include "cunicode.h"

namespace wdx
{
typedef enum FieldIndexes
{
   fiTitle = 0,
   fiArtist,
   fiAlbum,
   fiYear,
   fiTracknumber,
   fiComment,
   fiGenre,
   fiBitrate,
   fiSamplerate,
   fiChannels,
   fiLength_s,
   fiLength_m,
   fiTagType,
} CFieldIndexes;

plugin::plugin()
{
   m_Fields[fiTitle] = field("Title", ft_stringw, contflags_edit);
   m_Fields[fiArtist] = field("Artist", ft_stringw, contflags_edit);
   m_Fields[fiAlbum] = field("Album", ft_stringw, contflags_edit);
   m_Fields[fiYear] = field("Year", ft_numeric_32, contflags_edit);
   m_Fields[fiTracknumber] = field("Tracknumber", ft_numeric_32, contflags_edit);
   m_Fields[fiComment] = field("Comment", ft_stringw, contflags_edit);
   m_Fields[fiGenre] = field("Genre", ft_stringw, contflags_edit);
   m_Fields[fiBitrate] = field("Bitrate", ft_numeric_32);
   m_Fields[fiSamplerate] = field("Sample rate", ft_numeric_32);
   m_Fields[fiChannels] = field("Channels", ft_numeric_32);
   m_Fields[fiLength_s] = field("Length", ft_numeric_32);
   m_Fields[fiLength_m] = field("Length (formatted)", ft_string);
   m_Fields[fiTagType] = field("Tag type", ft_string);
}

plugin::~plugin()
{
}

std::string plugin::OnGetDetectString() const
{
   //return "EXT=\"OGG\" | EXT=\"FLAC\" | EXT=\"OGA\"| EXT=\"MP3\"| EXT=\"MPC\"| EXT=\"WV\"| EXT=\"SPX\"| EXT=\"TTA\"";

   TagLib::String sExtList;
   const TagLib::String sOpen("EXT=\"");
   const TagLib::String sClose("\"");
   const TagLib::String sOr(" | ");

   for (const TagLib::String& str : TagLib::FileRef::defaultFileExtensions())
   {
      sExtList += sOpen + str.upper() + sClose + sOr;
   }

   // remove last sOr
   if (!sExtList.isEmpty())
   {
      sExtList = sExtList.substr(0, sExtList.size() - sOr.size());
   }

   return sExtList.toCString();
}

TagLib::FileRef& plugin::OpenFile(const std::wstring& sFileName)
{
   // if there is no such file then insert it
   // otherwise find its reference
   files_t::iterator iter = m_Files2Write.find(sFileName);

   if (m_Files2Write.end() == iter)
   {
      m_Files2Write[sFileName] = TagLib::FileRef(sFileName.c_str());
      return m_Files2Write[sFileName];
   }
   else
   {
      return (*iter).second;
   }

}

int plugin::OnGetValue(const std::wstring& sFileName, const int iFieldIndex,
      const int iUnitIndex, void* pFieldValue, const int iMaxLen, const int iFlags)
{
   TagLib::FileRef file(sFileName.c_str());

   if (file.isNull() || !file.tag() || !file.audioProperties())
      return ft_fileerror;

   TagLib::Tag *tag = file.tag();
   TagLib::AudioProperties *prop = file.audioProperties();

   switch (iFieldIndex)
   {
      case fiTitle:
         wcslcpy((wchar_t*) pFieldValue, tag->title().toWString().c_str(), iMaxLen / 2);
         break;
      case fiArtist:
         wcslcpy((wchar_t*) pFieldValue, tag->artist().toWString().c_str(), iMaxLen / 2);
         break;
      case fiAlbum:
         wcslcpy((wchar_t*) pFieldValue, tag->album().toWString().c_str(), iMaxLen / 2);
         break;
      case fiYear:
         {
         if (!tag->year())
            return ft_fieldempty;
         *(__int32*) pFieldValue = tag->year();
         break;
      }
      case fiTracknumber:
         {
         if (!tag->track())
            return ft_fieldempty;
         *(__int32*) pFieldValue = tag->track();
         break;
      }
      case fiComment:
         wcslcpy((wchar_t*) pFieldValue, tag->comment().toWString().c_str(), iMaxLen / 2);
         break;
      case fiGenre:
         wcslcpy((wchar_t*) pFieldValue, tag->genre().toWString().c_str(), iMaxLen / 2);
         break;
      case fiBitrate:
         *(__int32*) pFieldValue = prop->bitrate();
         break;
      case fiSamplerate:
         *(__int32*) pFieldValue = prop->sampleRate();
         break;
      case fiChannels:
         *(__int32*) pFieldValue = prop->channels();
         break;
      case fiLength_s:
         *(__int32*) pFieldValue = prop->length();
         break;
      case fiLength_m:
         {
         int seconds = prop->length() % 60;
         int minutes = (prop->length() - seconds) / 60;

         utils::strlcpy((char*) pFieldValue,
               std::string(utils::Int2Str(minutes) + "m " +
                     utils::formatSeconds(seconds) + "s").c_str(), iMaxLen);
         break;
      }
      case fiTagType:
         {
         utils::strlcpy((char*) pFieldValue, GetTagType(file.file()).c_str(), iMaxLen);
         break;
      }
      default:
         return ft_nosuchfield;
         break;
   }

   return m_Fields[iFieldIndex].m_Type;
}

std::string plugin::GetTagType(TagLib::File* pFile) const
      {
   std::ostringstream osResult;
   TagLib::ID3v2::Tag *pId3v2 = NULL;
   TagLib::ID3v1::Tag *pId3v1 = NULL;
   TagLib::APE::Tag *pApe = NULL;
   TagLib::Ogg::XiphComment *pXiph = NULL;

   // get pointers to tags
   TagLib::MPEG::File* pMpegFile = dynamic_cast<TagLib::MPEG::File*>(pFile);
   if (pMpegFile && pMpegFile->isValid())
   {
      pId3v2 = pMpegFile->ID3v2Tag();
      pId3v1 = pMpegFile->ID3v1Tag();
      pApe = pMpegFile->APETag();
   }

   TagLib::FLAC::File* pFlacFile = dynamic_cast<TagLib::FLAC::File*>(pFile);
   if (pFlacFile && pFlacFile->isValid())
   {
      pId3v2 = pFlacFile->ID3v2Tag();
      pId3v1 = pFlacFile->ID3v1Tag();
      pXiph = pFlacFile->xiphComment();
   }

   TagLib::MPC::File* pMpcFile = dynamic_cast<TagLib::MPC::File*>(pFile);
   if (pMpcFile && pMpcFile->isValid())
   {
      pId3v1 = pMpcFile->ID3v1Tag();
      pApe = pMpcFile->APETag();
   }

   TagLib::Ogg::File* pOggFile = dynamic_cast<TagLib::Ogg::File*>(pFile);
   bool bJustSayXiph = pOggFile && pOggFile->isValid(); // ogg files could have only xiph comments

   TagLib::TrueAudio::File* pTAFile = dynamic_cast<TagLib::TrueAudio::File*>(pFile);
   if (pTAFile && pTAFile->isValid())
   {
      pId3v2 = pTAFile->ID3v2Tag();
      pId3v1 = pTAFile->ID3v1Tag();
   }

   TagLib::WavPack::File* pWPFile = dynamic_cast<TagLib::WavPack::File*>(pFile);
   if (pWPFile && pWPFile->isValid())
   {
      pId3v1 = pWPFile->ID3v1Tag();
      pApe = pWPFile->APETag();
   }

   // format text
   bool bUseSeparator = false;
   if (pId3v2 && !pId3v2->isEmpty())
   {
      osResult << "ID3v2."
            << pId3v2->header()->majorVersion()
            << "."
            << pId3v2->header()->revisionNumber();
      bUseSeparator = true;
   }

   if (pId3v1 && !pId3v1->isEmpty())
   {
      osResult << (bUseSeparator ? ", " : "") << "ID3v1";
      bUseSeparator = true;
   }

   if (pApe && !pApe->isEmpty())
      osResult << (bUseSeparator ? ", " : "") << "APE";

   if ((pXiph && !pXiph->isEmpty()) || bJustSayXiph)
      osResult << (bUseSeparator ? ", " : "") << "XiphComment";

   return osResult.str();
}

int plugin::OnSetValue(const std::wstring& sFileName, const int iFieldIndex,
      const int iUnitIndex, const int iFieldType, const void* pFieldValue, const int iFlags)
{
//	if ( !TagLib::File::isWritable(sFileName.c_str()) )
//		return ft_fileerror;

   TagLib::FileRef& file = OpenFile(sFileName);

   if (file.isNull() || !file.tag())
      return ft_fileerror;

   TagLib::Tag *tag = file.tag();

   switch (iFieldIndex)
   {
      case fiTitle:
         tag->setTitle((wchar_t*) pFieldValue);
         break;
      case fiArtist:
         tag->setArtist((wchar_t*) pFieldValue);
         break;
      case fiAlbum:
         tag->setAlbum((wchar_t*) pFieldValue);
         break;
      case fiYear:
         tag->setYear(*(__int32*) pFieldValue);
         break;
      case fiTracknumber:
         tag->setTrack(*(__int32*) pFieldValue);
         break;
      case fiComment:
         tag->setComment((wchar_t*) pFieldValue);
         break;
      case fiGenre:
         tag->setGenre((wchar_t*) pFieldValue);
         break;
      default:
         return ft_nosuchfield;
         break;
   }

   return ft_setsuccess;
}

void plugin::OnEndOfSetValue()
{
   for (files_t::iterator iter = m_Files2Write.begin(); iter != m_Files2Write.end(); ++iter)
      (*iter).second.save();

   m_Files2Write.clear();
}
}
