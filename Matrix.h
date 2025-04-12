#include <stddef.h>
#include <sys/types.h>

typedef enum {TYPE_STRING, TYPE_INT, TYPE_FLOAT} ColumnType;
#define MAX_COLUMNS 400 
#define COLUMN_AS(type, col) ((type*)(col).values)

typedef struct {
	ColumnType type;
	void *values;
	size_t size;
	size_t capacity;
} Column;

typedef struct {
	char **key_columns;
	Column *column;
	size_t size;
} Matrix;

void free_matrix(Matrix *matrix);

size_t resize(Column *column);

size_t add_element(Column *column, void *element); 
 
Column *get_by_key(const Matrix *matrix, char *key);
