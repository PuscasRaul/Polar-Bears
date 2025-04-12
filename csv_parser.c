#include <endian.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "Matrix.h"

#define COLUMN_SIZE 155

char* my_strtok(char* str, const char* delimiters) {
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

size_t load_keys(char *buffer, Matrix *matrix) {
	char *s = strtok(buffer, ",");
	while(s) {
		// read the keys
		matrix->key_columns[matrix->size] = malloc(strlen(s));

		if (!matrix->key_columns[matrix->size]) {
			fprintf(stderr, "allocating key in key_columns\n");
			return 1;
		}

		if (!memmove(matrix->key_columns[matrix->size], s, strlen(s))) {
			fprintf(stderr, "memmove error for key: %s\n", s);
			return 1;
		}

		// as much as we can, we do not know the type of the column yet
		matrix->column[matrix->size].size = 0;
		matrix->column[matrix->size].capacity = 64;

		++matrix->size;
		s = strtok(NULL, ",");
	}

	return 0;
}

size_t load_key_types(char *buffer, Matrix *matrix ) {
	char *s = strtok(buffer, ",");
	size_t column_index = 0;
	while(s) {
		// check for int first
		char *endptr;
		errno = 0;
		int int_value = strtol(s, &endptr, 10);
		
		// means there was conversion made
		if (endptr != s && errno != ERANGE && *endptr == '\0') {
			// fprintf(stderr, "CASE INT\n");
			matrix->column[column_index].type = TYPE_INT;
			matrix->column[column_index].values = 
				(int*) malloc(sizeof(int) * matrix->column[column_index].capacity);

			if (!matrix->column[column_index].values) {
				fprintf(stderr, "error allocating column cells for key: %s", 
							  matrix->key_columns[column_index]);
				return 1;
			}

			if (add_element(&matrix->column[column_index++], &int_value)) 
				return 1;
			
			s = strtok(NULL, ",");
			continue;
		}

		errno = 0;
		double double_value = strtod(s, &endptr);

		if (endptr != s && errno != ERANGE) {
			// fprintf(stderr, "CASE DOUBLE\n");
			matrix->column[column_index].type = TYPE_FLOAT;
			matrix->column[column_index].values = 
				(double*) malloc(sizeof(double) * matrix->column[column_index].capacity);

			if (!matrix->column[column_index].values) {
				fprintf(stderr, "error allocating column cells for key: %s", 
							  matrix->key_columns[column_index]);
				return 1;
			}
			
			if (add_element(&matrix->column[column_index++], &double_value)) 
				return 1;
			
			s = strtok(NULL, ",");
			continue;
		}

		// otherwise default to type string
		// fprintf(stderr, "CASE STRING \n");
		matrix->column[column_index].type = TYPE_STRING;
		matrix->column[column_index].values = 
			(char **) malloc(sizeof(char*) * matrix->column[column_index].capacity);

		if (!matrix->column[column_index].values) {
				fprintf(stderr, "error allocating column cells for key: %s", 
							  matrix->key_columns[column_index]);
				return 1;
		}

		if (add_element(&matrix->column[column_index++], s)) 
			return 1;
		
		s = strtok(NULL, ",");
	}

	return 0;
}

size_t read_line(char *buffer, Matrix *matrix) {
	char *s = my_strtok(buffer, ",");
	size_t column_index = 0;
	while (s) {
		char *endptr;
		switch(matrix->column[column_index].type) {
			case TYPE_INT:
				;
				int int_value;
				if (!strcmp(s, ""))
					int_value = INT_MIN;
				else int_value = strtol(s, &endptr, 10);
				if (add_element(&matrix->column[column_index], &int_value))
					return 1;
				break;
			case TYPE_FLOAT:
				;
				double double_value;
				if (!strcmp(s, ""))
					double_value = INFINITY;
				else double_value = strtod(s, &endptr);
				if (add_element(&matrix->column[column_index], &double_value))
					return 1;
				break;
			case TYPE_STRING:
				;
				if (add_element(&matrix->column[column_index], s))
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

Column *get_by_key(const Matrix *matrix, char *key) {
	for (size_t i = 0; i < matrix->size; i++) {
		if (!strcmp(matrix->key_columns[i], key)) {
			return &matrix->column[i];
		}
	}
	return NULL;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		perror("provide the path to the .csv file");
		return 1;
	}

	FILE *csv_file = fopen(argv[1], "r");
	if (!csv_file) {
		perror("could not open file");
		return 1;
	}

	// initialize the matrix and it's columns and key-number pairing array
	Matrix matrix;
	matrix.size = 0;
	matrix.column = (Column*) malloc(sizeof(Column) * MAX_COLUMNS);
	if (!matrix.column) {
		fprintf(stderr, "Error allocating matrix columns pointer\n");
		fclose(csv_file);
		return 1;
	}
	matrix.column->size = 0;

	matrix.key_columns = (char**) malloc(sizeof(char*) * MAX_COLUMNS);
	if (!matrix.key_columns) {
		fprintf(stderr, "Error allocating key number pairing");
		fclose(csv_file);
		free(matrix.column);
	}

	char buffer[1000];
	if (!fgets(buffer, 1000, csv_file)) {
		fprintf(stderr, "Error reading key line\n");
		fclose(csv_file);
		free(matrix.column);
	}

	if (load_keys(buffer, &matrix)) {
		fclose(csv_file);
		free_matrix(&matrix);
		return 1;
	}
	
	if (!fgets(buffer, 1000, csv_file)) {
		fprintf(stderr, "Error trying to read the second line\n");
		fclose(csv_file);
		free_matrix(&matrix);
		return 1;
	}

	if (load_key_types(buffer, &matrix)) {
		free_matrix(&matrix);
		fclose(csv_file);
	}

	while (fgets(buffer, 1000, csv_file)) {
		if (read_line(buffer, &matrix)) {
			free_matrix(&matrix);
			fclose(csv_file);
		}
	}

	for (size_t i = 0; i < matrix.size; i++) {
		switch(matrix.column[i].type) {
			case TYPE_INT:
				for (size_t j = 0; j < matrix.column[i].size; j++)
					printf("%d\n", ((int*) matrix.column[i].values)[j]);
				break;
			case TYPE_FLOAT:
				for (size_t j = 0; j < matrix.column[i].size; j++)
					printf("%f\n", ((double*) matrix.column[i].values)[j]);
				break;
			case TYPE_STRING:
				for (size_t j = 0; j < matrix.column[i].size; j++)
					printf("%s\n", ((char**) matrix.column[i].values)[j]);
				break;
			default:
				continue;
		}
	}

	for (size_t i = 0; i < matrix.size; i++) {
		printf("%s\n", matrix.key_columns[i]);
	}

	free_matrix(&matrix);
	return 0;
}

