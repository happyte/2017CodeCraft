#ifndef __LIB_IO_H__
#define __LIB_IO_H__

#define MAX_IN_NUM    (20000)
#define MAX_OUT_NUM    (2000)
#define MAX_LINE_LEN    (2000)

typedef struct opc_parameter{
	int lineNum;
	char** lines; 
} opc_parameter_t;

extern int read_file(char ** const buff, const unsigned int spec, const char * const filename);


extern void write_result(const char * const buff,const char * const filename);


extern void release_buff(char ** const buff, const int valid_item_num);

#endif

