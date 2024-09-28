#include "archiver.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>



unsigned int hash4ByteString(const char* str) {
    return *str + (str[1] << 8) + (str[2] << 16) + (str[3] << 24);
}

//stinrg table
int addWord(string_table* table, char* word)
{
	//find duplicate
	for(int i = 0; i < table->size; i++)
	{
		char* symbA = table->array + table->indicies[i];
		char* symbB = word;
		while(1)
		{
			if(*symbA != *(symbB++)) { 	break;}
			if(*(symbA++) == 0) { return i;}
		}
	}
	

    int wordLength = strlen(word) + 1;
    char* newArray = realloc(table->array, table->size_in_bytes + wordLength);
    if (newArray == NULL) {
        printf("Memory allocation error!\n");
        return -1;
    }
    table->array = newArray;
    strcpy(&table->array[table->size_in_bytes], word); 
	
	
	unsigned int* newOffsetArray = realloc(table->indicies, (table->size + 1) * sizeof(int));
	if(newOffsetArray == NULL)
	{
        printf("Memory allocation error!\n");
		return -1;
	}
	table->indicies = newOffsetArray;
	if(table->size == 0) {
		*table->indicies = 0; 
	}
	else {
		table->indicies[table->size] = table->size_in_bytes; 
	}
	
    table->size_in_bytes += wordLength;
	table->size++;
    return table->size - 1;
}

//directory table
void addDirectory(directory_table* dTable, directory_node node)
{
	directory_node* newArray = realloc(dTable->array, (dTable->size + 1) * sizeof(directory_node));
    
    if (newArray == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }
	
    dTable->array = newArray;
    dTable->array[dTable->size] = node;
    dTable->size++;
}

//File sytem
void addFile(file_table* file_manager, FILE* new_file)
{
	FILE** newArray = realloc(file_manager->array, (file_manager->size + 1) * sizeof(FILE*));
    
    if (newArray == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }
	
    file_manager->array = newArray;
    file_manager->array[file_manager->size] = new_file;
    file_manager->size++;
}

//metadata table
void addMetadata(metadata_table* mTable, metadata data)
{
	 metadata* newArray = realloc(mTable->array, (mTable->size + 1) * sizeof(metadata));

    if (newArray == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }
	
    mTable->array = newArray;
    mTable->array[mTable->size] = data;
    mTable->size++;
}







//read directories
char ListDirectoryContents(
    const char* path, int* max_length,
    string_table* sTable, directory_table* dTable, metadata_table* mTable, file_table* fTable,
    unsigned int parent_index, int last_size) 
{
    DIR *dir;
    struct dirent *entry;
    struct stat fileInfo;
    char subDir[PATH_MAX];

    if (!(dir = opendir(path))) {
        return FILE_ERROR;
    }

    while ((entry = readdir(dir)) != NULL) {
        char *name = entry->d_name;
        if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
            int word_length = last_size + strlen(name);
            if (*max_length < word_length) {
                *max_length = word_length;
            }

            int name_index = addWord(sTable, name);
            snprintf(subDir, sizeof(subDir), "%s/%s", path, name);
            if (stat(subDir, &fileInfo) == 0) {
                if (S_ISDIR(fileInfo.st_mode)) {
                    directory_node dNode = {name_index, 0, parent_index};
                    addDirectory(dTable, dNode);
                    dTable->array[parent_index].size++;

                    ListDirectoryContents(subDir, max_length, sTable, dTable, mTable, fTable, dTable->size - 1, word_length + 1);
                } else {
                    FILE *f = fopen(subDir, "rb");
                    if (f == NULL) {
                        closedir(dir);
                        return FILE_ERROR;
                    }

                    fseek(f, 0, SEEK_END);
                    unsigned int fileSize = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    addFile(fTable, f);

					struct stat premission_stut;
					stat(subDir, &premission_stut);
					mode_t permission = premission_stut.st_mode;
                    metadata data = {name_index, parent_index, 0, fileSize, 0, permission, 0, 0};
                    addMetadata(mTable, data);
                }
            }
        }
    }

    closedir(dir); 
    return FILE_OK;
}



