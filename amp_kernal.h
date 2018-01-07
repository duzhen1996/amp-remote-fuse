#ifndef AMP_KERNAL_H
#define AMP_KERNAL_H

#include "amp.h"
#include <unistd.h>

//包含操作类型和文件名
//这里定义操作类型和type的定义
//0：创建文件
//1：读文件
//2：写文件
//3、截断文件truncate
//后两个的包中带一个page
//现在还不知道要不要加入page的大小和数量，可以先看看
//因为这两个信息可以被在amp的请求那里看到，所以先待定
//此外还有读写的范围
struct __fuse_msg {
	int type;
	char path_name[512];
    //truncate使用的文件截断大小
    off_t length;
    //write和read使用的
    size_t bytes;
    off_t offset;
};
typedef struct __fuse_msg  fuse_msg_t;



#endif