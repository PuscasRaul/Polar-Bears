#include "Matrix.h"
#include <stdio.h>

#ifndef CSV_PARSER_H
#define CSV_PARSER_H

struct csv_parser {
	Matrix matrix;
	char *csv_file;

	int (*read_csv)(struct csv_parser *);
	void (*destroy_parser)(struct csv_parser *);
};

struct csv_parser *create_parser(
		const char *file, 
		int (*read_csv)(struct csv_parser *), 
		void (*destroy_parser)(struct csv_parser*));

int read_csv(struct csv_parser *parser);
void destroy_parser(struct csv_parser *parser);

#endif


