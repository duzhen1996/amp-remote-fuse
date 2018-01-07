#ifndef METADATA_H
#define METADATA_H

#include <stdlib.h>
#include <memory.h>
#include <sys/stat.h>
#include <unistd.h>
//这个函数处理元数据的结构问题

//元数据表大小
#define META_TABLE_SIZE   10

//定义每一个元数据表中的条目
struct __metadata_entry {
    //指向路径名称的指针，到时候可以把一个path名称拷贝到一个空间中
    char* pathName;
    //指向元数据指针
    struct stat *meta;
};
typedef struct __metadata_entry metadata_entry_t;

//一个指向50位的元数据表指针的指针
metadata_entry_t** meta_table_point = NULL;

//元数据表的初始化函数
void init_metadata_table(){
    meta_table_point = (metadata_entry_t**)malloc(sizeof(metadata_entry_t*));
    //申请一段空间，来存储元数据
    //用一个临时指针来申请空间
    printf("准备初始化元数据表\n");
    metadata_entry_t* tmp_point = 
        (metadata_entry_t*)malloc(sizeof(metadata_entry_t)*META_TABLE_SIZE);

    //初始化所有的空间
    printf("正在初始化分配的空间\n");
    memset(tmp_point, 0, sizeof(metadata_entry_t)*META_TABLE_SIZE);

    //判断一个这个条目是不是有东西，主要就是判断两个指针是不是都是0。当两个指针是0的时候
    //就代表这个条目是空的
    // *input_meta_table = tmp_point;
    // input_meta_table = &tmp_point;
    //上面两种书写都不对，不能全局变量指向局部变量的值：http://blog.csdn.net/qq_28093585/article/details/77752501
    // 也不能直接去取地址0的内容
    *input_meta_table = tmp_point;
    printf("元数据表初始化完毕\n");
}

#endif