//restore paths to files
char* buildPathReverse(string_table* sTable, directory_table* dTable, metadata meta, const char* path_start, char* buffer, int buffer_size) 
{
    char* pathEnd = buffer + buffer_size - 1;
    *pathEnd = '\0';
    int currentIndex = meta.directory_index;
	const char* fileName = sTable->array + sTable->indicies[meta.name_index];
	size_t fileNameLen = strlen(fileName);
    pathEnd -= fileNameLen;
    strncpy(pathEnd, fileName, fileNameLen);
    pathEnd--;
    *pathEnd = DEFAULT_SOLIDIUS;
	
    while (currentIndex != -1) {
		const char* dirName = sTable->array + sTable->indicies[dTable->array[currentIndex].name_index];
        size_t nameLen = strlen(dirName);
        pathEnd -= nameLen;
        strncpy(pathEnd, dirName, nameLen);
		
        pathEnd--;
        *pathEnd = DEFAULT_SOLIDIUS;
		
        currentIndex = dTable->array[currentIndex].parent_index;
    }
	
	fileNameLen = strlen(path_start);
    pathEnd -= fileNameLen;
	strncpy(pathEnd, path_start, fileNameLen);

    return pathEnd;
}

//read string from archive
void readStringTable(string_table* table, int size, FILE* f)
{
	table->size_in_bytes = size;
	table->array = malloc(size);
	int e = fread(table->array, size, 1, f);
	for(int i = 0; i < size; i++) {
		if(table->array[i] == 0) {
			table->size++;
		}
	}
	int index = 0;
	table->indicies = malloc(size * 4);
	table->indicies[index++] = 0;
	for(int i = 0; i < size; i++) {
		if(table->array[i] == 0) {
			table->indicies[index++] = i + 1;
		}
	}
}

//read directories from archive
void readStandartTable(directory_table* table, int size, int value_size, FILE* f)
{
	table->size = size / value_size;
	table->array = malloc(size);
	int e = fread(table->array, size, 1, f);
}

//restore directory from archive data
void restoreDirectory(char* path, int size, int max_length, directory_table* dTable, string_table* sTable, int* index, char solidus)
{
	const char* dir_name = sTable->array + sTable->indicies[dTable->array[*index].name_index];
	int str_len = strlen(dir_name);
	char new_path[max_length];
	snprintf(new_path, max_length, "%s%c%s", path, solidus, dir_name);
	struct stat st = {0};
	if (stat(new_path, &st) == -1) {
		int e = makeDirectory(new_path);
	}


	int number_of_subs = dTable->array[*index].size;
	for (int i = 0; i < number_of_subs; i++) {
		(*index)++;
		restoreDirectory(new_path, size + str_len + 1, max_length, dTable, sTable, index, solidus);
	}
}

//restore files from archive
void restoreFiles(char* buffer, char* uncompressed_buffer, head_table* hTable, string_table* sTable, directory_table* dTable, metadata_table* mTable, const char* path_start, char* path, int path_start_len)
{
	char* l;
	FILE* f;
	for(int i = 0; i < mTable->size; i++)
	{
		if(mTable->array[i].size == 0) { 
			l = buildPathReverse(sTable, dTable,  mTable->array[i], path_start,  path, hTable->max_length + path_start_len);
			f = fopen(l, "wb");
			fclose(f);
			continue;
		}
		//uncompress data
		
		
		long unsigned int decompressed_len = mTable->array[i].size;
		if(mTable->array[i].compression) {
			int e = uncompress(uncompressed_buffer, &decompressed_len, buffer + mTable->array[i].offset, mTable->array[i].compressed_size);
			if (e != Z_OK) {
				printf("Decompression failed with error code %d\n", e);
				return;
			}
		}
		else {
			uncompressed_buffer = buffer + mTable->array[i].offset;
		}
		//printf("%s %d %ld\n", uncompressed_buffer, mTable->array[i].compressed_size, decompressed_len);





		l = buildPathReverse(sTable, dTable,  mTable->array[i], path_start,  path, hTable->max_length + path_start_len);
		f = fopen(l, "wb");
		if(f == NULL) {continue; }

		int fd = fileno(f);
		if (fd == -1) {
			perror("fileno");
			fclose(f);
			continue;
		}



		//if (chmod(l, mTable->array[i].permission) == -1) {
		//	perror("chmod");
		//	fclose(f);
		//	continue;
		//}
		fwrite(uncompressed_buffer, decompressed_len, 1, f);
		fclose(f);
	}
}









