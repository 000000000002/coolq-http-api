// 
// app.h : Define macros and include files for CQ app.
// 
// Copyright (C) 2017  Richard Chien <richardchienthebest@gmail.com>
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
// 

#pragma once

#include <cqp.h>

#define CQ_APP_ID "io.github.richardchien.coolqhttpapi"
#define CQ_APP_INFO CQAPIVERTEXT "," CQ_APP_ID
#define CQ_APP_NAME "CoolQ HTTP API Plugin"
#define CQ_APP_VERSION "v1.1.5"
#define CQ_APP_FULLNAME CQ_APP_NAME " " CQ_APP_VERSION

#include "common.h"
#include "log.h"

class CQApp;
extern CQApp *CQ;

#include "CQApp.h"
