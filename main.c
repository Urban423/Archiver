#include <stdio.h>
#include <string.h>
#include "archiver.h"

char is_letter(char letter)
{
	if(letter == '.' || letter == '/' || letter == '\\') {
		return 0;
	}
	return 1;
}

int main(int argc, char** argv)
{
	Archive archive;
	const char* file  		= NULL;
	const char* directory 	= NULL;
	char read = 0;
	char print = 0;
	char compression = 0;
	if(argc == 1)
	{
		printf("ERROR: no input data(use -help)\n");
		return -1;
	}
	for(int i = 0; i < argc; i++)
	{
		if(strcmp(argv[i], "-r") == 0) {
			read = 1;
		}
		else if(strcmp(argv[i], "-help") == 0) {
			printf("1) '-help' - will show you the list of commands\n");
			printf("2) '-r' - will make the program to restore archiph\n");
			printf("3) '-w' - will make the program to store archiph(used by default)\n");
			printf("4) '-d [directory_path]' - set the directory to the input/output directory\n");
			printf("5) '-f [file_path]' - set the directory to the input/output file\n");
			printf("6) '-p' - print the archive\n\n");
		} 
		else if(strcmp(argv[i], "-d") == 0) {
			directory = argv[++i];
		}
		else if(strcmp(argv[i], "-f") == 0) {
			file = argv[++i];
		}
		else if(strcmp(argv[i], "-p") == 0) {
			print = 1;
		}
		else if(strcmp(argv[i], "-c") == 0) {
			compression = 1;
		}
	}
	
	if(directory == NULL) {
		printf("ERROR: no input directory\n");
		return -1;
	}
	if(file == NULL && read == 1) {
		printf("ERROR: no input file path\n");
		return -1;
	}
	
	
	char check_input = 1;
	char* str = (char*)directory;
	while(1)
	{
		if(*str == 0) {
			break;
		}
		if(is_letter(*(str++))) {
			check_input = 0;
		}
	}
	if(check_input && read == 0) {
		printf("ERROR: input directory '%s' is incorrect\n", directory);
		return -1;
	}
	
	char* dir = (char*)directory;
	int directory_path_len = strlen(dir);
	if(dir[directory_path_len - 1] == '\\' || dir[directory_path_len - 1] == '/') {
		dir[directory_path_len - 1] = 0;
	}
	
	if(read) {
		archive = loadArchiveFromFile(file);
		if(print) {
			printArchive(&archive, directory);
		}
		saveArchiveAsDirectory(archive, directory);
	}
	else {
		archive = loadArchiveFromDirectory(directory, compression);
		if(print) {
			printArchive(&archive, directory);
		}
		saveArchiveAsFile(archive, file);
	}
	return 0;
	
};