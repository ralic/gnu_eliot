/*****************************************************************************
 * Eliot
 * Copyright (C) 2005-2008 Antoine Fraboulet & Olivier Teulière
 * Authors: Antoine Fraboulet <antoine.fraboulet @@ free.fr>
 *          Olivier Teulière  <ipkiss @@ gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *****************************************************************************/

/**
 *  \file   regexpmain.c
 *  \brief  Program used to test regexp
 *  \author Antoine Fraboulet
 *  \date   2005
 */

#include "config.h"

#include <exception>
#include <iostream>
#include <cstring> // For memset

#if ENABLE_NLS
#   include <libintl.h>
#   define _(String) gettext(String)
#else
#   define _(String) String
#endif

#include "dic.h"
#include "header.h"
#include "encoding.h"


void usage(const char *iBinaryName)
{
    cerr << _("usage: %s dictionary") << iBinaryName << endl;
    cerr << _("   dictionary: path to eliot dawg dictionary") << endl;
}


int main(int argc, char* argv[])
{
#if HAVE_SETLOCALE
    // Set locale via LC_ALL
    setlocale(LC_ALL, "");
#endif

#if ENABLE_NLS
    // Set the message domain
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
#endif

    if (argc != 2)
    {
        usage(argv[0]);
        return 0;
    }

    try
    {
        Dictionary dic(argv[1]);

        string line;
        cout << "**************************************************************" << endl;
        cout << "**************************************************************" << endl;
        cout << _("Enter a regular expression:") << endl;
        while (getline(cin, line))
        {
            if (line == "")
                break;

            /* Automaton */
            vector<wstring> wordList;
            dic.searchRegExp(convertToWc(line), wordList, 1, 15);

            cout << _("result:") << endl;
            vector<wstring>::const_iterator it;
            for (it = wordList.begin(); it != wordList.end(); it++)
            {
                cerr << convertToMb(*it) << endl;
            }
            cout << "**************************************************************" << endl;
            cout << "**************************************************************" << endl;
            cout << _("Enter a regular expression:") << endl;
        }

        return 0;
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown exception taken" << endl;
        return 1;
    }
}