Archive loadArchiveFromDirectory(const char* directory, char compression)
{

	long long 			signature 	= 0x4352416E61627255;
	unsigned int 		tag 		= 0;
	unsigned int 		total_size	= 0;
	unsigned int 		head_size	= 0;
	int 				max_length 	= 0;
	offset_table*		Tables		= NULL;
	head_table	 		hTable 		= {0, 3, 0, 0x10};
	directory_table 	dTable 		= {0, NULL};
	string_table 		sTable 		= {0, 0, NULL, NULL};
	metadata_table 		mTable		= {0, NULL};
	file_table			fTable		= {0, NULL};
	int*			stringOffset	= NULL;
	directory_node 		dNode 		= {0, 0, -1};


	//deflate
	char* str = (char*)directory;
	char* start_of_path = 0;
	while(1) {
		if(*str == 0) { break;}
		if(*(str++) == DEFAULT_SOLIDIUS) {start_of_path = str;}
	}
	addWord(&sTable, start_of_path);
	addDirectory(&dTable, dNode);
	ListDirectoryContents(directory, &max_length, &sTable, &dTable, &mTable, &fTable, 0, strlen(directory) - 1);
	hTable.max_length = max_length;
	hTable.offset_table_offset = 8 + sizeof(head_table);
	
	
	Tables = (offset_table*)malloc(sizeof(offset_table) * hTable.number_of_tables);
	Tables[0].tag = hash4ByteString("strT");
	Tables[1].tag = hash4ByteString("dirT");
	Tables[2].tag = hash4ByteString("meta");
	
	
	
	head_size += 8; //signature
	head_size += sizeof(head_table); //head_table
	head_size += hTable.number_of_tables * sizeof(offset_table); //offset tables
	head_size += sTable.size_in_bytes; //string table
	head_size += dTable.size * sizeof(directory_node); // directory table
	head_size += mTable.size * sizeof(metadata); // metadata table
	//files already inside
	

	for(int i = 0; i < mTable.size; i++) {
        if (mTable.array[i].size == 0) {
            mTable.array[i].compressed_size = 0;
			continue;
        }
		mTable.array[i].compression = compression;
		int file_size = 0;
		if(compression) {
			 mTable.array[i].compressed_size = compressBound(mTable.array[i].size);
			 total_size += mTable.array[i].compressed_size;
		}
		else {
			total_size +=  mTable.array[i].size;
		}
		
		if(hTable.max_file_size < mTable.array[i].size) {
			hTable.max_file_size = mTable.array[i].size;
		}
		if(hTable.max_file_size < mTable.array[i].compressed_size) {
			hTable.max_file_size = mTable.array[i].compressed_size;
		}
    }
	char* buffer = malloc(total_size);
	char* read_buffer = malloc(hTable.max_file_size);
	int byte_offset = 0;
	for(int i = 0; i < mTable.size; i++) {
		if(mTable.array[i].size == 0) { continue;}
		mTable.array[i].offset = byte_offset;
		int e = fread(read_buffer, mTable.array[i].size, 1, fTable.array[i]);
		long unsigned int compressed_file_size = mTable.array[i].compressed_size;
		if(mTable.array[i].compression) {
			e = compress(buffer + byte_offset, &compressed_file_size, (const unsigned char*)read_buffer, mTable.array[i].size);
			byte_offset += compressed_file_size;
			if (e != Z_OK) {
				printf("Compression failed with error code %d\n", e);
			}
			mTable.array[i].compressed_size = compressed_file_size;
		}
		else {
			memcpy(buffer + byte_offset, read_buffer, mTable.array[i].size);
			mTable.array[i].compressed_size = mTable.array[i].size;
			byte_offset += mTable.array[i].size;
		}
		fclose(fTable.array[i]);
	};
	total_size = byte_offset;
	
	Tables[2].size 		= mTable.size * sizeof(metadata);
	Tables[2].offset 	= head_size - Tables[2].size;
	Tables[1].size 		= dTable.size * sizeof(directory_node);
	Tables[1].offset 	= Tables[2].offset - Tables[1].size;
	Tables[0].size 		= sTable.size_in_bytes;
	Tables[0].offset 	= Tables[1].offset - Tables[0].size;

	
	hTable.total_files_size = total_size;
	hTable.total_size = head_size + hTable.total_files_size;
	hTable.offset_table_offset = Tables[0].offset - sizeof(offset_table) * hTable.number_of_tables;
	
	free(fTable.array);
	Archive res = {signature, hTable, Tables, sTable, dTable, mTable , buffer};
	return res;
}

