#ifndef METADATA_H
#define METADATA_H

#include <stdlib.h>
#include <memory.h>
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

//一个指向50位的元数据表指针的指针
struct __metadata_entry** meta_table_point;

//元数据表的初始化函数
void init_metadata_table(struct __metadata_entry **input_meta_table){
    //申请一段空间，来存储元数据
    //用一个临时指针来申请空间
    struct __metadata_entry* tmp_point = 
        (struct __metadata_entry*)malloc(sizeof(struct __metadata_entry)*META_TABLE_SIZE);

    //初始化所有的空间
    memset(tmp_point, 0, sizeof(struct __metadata_entry)*META_TABLE_SIZE);

    //判断一个这个条目是不是有东西，主要就是判断两个指针是不是都是0。当两个指针是0的时候
    //就代表这个条目是空的
    *input_meta_table = tmp_point;
}

#endif