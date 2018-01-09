#ifndef AMP_KERNAL_H
#define AMP_KERNAL_H

#include "amp.h"
#include <unistd.h>

#define  SERVER_PORT 5446

#define SERVER  1
#define CLIENT  2

#define SERVER_ID1   1
#define CLIENT_ID1   1

int server_id = SERVER_ID1;

//包含操作类型和文件名
//这里定义操作类型和type的定义
//0：创建文件
//1：读文件
//2：写文件
//3、截断文件truncate
//后两个的包中带一个page
//
//此外还有读写的范围
//此外，我们还需要一个空间来接受返回的元数据，对于数据的任何操作几乎都需要返回一个文件的元数据
//因为坚持将元数据从服务器端脱离出来，所以我们需要在不是服务器的地方管理元数据
//我们将元数据从服务器端拿回来，重新修改mode和gid、uid即可。
struct __fuse_msg {
	int type;
	char path_name[512];
    //truncate使用的文件截断大小
    off_t length;
    //write和read使用的
    size_t bytes;
    off_t offset;
    //这里存着当前发送需要的段大小
    size_t page_size_now;
    
    struct stat server_stat;
};
typedef struct __fuse_msg  fuse_msg_t;


#endif