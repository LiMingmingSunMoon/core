/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_DBACCESS_SOURCE_FILTER_HSQLDB_FBCREATEPARSER_HXX
#define INCLUDED_DBACCESS_SOURCE_FILTER_HSQLDB_FBCREATEPARSER_HXX

#include "createparser.hxx"

namespace dbahsql
{
class FbCreateStmtParser : public CreateStmtParser
{
public:
    FbCreateStmtParser() {}
    virtual OUString compose() const override;
};

} // dbahsql

#endif // INCLUDED_DBACCESS_SOURCE_FILTER_HSQLDB_FBCREATEPARSER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
