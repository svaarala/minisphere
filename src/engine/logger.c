#include "minisphere.h"
#include "logger.h"

#include "lstring.h"

struct logger
{
	unsigned int      refcount;
	unsigned int      id;
	sfs_file_t*       file;
	int               num_blocks;
	int               max_blocks;
	struct log_block* blocks;
};

struct log_block
{
	lstring_t* name;
};

static unsigned int s_next_logger_id = 0;

logger_t*
open_log_file(const char* filename)
{
	lstring_t* log_entry;
	logger_t*  logger = NULL;
	time_t     now;
	char       timestamp[100];

	console_log(2, "creating logger #%u for `%s`", s_next_logger_id, filename);
	
	logger = calloc(1, sizeof(logger_t));
	if (!(logger->file = sfs_fopen(g_fs, filename, NULL, "a")))
		goto on_error;
	time(&now);
	strftime(timestamp, 100, "%a %Y %b %d %H:%M:%S", localtime(&now));
	log_entry = lstr_newf("LOG OPENED: %s\n", timestamp);
	sfs_fputs(lstr_cstr(log_entry), logger->file);
	lstr_free(log_entry);
	
	logger->id = s_next_logger_id++;
	return ref_logger(logger);

on_error: // oh no!
	console_log(2, "failed to open file for logger #%u", s_next_logger_id++);
	free(logger);
	return NULL;
}

logger_t*
ref_logger(logger_t* logger)
{
	++logger->refcount;
	return logger;
}

void
free_logger(logger_t* logger)
{
	lstring_t* log_entry;
	time_t     now;
	char       timestamp[100];

	if (logger == NULL || --logger->refcount > 0)
		return;

	console_log(3, "disposing logger #%u no longer in use", logger->id);
	time(&now); strftime(timestamp, 100, "%a %Y %b %d %H:%M:%S", localtime(&now));
	log_entry = lstr_newf("LOG CLOSED: %s\n\n", timestamp);
	sfs_fputs(lstr_cstr(log_entry), logger->file);
	lstr_free(log_entry);
	sfs_fclose(logger->file);
	free(logger);
}

bool
begin_log_block(logger_t* logger, const char* title)
{
	lstring_t*        block_name;
	struct log_block* blocks;
	int               new_count;
	
	new_count = logger->num_blocks + 1;
	if (new_count > logger->max_blocks) {
		if (!(blocks = realloc(logger->blocks, new_count * 2))) return false;
		logger->blocks = blocks;
		logger->max_blocks = new_count * 2;
	}
	if (!(block_name = lstr_newf("%s", title))) return false;
	write_log_line(logger, "BEGIN", lstr_cstr(block_name));
	logger->blocks[logger->num_blocks].name = block_name;
	++logger->num_blocks;
	return true;
}

void
end_log_block(logger_t* logger)
{
	lstring_t* block_name;
	
	--logger->num_blocks;
	block_name = logger->blocks[logger->num_blocks].name;
	write_log_line(logger, "END", lstr_cstr(block_name));
	lstr_free(block_name);
}

void
write_log_line(logger_t* logger, const char* prefix, const char* text)
{
	time_t now;
	char   timestamp[100];
	
	int i;
	
	time(&now);
	strftime(timestamp, 100, "%a %Y %b %d %H:%M:%S -- ", localtime(&now));
	sfs_fputs(timestamp, logger->file);
	for (i = 0; i < logger->num_blocks; ++i)
		sfs_fputc('\t', logger->file);
	if (prefix != NULL) {
		sfs_fputs(prefix, logger->file);
		sfs_fputc(' ', logger->file);
	}
	sfs_fputs(text, logger->file);
	sfs_fputc('\n', logger->file);
}
