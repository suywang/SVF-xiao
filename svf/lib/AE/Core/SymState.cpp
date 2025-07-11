//===- SymState.cpp ----Symbolic State-------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013-2022>  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

//
// Created by jiawei and xiao on 6/1/23.
//


#include "AE/Core/SymState.h"

using namespace SVF;

Map<std::string, TypeState> TypeStateParser::_typeMap = {
    {"$uninit",   TypeState::Uinit},
    {"$error",    TypeState::Error},
    {"Allocated", TypeState::Allocated},
    {"Freed",     TypeState::Freed},
    {"Opened",    TypeState::Opened}
};

Map<TypeState, std::string> TypeStateParser::_revTypeMap = {
    {TypeState::Uinit, "$uninit"},
    {TypeState::Error, "$error"},
    {TypeState::Allocated, "Allocated"},
    {TypeState::Freed, "Freed"},
    {TypeState::Opened, "Opened"}
};

SymState::SymState(ConsExeState es, TypeState ts) : _exeState(SVFUtil::move(es)), _typeState(SVFUtil::move(ts)),
    _branchCondition(Z3Expr::getContext().bool_val(true))
{

}