/*                  C O M M A N D S T R I N G . C P P
 * BRL-CAD
 *
 * Copyright (c) 2022-2025 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file CommandString.cpp
 *
 *  BRL-CAD core C++ interface:
 *      libged command string parser module
 */

#include "bu/malloc.h"
#include "bu/parallel.h"
#include "bu/str.h"
#include "ged/commands.h"
#include "rt/db_io.h"

#include <brlcad/CommandString/CommandString.h>


using namespace BRLCAD;


CommandString::CommandString
(
    Database& database
) {
    if (!BU_SETJUMP) {
        BU_GET(m_ged, ged);
    }
    else {
        BU_UNSETJUMP;

        m_ged = nullptr;
    }

    BU_UNSETJUMP;

    if (m_ged != nullptr) {
        if (!BU_SETJUMP) {
            ged_init((m_ged));

            if (database.m_wdbp != nullptr)
                m_ged->dbip = db_clone_dbi(database.m_wdbp->dbip, nullptr);
            else
                m_ged->dbip = nullptr;
        }
        else {
            BU_UNSETJUMP;

            BU_PUT(m_ged, ged);
            m_ged = nullptr;
        }

        BU_UNSETJUMP;
    }
}


CommandString::~CommandString(void) {
    if (m_ged != nullptr) {
        if (!BU_SETJUMP)
            ged_close(m_ged);

        BU_UNSETJUMP;
    }
}


bool CommandString::Parse
(
    const std::vector<const char*>& arguments
) {
    int ret = BRLCAD_ERROR;

    if (m_ged != nullptr) {
        if (!BU_SETJUMP)
            ret = ged_exec(m_ged, arguments.size(), const_cast<const char**>(arguments.data()));

        BU_UNSETJUMP;
    }

    return (ret == BRLCAD_OK);
}


bool CommandString::Parse
(
    const std::vector<const char*>& arguments,
    ParseFlag                       flag
) {
    int ret = BRLCAD_ERROR;
    flag = ParseFlag::Undefined;

    if (m_ged != nullptr) {
        if (!BU_SETJUMP)
            ret = ged_exec(m_ged, arguments.size(), const_cast<const char**>(arguments.data()));
        else {
            BU_UNSETJUMP;

            return 1;
        }

        BU_UNSETJUMP;
    }
    else
        return 1;

    if (ret == BRLCAD_OK)
        flag = ParseFlag::Ok;
    else {
        if (ret & GED_HELP)
            flag = ParseFlag::Help;
        else if (ret & GED_MORE)
            flag = ParseFlag::More;
        else if (ret & GED_QUIET)
            flag = ParseFlag::Quiet;
        else if (ret & GED_UNKNOWN)
            flag = ParseFlag::Unknown;
        else if (ret & GED_EXIT)
            flag = ParseFlag::Exit;
        else if (ret & GED_OVERRIDE)
            flag = ParseFlag::Override;
    }

    return 0;
}


bool CommandString::Parse
(
    const std::vector<const char*>&            arguments,
    const std::function<void(ParseFlag flag)>& callback
) {
    int ret = BRLCAD_ERROR;

    if (m_ged != nullptr) {
        if (!BU_SETJUMP)
            ret = ged_exec(m_ged, arguments.size(), const_cast<const char**>(arguments.data()));
        else {
            BU_UNSETJUMP;

            return 1;
        }

        BU_UNSETJUMP;
    }
    else
        return 1;

    if (ret == BRLCAD_OK)
        callback(ParseFlag::Ok);
    else {
        if (ret & GED_HELP)
            callback(ParseFlag::Help);
        else if (ret & GED_MORE)
            callback(ParseFlag::More);
        else if (ret & GED_QUIET)
            callback(ParseFlag::Quiet);
        else if (ret & GED_UNKNOWN)
            callback(ParseFlag::Unknown);
        else if (ret & GED_EXIT)
            callback(ParseFlag::Exit);
        else if (ret & GED_OVERRIDE)
            callback(ParseFlag::Override);
        else
            callback(ParseFlag::Undefined);
    }

    return 0;
}


const char* CommandString::Results(void) const {
    return bu_vls_cstr(m_ged->ged_result_str);
}


size_t CommandString::NumberOfResults(void) const {
    return ged_results_count(m_ged->ged_results);
}


const char* CommandString::Result
(
    size_t index
) const {
    return ged_results_get(m_ged->ged_results, index);
}


void CommandString::ClearResults(void) {
    bu_vls_free(m_ged->ged_result_str);
    ged_results_clear(m_ged->ged_results);
}


void CommandString::CompleteCommand
(
    const char*                                          pattern,
    const std::function<bool(const char* commandMatch)>& callback
) {
    const char** completions         = nullptr;
    int          numberOfCompletions = ged_cmd_completions(&completions, pattern);

    for (size_t i = 0; i < numberOfCompletions; ++i) {
        if (!callback(completions[i]))
            break;
    }

    bu_argv_free(numberOfCompletions, const_cast<char**>(completions));
}


void CommandString::CompleteObject
(
    const char*                                         pattern,
    const std::function<bool(const char* objectMatch)>& callback
) {
    const char** completions         = nullptr;
    bu_vls       cprefix             = BU_VLS_INIT_ZERO;
    int          numberOfCompletions = ged_geom_completions(&completions, &cprefix, m_ged->dbip, pattern);

    for (size_t i = 0; i < numberOfCompletions; ++i) {
        if (!callback(completions[i]))
            break;
    }

    bu_argv_free(numberOfCompletions, const_cast<char**>(completions));
}