Archive loadArchiveFromFile(const char* file)
{
	long long 			signature 	= 0;
	unsigned int 		tag 		= 0;
	unsigned int 		total_size	= 0;
	unsigned int 		head_size	= 0;
	int 				max_length 	= 0;
	offset_table*		Tables		= NULL;
	head_table	 		hTable 		= {0, 3, 0, 0x10};
	directory_table 	dTable 		= {0, NULL};
	string_table 		sTable 		= {0, 0, NULL, NULL};
	metadata_table 		mTable		= {0, NULL};
	file_table			fTable		= {0, NULL};
	int*			stringOffset	= NULL;
	directory_node 		dNode 		= {0, 0, -1};
	
	FILE* f = fopen(file, "rb");
	if(f == NULL) {
		printf("ERROR: file '%s' no found\n", file);
		Archive er;
		return er;
	}
	fseek(f, 0, SEEK_SET);
	int e = fread(&signature, 8, 1, f); //signature
	e = fread(&hTable, sizeof(head_table), 1, f); //headTable
	Tables = malloc(sizeof(offset_table) * hTable.number_of_tables); // offset tables
	fseek(f, hTable.offset_table_offset, SEEK_SET);
	e = fread(Tables, sizeof(offset_table) * hTable.number_of_tables, 1, f);
	
	for(int i = 0; i < hTable.number_of_tables; i++) {
		fseek(f, Tables[i].offset, SEEK_SET);
		switch(Tables[i].tag)
		{
			case(1416787059):
			{
				readStringTable(&sTable, Tables[i].size, f);
				break;
			}
			case(1416784228):
			{
				readStandartTable(&dTable, Tables[i].size, sizeof(directory_node), f);
				break;
			}
			case(1635018093):
			{
				readStandartTable((directory_table*)(&mTable), Tables[i].size, sizeof(metadata), f);
				break;
			}
		}
	}
	int  offset = 0;
	char* buffer = malloc(hTable.total_files_size);
	for(int i = 0; i < mTable.size; i++)
	{
		fseek(f, mTable.array[i].offset, SEEK_SET);
		e = fread(buffer + offset, mTable.array[i].compressed_size, 1, f);
		offset += mTable.array[i].compressed_size;
		mTable.array[i].offset -= hTable.total_size - hTable.total_files_size;
	}
	
	fclose(f);
	hTable.max_length += strlen(file) + 1;
	
	
	Archive res = {signature, hTable, Tables, sTable, dTable, mTable, buffer};
	return res;
}

