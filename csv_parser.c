#include <ctype.h>
#include <endian.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "csv_parser.h"

struct csv_parser *create_parser(
		const char *file_path, 
		int (*read_csv)(struct csv_parser *),
		void (*destroy_parser)(struct csv_parser*)
) {
		struct csv_parser *parser = malloc(sizeof(struct csv_parser));
		if (!parser) 
			return NULL;
		parser->csv_file = strdup(file_path); 
		parser->read_csv = read_csv;
		parser->destroy_parser = destroy_parser;
		init_matrix(&parser->matrix);
		return parser;
}

void destroy_parser(struct csv_parser *parser) {
	free_matrix(&parser->matrix);
	free(parser->csv_file);
	free(parser);
}

static char *my_strtok(char* str, const char* delimiters) {
    static char* saved_token = NULL;
    char* token_start;

    if (str != NULL) {
        saved_token = str;
    }

    if (saved_token == NULL || *saved_token == '\0') {
        return NULL;
    }

    token_start = saved_token;

    if (strchr(delimiters, *token_start)) {
        saved_token++;
        *token_start = '\0';
        return token_start;
    }

    saved_token = token_start;
    while (*saved_token != '\0' && !strchr(delimiters, *saved_token)) {
        saved_token++;
    }

    if (*saved_token != '\0') {
        *saved_token = '\0';
        saved_token++;
    } else {
        saved_token = NULL;
    }

    return token_start;
}

static size_t load_keys(char *buffer, struct csv_parser *parser) {
	char *s = my_strtok(buffer, ",");
	while(s) {
		// read the keys
		parser->matrix.key_columns[parser->matrix.size] = malloc((strlen(s) + 1) * sizeof(char));

		if (!parser->matrix.key_columns[parser->matrix.size]) {
			fprintf(stderr, "allocating key in key_columns\n");
			return 1;
		}

		if (!memmove(parser->matrix.key_columns[parser->matrix.size], s, strlen(s))) {
			fprintf(stderr, "memmove error for key: %s\n", s);
			return 1;
		}

		int index = strlen(s) - 1;
		while (!isalnum(parser->matrix.key_columns[parser->matrix.size][index]) 
					 && isspace(parser->matrix.key_columns[parser->matrix.size][index])) {
			parser->matrix.key_columns[parser->matrix.size][index] = index + 1;
			--index;
		}
		parser->matrix.key_columns[parser->matrix.size][index + 1] = '\0';

		++parser->matrix.size;
		s = my_strtok(NULL, ",");
	}

	return 0;
}

static size_t load_key_types(char *buffer, struct csv_parser *parser) {
	char *s = my_strtok(buffer, ",");
	size_t column_index = 0;
	while(s) {
		// check for int first
		char *endptr;
		errno = 0;
		int int_value = strtol(s, &endptr, 10);
		
		// means there was conversion made
		if (endptr != s && errno != ERANGE && *endptr == '\0') {
			// fprintf(stderr, "CASE INT\n");
			parser->matrix.column[column_index].type = TYPE_INT;
			parser->matrix.column[column_index].values = 
				(int*) malloc(sizeof(int) * parser->matrix.column[column_index].capacity);

			if (!parser->matrix.column[column_index].values) {
				fprintf(stderr, "error allocating column cells for key: %s", 
							  parser->matrix.key_columns[column_index]);
				return 1;
			}

			if (add_element(&parser->matrix.column[column_index++], &int_value)) 
				return 1;
			
			s = my_strtok(NULL, ",");
			continue;
		}

		errno = 0;
		double double_value = strtod(s, &endptr);

		if (endptr != s && errno != ERANGE) {
			// fprintf(stderr, "CASE DOUBLE\n");
			parser->matrix.column[column_index].type = TYPE_FLOAT;
			parser->matrix.column[column_index].values = 
				(double*) malloc(sizeof(double) * parser->matrix.column[column_index].capacity);

			if (!parser->matrix.column[column_index].values) {
				fprintf(stderr, "error allocating column cells for key: %s", 
							  parser->matrix.key_columns[column_index]);
				return 1;
			}
			
			if (add_element(&parser->matrix.column[column_index++], &double_value)) 
				return 1;
			
			s = my_strtok(NULL, ",");
			continue;
		}

		// otherwise default to type string
		// fprintf(stderr, "CASE STRING \n");
		parser->matrix.column[column_index].type = TYPE_STRING;
		parser->matrix.column[column_index].values = 
			(char **) malloc(sizeof(char*) * parser->matrix.column[column_index].capacity);

		if (!parser->matrix.column[column_index].values) {
				fprintf(stderr, "error allocating column cells for key: %s", 
							  parser->matrix.key_columns[column_index]);
				return 1;
		}

		if (add_element(&parser->matrix.column[column_index++], s)) 
			return 1;
		
		s = my_strtok(NULL, ",");
	}

	return 0;
}

static size_t read_line(char *buffer, struct csv_parser *parser) {
	char *s = my_strtok(buffer, ",");
	size_t column_index = 0;
	while (s) {
		char *endptr;
		switch(parser->matrix.column[column_index].type) {
			case TYPE_INT:
				;
				int int_value;
				if (!strcmp(s, ""))
					int_value = INT_MIN;
				else int_value = strtol(s, &endptr, 10);
				if (add_element(&parser->matrix.column[column_index], &int_value))
					return 1;
				break;
			case TYPE_FLOAT:
				;
				double double_value;
				if (!strcmp(s, ""))
					double_value = INFINITY;
				else double_value = strtod(s, &endptr);
				if (add_element(&parser->matrix.column[column_index], &double_value))
					return 1;
				break;
			case TYPE_STRING:
				;
				if (add_element(&parser->matrix.column[column_index], s))
					return 1;
				break;
			default:
				break;
		}
		++column_index;
		s = my_strtok(NULL, ",");
	}
	return 0;
}

int read_csv(struct csv_parser *parser) {
	FILE *csv_file = fopen(parser->csv_file, "r");
	if (!csv_file) {
		perror("could not open file");
		return 1;
	}

	char buffer[1000];
	if (!fgets(buffer, 1000, csv_file)) {
		fprintf(stderr, "Error reading key line\n");
		fclose(csv_file);
		return 1;
	}

	if (load_keys(buffer, parser)) {
		fclose(csv_file);
		return 1;
	}
	
	if (!fgets(buffer, 1000, csv_file)) {
		fprintf(stderr, "Error trying to read the second line\n");
		fclose(csv_file);
		return 1;
	}

	if (load_key_types(buffer, parser)) {
		fclose(csv_file);
	}

	while (fgets(buffer, 1000, csv_file)) {
		if (read_line(buffer, parser)) {
			fclose(csv_file);
		}
	}
	fclose(csv_file);
	return 0;
}

