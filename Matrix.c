#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "Matrix.h"

#define MAX_COLUMNS 155

int init_matrix(Matrix *matrix) {
	matrix->size = 0;
	matrix->column = calloc(MAX_COLUMNS, sizeof(Column));
	if (!matrix->column) 
		return 1;

	for (size_t i = 0; i < MAX_COLUMNS; i++) {
		matrix->column[i].capacity = 256;
		matrix->column[i].size = 0;
	}

	matrix->key_columns = calloc(MAX_COLUMNS, sizeof(char*));
	if (!matrix->key_columns) { 
		free(matrix->column);
		return 1;
	}
	
	return 0;
} 

// free's the matrix and all it's containg columns
// no return of any sort if anything fails it's seg fault
void free_matrix(Matrix *matrix) {
	for (size_t i = 0; i < matrix->size; i++) {
		switch(matrix->column[i].type) {
			case TYPE_INT:
				free(matrix->column[i].values);
				matrix->column[i].size = 0;
				matrix->column[i].capacity = -1;
				free(matrix->key_columns[i]);
				continue;
			case TYPE_FLOAT:
				free(matrix->column[i].values);
				matrix->column[i].size = 0;
				matrix->column[i].capacity = -1;
				free(matrix->key_columns[i]);
				continue;
			case TYPE_STRING:
				for (size_t j = 0; j < matrix->column[i].size; j++)
					free(((char**) matrix->column[i].values)[j]);
				free(matrix->column[i].values);
				matrix->column[i].size = 0;
				matrix->column[i].capacity = -1;
				free(matrix->key_columns[i]);
				continue;
			default:
				continue;
		}
	}

	free(matrix->column);
	free(matrix->key_columns);
}

// resizes a columns values field to 2 * capacity
// returns 1 on some sort of error
// 0 on success
static size_t resize(Column *column) {
	column->capacity *= 2;
	switch (column->type) {
		case TYPE_INT:
			;
			int *temp_int_values = 
				realloc((int*) column->values, column->capacity * sizeof(int));
			if (!temp_int_values) 
				return 1;
			column->values = temp_int_values;
			return 0;
		case TYPE_FLOAT:
			;
			double *temp_double_values = 
				realloc((double*) column->values, column->capacity * sizeof(double));
			if (!temp_double_values)
				return 1;
			column->values = temp_double_values;
			return 0;
		case TYPE_STRING:
			;
			char **temp_string_values =
				realloc((char**) column->values, column->capacity * sizeof(char*));
			if (!temp_string_values)
				return 1;
			column->values = temp_string_values;
			return 0;
		default:
			return 1;
	}
}

size_t add_element(Column *column, void *element) {
	if (column->size >= column->capacity) 
		if (resize(column))
			return 1;

	switch (column->type) {
		case TYPE_INT:
			;
			int *int_value = (int*) element;
			((int*) (column->values))[column->size] = *int_value;
			++column->size;
			return 0;
		case TYPE_FLOAT:
			;
			double *double_value = (double*) element;
			((double*) (column->values))[column->size] = *double_value;
			++column->size;
			return 0;
		case TYPE_STRING:
			;
			char *string = (char*) element;
			((char**) (column->values))[column->size] = 
				malloc (sizeof(char) * strlen(string) + 1);
			memmove(((char**) (column->values))[column->size], 
					string, strlen(string));
			++column->size;
			return 0;
		default:
			return 1;
	}
}

Column *get_by_key(const Matrix *matrix,const char *key) {
	for (size_t i = 0; i < matrix->size; i++) {
		if (!strcmp(matrix->key_columns[i], key)) {
			return &matrix->column[i];
		}
	}
	return NULL;
}