void saveArchiveAsFile(Archive archive, const char* file)
{
	long long 			signature 	= archive.signature;
	offset_table*		Tables		= archive.Tables;
	head_table	 		hTable 		= archive.hTable;
	directory_table 	dTable 		= archive.dTable;
	string_table 		sTable 		= archive.sTable;
	metadata_table 		mTable		= archive.mTable;
	
	int head_size = 8; //signature
	head_size += sizeof(head_table); //head_table
	head_size += hTable.number_of_tables * sizeof(offset_table); //offset tables
	head_size += sTable.size_in_bytes; //string table
	head_size += dTable.size * sizeof(directory_node); // directory table
	head_size += mTable.size * sizeof(metadata); // metadata table
	
	char* buffer = malloc(archive.hTable.total_size);
	memcpy(buffer + head_size, archive.bufferFileData, archive.hTable.total_files_size);
	memcpy(buffer, &signature,	8); 	// signature
	
	int byte_offset = head_size;
	for(int i = 0; i < mTable.size; i++) {
		mTable.array[i].offset = byte_offset;
		byte_offset += mTable.array[i].compressed_size; 
	};
	memcpy(buffer + Tables[2].offset, mTable.array, Tables[2].size);	// metadata
	memcpy(buffer + Tables[1].offset, dTable.array, Tables[1].size);	// directory table
	memcpy(buffer + Tables[0].offset, sTable.array, Tables[0].size);	// string table
	memcpy(buffer  + hTable.offset_table_offset, Tables, 	sizeof(offset_table) * hTable.number_of_tables);	// offset tables
	memcpy(buffer  + 8, &hTable, 	sizeof(head_table));	// head table
	
	FILE* result_file = fopen(file, "wb");
	fwrite(buffer, archive.hTable.total_size, 1, result_file);
	fclose(result_file);
}

void saveArchiveAsDirectory(Archive archive, const char* directory)
{
	int dir_len = strlen(directory);
	int max_length = archive.hTable.max_length + dir_len + 2;
	char* path = malloc(max_length);
	char* uncompressed_buffer = malloc(archive.hTable.max_file_size);

	memcpy(path, directory, dir_len);
	path[dir_len] = '\0';
	struct stat st = {0};
	if (stat(path, &st) == -1) {
		int e = makeDirectory(path);
	}

	path[dir_len] = DEFAULT_SOLIDIUS;
	path[dir_len] = '\0';

	int index = 0;
	restoreDirectory(path, dir_len, archive.hTable.max_length, &archive.dTable, &archive.sTable, &index, DEFAULT_SOLIDIUS);
	restoreFiles(archive.bufferFileData, uncompressed_buffer, &archive.hTable, &archive.sTable, &archive.dTable, &archive.mTable, directory, path, dir_len);

	free(path);
}



void printArchive(Archive* archive, const char* path_start)
{
	int path_start_len = strlen(path_start) + 2;
	char* buffer = malloc(path_start_len + archive->hTable.max_length);
	
	printf("%d\n", (int)archive->signature);
	printf("%d %d %d %d %d %d %d\n",  archive->hTable.offset_table_offset,  archive->hTable.number_of_tables,  archive->hTable.max_length, archive->hTable.max_file_size, archive->hTable.total_size, archive->hTable.total_files_size,  archive->hTable.version);
	for(int i = 0; i <  archive->hTable.number_of_tables; i++)
	{
		printf("%d %d %d\n",  archive->Tables[i].tag,  archive->Tables[i].size, archive->Tables[i].offset);
	}
	for(int i = 0; i <  archive->sTable.size; i++)
	{
		printf("%s ",  archive->sTable.array +  archive->sTable.indicies[i]);
	}
	printf("\n");
	for(int i = 0; i <  archive->dTable.size; i++)
	{
		printf("%d %d %d\n",  archive->dTable.array[i].name_index,  archive->dTable.array[i].size,  archive->dTable.array[i].parent_index);
	}
	for(int i = 0; i <  archive->mTable.size; i++)
	{
		printf("%d %d %d %d %d %d %d %d\n",  archive->mTable.array[i].name_index,   archive->mTable.array[i].directory_index,  archive->mTable.array[i].offset,  archive->mTable.array[i].size,  archive->mTable.array[i].compressed_size, archive->mTable.array[i].permission, archive->mTable.array[i].type, archive->mTable.array[i].compression  );
	}
	for(int i = 0; i <  archive->mTable.size; i++)
	{
		char* l = buildPathReverse(& archive->sTable, & archive->dTable,  archive->mTable.array[i], path_start,  buffer, archive->hTable.max_length + path_start_len);
		printf("%s %d\n", l, (int)strlen(l));
	}
	free(buffer);
	printf("\n");
}
