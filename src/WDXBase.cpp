// 	WDXTagLib is a content plugin for Total Commander that allows you to show audio
// file tags in columns, edit tags and search in tags.
// Copyright (C) 2008 Dmitrie Murzaikin (murzzz@gmail.com)
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

#include "WDXBase.h"
#include "CUtils.h"
#include <exception>

namespace WDX_API
{
WDXBase::WDXBase()
:	m_PluginInterfaceVerionHi(0),
	m_PluginInterfaceVerionLow(0)
{
	//ctor
}

WDXBase::~WDXBase()
{
	//dtor
}

std::string WDXBase::GetDetectString() const
{
	return OnGetDetectString();
}

std::string WDXBase::OnGetDetectString() const
{
	return "";
}

void WDXBase::SetIniName(const std::string& sIniName)
{
	if (sIniName == m_IniName)
		return;
	m_IniName = sIniName;
}

std::string WDXBase::GetIniName() const
{
	return m_IniName;
}

void WDXBase::SetPluginInterfaceVersion(const DWORD dwHi, const DWORD dwLow)
{
	m_PluginInterfaceVerionHi = dwHi;
	m_PluginInterfaceVerionLow = dwLow;
}

int WDXBase::GetSupportedField( const int iFieldIndex, char* pszFieldName,
								char* pszUnits, int iMaxLen)
{
	try
	{
		if ( iFieldIndex < 0 || iFieldIndex >= (int)m_Fields.size() )
			return ft_nomorefields;

		//const Field& f = m_Fields.at( iFieldIndex );
		const Field& f = m_Fields[ iFieldIndex ];
		CUtils::strlcpy( pszFieldName, f.m_Name.c_str(), iMaxLen - 1 );
		CUtils::strlcpy( pszUnits, f.m_MultChoice.c_str(), iMaxLen - 1 );
		return f.m_Type;
	}
	catch(...)
	{
		ExceptionHandler();
		return ft_nomorefields;
	}
}

int WDXBase::GetValue(const WCHAR* pszFileName, const int iFieldIndex,
						const int iUnitIndex, void* pFieldValue,
						const int iMaxLen, const int iFlags)
{
	try
	{
		if (iUnitIndex < 0)
			CUtils::ShowError(CUtils::Int2Str(iUnitIndex));

		if ( iFieldIndex < 0 || iFieldIndex >= (int)m_Fields.size() )
			return ft_nosuchfield;

		// abort flag down
		SetAborted( false );

		return OnGetValue(pszFileName, iFieldIndex, iUnitIndex, pFieldValue, iMaxLen, iFlags);
	}
	catch(...)
	{
		ExceptionHandler();
		return ft_fileerror;
	}
}

int WDXBase::SetValue(const WCHAR* FileName, const int FieldIndex,
						const int UnitIndex, const int FieldType,
						const void* FieldValue, const int flags)
{
	try
	{
		if ( !FileName || (-1 == FieldIndex) ) // this indicates end of changing attributes
		{
			OnEndOfSetValue();
			return ft_setsuccess;
		}

		if ( FieldIndex < 0 || FieldIndex >= (int)m_Fields.size() )
			return ft_nosuchfield;

		return OnSetValue(FileName, FieldIndex, UnitIndex, FieldType, FieldValue, flags);
	}
	catch(...)
	{
		ExceptionHandler();
		return ft_fileerror;
	}
}

int WDXBase::OnSetValue(const string_t& sFileName, const int iFieldIndex,
				const int iUnitIndex, const int iFieldType,
				const void* pFieldValue, const int iFlags) const
{
	return ft_nosuchfield;
}

int WDXBase::GetSupportedFieldFlags(const int iFieldIndex)
{
	try
	{
		if (-1 == iFieldIndex) // we should return a combination of all supported flags here
		{
			int iTotalFlags = 0;
			for (CMapOfFields::const_iterator iter = m_Fields.begin();
				iter != m_Fields.end();
				++iter)
			{
				const Field& f = (*iter).second;
				if (f.m_Flag)
					iTotalFlags |= f.m_Flag;
			}
			return iTotalFlags;
		}

		if ( iFieldIndex < 0 || iFieldIndex >= (int)m_Fields.size() )
			return ft_nomorefields;

		//return m_Fields.at(iFieldIndex).m_Flag;
		return m_Fields[iFieldIndex].m_Flag;
	}
	catch(...)
	{
		ExceptionHandler();
		return 0; // not sure what to return here
	}
}

void WDXBase::OnEndOfSetValue() const
{
}

void WDXBase::ExceptionHandler() const
{
	try
	{
		throw;
	}
	catch(const std::exception& e)
	{
		CUtils::ShowError(CUtils::toWideString(e.what()));
	}
	catch(...)
	{
		CUtils::ShowError(TEXT("Unknown exception"));
	}
}

void WDXBase::StopGetValue(const string_t& sFileName)
{
	// abort flag up

	SetAbortedFilename( sFileName );
	SetAborted( true );
}

void WDXBase::SetAborted( const bool bValue )
{
	m_bIsAborted = bValue;

	// clear filename when not in aborted state
	if (!m_bIsAborted)
		SetAbortedFilename(TEXT(""));
}

bool WDXBase::IsAborted() const
{
	return m_bIsAborted;
}

void WDXBase::SetAbortedFilename(const string_t& sValue)
{
	m_sAbortedFilename = sValue;
}

string_t WDXBase::GetAbortedFilename() const
{
	return m_sAbortedFilename;
}

}
