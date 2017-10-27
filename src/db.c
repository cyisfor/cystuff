#include "db.h"
#include "mmapfile.h"

#include <sys/mman.h> // munmap
#include <string.h> // memchr

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <unistd.h> // sleep

sqlite3* c = NULL;

static bool derp(int res, int n, sqlite3_stmt* stmt, const char* tail, size_t sl, size_t l) {
	if(res == SQLITE_OK || res == SQLITE_ROW || res == SQLITE_DONE) return false;
	printf("uhh %d %d %d %d\n",res,SQLITE_OK,SQLITE_ROW,SQLITE_DONE);
	fwrite(tail,1,sl,stderr);
	fputc('\n',stderr);
	db_check(res);
}

result_handler default_result_handler = &derp;

#ifdef DEBUG
int db_checkderp(int res, int line)
#else
int db_check(int res, int line)
#endif
{
	switch(res) {
	case SQLITE_OK:
	case SQLITE_ROW:
	case SQLITE_DONE:
		return res;
	};
	printf("%d sqlite error %s (%s)\n",line, sqlite3_errstr(res), sqlite3_errmsg(c));
	abort();
}
 
sqlite3_stmt *begin, *commit;
#define LITLEN(lit) lit, sizeof(lit)-1
sqlite3* db_init() {
	//chdir(getenv("FILEDB"));
	assert(SQLITE_OK == sqlite3_open_v2("media.sqlite", &c,																			
																		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
																			| SQLITE_OPEN_NOMUTEX
																			| SQLITE_OPEN_PRIVATECACHE,
																			NULL));
#ifdef dunnosqlite
	sqlite3_stmt* stmt = NULL;
	const char* tail = NULL;
	int res = sqlite3_prepare_v2(c, "", 0,
															 &stmt,
															 &tail);

	res = sqlite3_prepare_v2(c, LITLEN("\n-- hi\n"),
															 &stmt,
															 &tail);

	res = sqlite3_prepare_v2(c, LITLEN("CREATE 123 DERP SELECT"),
													 &stmt,
													 &tail);
	db_check(res);
#endif
	
	sqlite3_extended_result_codes(c, 1);
	db_check(sqlite3_prepare_v2(c, "BEGIN", 5,
															 &begin,
															 NULL));
	db_check(sqlite3_prepare_v2(c, "COMMIT", 6,
															 &commit,
															 NULL));

	return c;
}


void db_begin() {
	db_check(sqlite3_step(begin));
	sqlite3_reset(begin);
}

void db_commit() {
	db_check(sqlite3_step(commit));
	sqlite3_reset(commit);
}

void db_retransaction() {
	db_commit();
	db_begin();
}
	
void db_close(void) {
	sqlite3_finalize(begin);
	sqlite3_finalize(commit);
	
	int attempt = 0;
	for(;attempt<10;++attempt) {
		int res = sqlite3_close(c);
		if(res == SQLITE_OK) return;
		printf("sqlite close error %s %s\n", sqlite3_errstr(res), sqlite3_errmsg(c));
		if(attempt > 1) {
			sleep(attempt);
		}
		sqlite3_stmt* stmt = NULL;
		while((stmt = sqlite3_next_stmt(c, stmt))) {
			printf("closing statement\n%s\n",sqlite3_sql(stmt));
			db_check(sqlite3_finalize(stmt));
		}
	}
	error(23,23,"could not close the database");
}

static int asht(void* ctx, int cols,char** vals,char** names) {
	printf("uh %d\n",cols);
}

void db_load(const char* path, result_handler on_res) {
	size_t len = 0;
	const char* sql = mmapfile(path,&len);
	char* errmsg = NULL;
	db_execmanyn(sql, len, on_res);
	munmap((void*)sql, len);
}

int db_execn(const char* s, size_t l) {
	sqlite3_stmt* stmt = db_preparen(s,l);
	int res = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	return res;
}

void db_execmanyn(const char* s, size_t l, result_handler on_res) {
	if(on_res == NULL) on_res = default_result_handler;
	sqlite3_stmt* stmt = NULL;
	const char* next = NULL;
	int i = 0;
	for(;;++i) {
		size_t sl = l;
		int res = sqlite3_prepare_v2(c, s, l,
															&stmt,
															&next);
		#define CHECK if(res != SQLITE_OK && res != SQLITE_DONE && res != SQLITE_ROW) { \
				on_res(res,i,stmt,s,sl,l); return; }
		CHECK;
		if(stmt == NULL) return; // just trailing comments, whitespace
		
		if(next != NULL) {
			sl = next - s;
		}

		
		res = sqlite3_step(stmt);
		CHECK;
		res = sqlite3_finalize(stmt);
		CHECK;
		if(on_res(res,i,stmt,s,sl,l)) return;
		
		if(next == NULL)
			break;

		l -= next - s;
		s = next;
	}
}
		

sqlite3_stmt* db_preparen(const char* s, size_t l) {
	sqlite3_stmt* stmt = NULL;
	db_check(sqlite3_prepare_v2(c, s, l,
													 &stmt,
													 NULL));
	return stmt;
}

#ifdef DEBUG
int db_stepderp(sqlite3_stmt* stmt,const char* file, int line)
{
	return db_checkderp(sqlite3_step(stmt),file, line);
}
#else
int db_step(sqlite3_stmt* stmt)	
{
	return db_check(sqlite3_step(stmt));
}
#endif

ident db_lastrow(void) {
	return sqlite3_last_insert_rowid(c);
}
