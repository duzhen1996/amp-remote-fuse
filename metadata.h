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
    char* path_name;
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
    *meta_table_point = tmp_point;
    printf("元数据表初始化完毕\n");
}

//用一个函数来插入一条元数据
void insert_metadata_table(const char* path, struct stat *input_meta){
    //循环计数变量
    int i;
    
    //指向第一条元数据的指针
    metadata_entry_t* metatable = *meta_table_point;

    //首先找到空位置
    for(i = 0; i < META_TABLE_SIZE; i++){
        //找到两个指针都是空指针的
        if(metatable[i].path_name == 0 && metatable[i].meta == 0){
            printf("在表中找到的空余位置\n");
            metatable[i].path_name = (char *)malloc(512);
            //重置申请的空间
            memset(metatable[i].path_name,0,512);
            //将路径拷进来
            strcpy(metatable[i].path_name, path);
            
            //将元数据结构体拷贝进来
            metatable[i].meta = (struct stat *)malloc(sizeof(struct stat));
            //初始化空间
            memset(metatable[i].meta, 0, sizeof(struct stat));
            //将元数据拷贝进来
            memcpy(metatable[i].meta, input_meta, sizeof(struct stat));

            //数据插入的时候看看是不是对的
            printf("插入元数据表：%d\n", metatable[i].meta->st_mode);

            printf("完成元数据的插入\n");
            break;
        }
    }
}

//通过文件名获取一个文件的元数据，能找到返回0，找不到返回-1，将元数据通过形参传出去
int get_metadata_by_pathname(const char* path, struct stat *output_meta){
    printf("获取一条元数据\n");
    
    int i;
    metadata_entry_t* meta_en = NULL;
    //遍历元数据表，寻找元数据
    meta_en = *meta_table_point;
    for(i = 0; i < META_TABLE_SIZE; i++){
        //如果两个指针都是0，那就退出
        if(meta_en[i].path_name == 0 && meta_en[i].meta == 0){
            printf("遍历完毕\n");
            return -1;
        }else{
            //查看目录是不是一样的
            if(strcmp(path, meta_en[i].path_name)==0){
                printf("找到了\n");
                *output_meta = *(meta_en[i].meta);
                return 0;
            }
        }
    }

    //不可能到这里
    printf("不可能到这里\n");
    return -1
}

//通过文件名来更新一个文件的元数据
void update_metadata_by_pathname(const char* path, struct stat *update_meta){
    printf("更新文件的元数据\n");

    int i;
    metadata_entry_t* meta_en = NULL;
    //遍历元数据表，寻找元数据
    meta_en = *meta_table_point;
    for(i = 0; i < META_TABLE_SIZE; i++){
        //如果两个指针都是0，那就退出
        if(meta_en[i].path_name == 0 && meta_en[i].meta == 0){
            printf("遍历完毕，没有找到对应元数据\n");
            break;
        }else{
            //查看目录是不是一样的
            if(strcmp(path, meta_en[i].path_name)==0){
                printf("找到了\n");
                *(meta_en[i].meta) = *update_meta;
                printf("元数据更新完毕\n");
                break;
            }
        }
    }
}


#endif