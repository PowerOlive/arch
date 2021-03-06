/*
 * sqlite3_db_impl.cpp
 *
 *  Created on: 2011-6-29
 *      Author: qiyingwang
 */
#include "database/sqlite3_db_impl.hpp"
#if HAVE_SQLITE_LIB == 1
#include <sqlite3.h>
#include "util/string_helper.hpp"

using namespace arch::database;
using namespace arch::util;

#define SQLITE3_BUSY_TIMEOUT 10

static int SQLite3ConnectionBusyHandlerCallBack(void* data, int count)
{
	SQLite3Connection* conn = (SQLite3Connection*) data;
	if (conn->GetBusyTimeout() > 0)
	{
		if (count * SQLITE3_BUSY_TIMEOUT >= conn->GetBusyTimeout())
		{
			return 0;
		}
	}
	return 1;
}

SQLite3Connection::SQLite3Connection() :
		m_handler(NULL), m_error_code(0), m_busy_timeout(0)
{
	m_szSQL[0] = '\0';
	m_szErrMsg[0] = '\0';

}

SQLite3Connection::~SQLite3Connection()
{
	Close();
}

int SQLite3Connection::Connect(const std::string& path)
{
	if (NULL != m_handler)
		return 0;
	int rc = sqlite3_open(path.c_str(), &m_handler);
	if (rc)
	{
		//LOG_ERROR("Failed to open local config DB file:%s for reason(%d):%s",
		//        path.c_str(), rc, sqlite3_errmsg(m_handler));
		sqlite3_close(m_handler);
		m_handler = NULL;
		return -1;
	}
	return 0;
}

void SQLite3Connection::SetBusyTimeout(int32 val)
{
	if (val < 0)
	{
		//clear busy handler, timeout
		sqlite3_busy_timeout(m_handler, -1);
		sqlite3_busy_handler(m_handler, NULL, NULL);
	}
	else
	{
		sqlite3_busy_timeout(m_handler, 10);
		sqlite3_busy_handler(m_handler, SQLite3ConnectionBusyHandlerCallBack,
				this);
	}
	m_busy_timeout = val;
}

void SQLite3Connection::Close()
{
	if (NULL != m_handler)
	{
		int ret = sqlite3_close(m_handler);
		if(SQLITE_OK != ret)
		{
		}
		m_handler = NULL;
	}
}

int SQLite3Connection::ExecuteUpdate(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = VExecuteUpdate(fmt, ap);
	va_end(ap);
	return ret;
}

int SQLite3Connection::Ping()
{
	return 0;
}

Statement* SQLite3Connection::PrepareStatement(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(m_szSQL, sizeof(m_szSQL) - 1, fmt, ap);
	va_end(ap);
	sqlite3_stmt* sqlite_stmt;
	const char* tail = 0;
	int ret = sqlite3_prepare_v2(m_handler, m_szSQL, -1,
			&sqlite_stmt, &tail);
	if (ret != SQLITE_OK)
	{
		m_error_code = ret;
		return NULL;
	}
	Statement* stmt = NULL;
	NEW(stmt, SQLite3Staement(sqlite_stmt));
	return stmt;
}

int SQLite3Connection::VExecuteUpdate(const char* fmt, va_list ap)
{
	if (NULL == m_handler)
	{
		return -1;
	}
	vsnprintf(m_szSQL, sizeof(m_szSQL) - 1, fmt, ap);
	char* errMsg = NULL;
	int ret = sqlite3_exec(m_handler, m_szSQL, 0, 0, &errMsg);
	if (ret)
	{
		snprintf(m_szErrMsg, MAX_ERR_MSG_LEN - 1, "%s", errMsg);
		sqlite3_free(errMsg);
		m_error_code = ret;
		return -1;
	}
	return 0;
}

int SQLite3Connection::ExecuteQuery(SQLResultSet& result, const char* fmt, ...)
{
	//assert(fmt);
	va_list ap;
	va_start(ap, fmt);
	SQLite3ResultSet* sqlite3_result = NULL;
	NEW(sqlite3_result, SQLite3ResultSet);
	if (NULL == sqlite3_result)
	{
		return -1;
	}
	int ret = VExecuteQuery(sqlite3_result, fmt, ap);
	va_end(ap);
	InjectRawResultSet(result, sqlite3_result, StandardDestructor<ResultSet>);
	return ret;
}

int SQLite3Connection::QueryCallBack(void* data, int n_columns,
		char** column_values, char** column_names)
{
	SQLite3ResultSet* res = (SQLite3ResultSet*) data;
	if (NULL != column_values && n_columns > 0 && column_values[0] != NULL)
	{
		res->AppendRow(n_columns, column_values);
	}
	if (NULL != column_names)
	{
		res->SetColumnNames(n_columns, column_names);
	}
	return 0;
}

