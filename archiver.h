#pragma once
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>


#ifdef windows
	#define makeDirectory(X) mkdir(X)
#else 
	#define makeDirectory(X) mkdir(X, 0700)
#endif


#define DEFAULT_SOLIDIUS '/'
#define FILE_OK 0
#define FILE_ERROR 1

/*
	file formata:
		1) signature
		2) head_table
		3) offset_table
		4) string_table
		5) directory_table
		6) metadata_table
		7) files
		
*/

//structures
typedef struct head_table {
	unsigned int offset_table_offset;
	unsigned int number_of_tables;
	unsigned int max_length;
	unsigned int max_file_size;
	unsigned int total_size;
	unsigned int total_files_size;
	unsigned char version;
}head_table;

typedef struct offset_table {
	unsigned int tag;
    unsigned int offset;
	unsigned int size;
} offset_table;

typedef struct string_table {
    unsigned int size_in_bytes;
	unsigned int size;
    char* array;
	unsigned int*  indicies;
} string_table;

typedef struct directory_node {
	unsigned int name_index;
	unsigned int size;
    int parent_index;
} directory_node;

typedef struct directory_table {
	unsigned int size;
	directory_node* array;
} directory_table;

typedef struct metadata {
	unsigned int name_index;
	unsigned int directory_index;
	unsigned int offset;
	unsigned int size;
	unsigned int compressed_size;
	unsigned int permission;
	unsigned char type;
	unsigned char compression;
} metadata;

typedef struct metadata_table {
	unsigned int size;
	metadata* array;
} metadata_table;

typedef struct file_table {
	unsigned int size;
	FILE** array;
} file_table;

typedef struct Archive {
	long long signature;
	head_table 		hTable;
	offset_table* 	Tables;
	string_table 	sTable;
	directory_table dTable;
	metadata_table 	mTable;
	char* bufferFileData;
} Archive;


Archive loadArchiveFromDirectory(const char* directory, char compression);
Archive loadArchiveFromFile		(const char* file);
void 	saveArchiveAsFile		(Archive  archive, const char* file);
void	saveArchiveAsDirectory	(Archive  archive, const char* directory);
void 	printArchive			(Archive* archive, const char* path_start);