int SQLite3Connection::VExecuteQuery(SQLite3ResultSet* result, const char* fmt,
		va_list ap)
{
	if (NULL == m_handler)
	{
		return -1;
	}
	vsnprintf(m_szSQL, sizeof(m_szSQL) - 1, fmt, ap);
	char* errMsg = NULL;
	int ret = sqlite3_exec(m_handler, m_szSQL, SQLite3Connection::QueryCallBack,
			result, &errMsg);
	if (ret != SQLITE_OK)
	{
		//LOG_ERROR("Failed to exec sql:%s for reason:%s.", m_szSQL, errMsg);
		snprintf(m_szErrMsg, MAX_ERR_MSG_LEN - 1, "%s", errMsg);
		sqlite3_free(errMsg);
		m_error_code = ret;
		return -1;
	}

	return 0;
}

int SQLite3Connection::GetErrCode() const
{
	return m_error_code;
}

const char* SQLite3Connection::GetSQL() const
{
	return m_szSQL;
}

const char* SQLite3Connection::GetError() const
{
	return m_szErrMsg;
}

int SQLite3Connection::SetProperties(const Properties& props)
{
	Properties::const_iterator found = props.find("BusyTimeout");
	if (found != props.end())
	{
		const std::string& val = found->second;
		if (string_toint32(val, m_busy_timeout))
		{
			SetBusyTimeout(m_busy_timeout);
		}
	}
	m_properties = props;
	return 0;
}

int SQLite3Connection::Connect(const Properties& props)
{
	std::string db;
	Properties::const_iterator found = props.find("DataBaseFile");
	if (found != props.end())
	{
		db = found->second;
	}
	if (db.empty())
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg) - 1,
				"Empty 'DataBaseFile' property specified");
		return -1;
	}

	int ret = Connect(db);
	SetProperties(props);
	return ret;
}

int SQLite3Connection::BeginTransaction()
{
	return ExecuteUpdate("BEGIN TRANSACTION");
}
int SQLite3Connection::CommitTransaction()
{
	return ExecuteUpdate("END TRANSACTION");
}
int SQLite3Connection::RollBack()
{
	return ExecuteUpdate("ROLLBACK TRANSACTION");
}

//////////////////////////////////////////////////////////
//ResultSet definition
//////////////////////////////////////////////////////////

SQLite3ResultSet::SQLite3ResultSet() :
		m_column_num(0), m_cursor(0)
{

}

int SQLite3ResultSet::SetColumnNames(int n_columns, char** column_names)
{
	if (NULL == column_names || n_columns <= 0)
	{
		return 0;
	}
	m_column_names.clear();
	for (int i = 0; i < n_columns; i++)
	{
		m_column_names.push_back(column_names[i]);
	}
	return 0;
}

int SQLite3ResultSet::AppendRow(int n_columns, char** column_values)
{
	if (NULL == column_values || n_columns <= 0)
	{
		return 0;
	}
	Row tmp;
	tmp.column_values = column_values;
	tmp.n_columns = n_columns;
	CopiedRow* row = NULL;
	NEW(row, CopiedRow(tmp));
	if (NULL == row)
	{
		return -1;
	}
	m_rows.push_back(row);
	m_column_num = n_columns;
	return 0;
}

SQLite3ResultSet::~SQLite3ResultSet()
{
	RowVector::iterator it = m_rows.begin();
	while (it != m_rows.end())
	{
		CopiedRow* row = *it;
		DELETE(row);
		it++;
	}
}

int SQLite3ResultSet::Next(Row& row)
{
	if (m_cursor >= m_rows.size())
		return -1;

	CopiedRow* found = m_rows[m_cursor];
	row.column_values = found->GetRow().column_values;
	row.n_columns = found->GetRow().n_columns;
	m_cursor++;
	return 0;
}

int SQLite3ResultSet::ColumnNum()
{
	return m_column_num;
}

int SQLite3ResultSet::RowNum()
{
	return m_rows.size();
}

int SQLite3ResultSet::ColumnNames(ColumnNameArray& names)
{
	names.clear();
	names = m_column_names;
	return 0;
}

int SQLite3Staement::BindText(int index, const char* value)
{
	return sqlite3_bind_text(m_stmt, index, value, -1, SQLITE_TRANSIENT );
}
int SQLite3Staement::BindInt64(int index, int64 value)
{
	return sqlite3_bind_int64(m_stmt, index, value);
}
int SQLite3Staement::Execute()
{
	int ret = sqlite3_step(m_stmt);
	if (ret != SQLITE_DONE)
	{
		return ret;
	}
	sqlite3_clear_bindings(m_stmt);
	sqlite3_reset(m_stmt);
	return 0;
}
int SQLite3Staement::Close()
{
	if (NULL != m_stmt)
	{
		sqlite3_finalize(m_stmt);
		m_stmt = NULL;
	}
	return 0;
}
SQLite3Staement::SQLite3Staement(sqlite3_stmt* stmt) :
		m_stmt(stmt)
{

}
SQLite3Staement::~SQLite3Staement()
{
	Close();
}

#endif
