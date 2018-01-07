//远程文件系统的实现
#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <string.h>
#include <fuse.h>
#include "metadata.h"
#include "amp_kernal.h"

//读一个目录中的内容
static int amp_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info* fi)
{
    //循环的计数变量
    int i;
    metadata_entry_t* meta_en = NULL;

    printf("查看是不是根目录\n");
    //因为没有path的设定，所以只要不是跟目录的就报错
    if (strcmp(path, "/") != 0)
		return -ENOENT;
    
    printf("遍历元数据表\n");
    meta_en = *meta_table_point;
    //遍历元数据表
    for(i = 0; i < META_TABLE_SIZE; i++){
        //如果两个指针都是0，那就退出
        if(meta_en[i].path_name == 0 && meta_en[i].meta == 0){
            printf("遍历完毕\n");
            break;
        }else{
            printf("找到一条元数据\n");
            filler(buf, meta_en[i].path_name, meta_en[i].meta,0);
        }
    }

    return filler(buf, "凑数", NULL, 0);

    return 0;
}

//获得一个一个文件的基本信息
static int amp_getattr(const char* path, struct stat* st)
{
    //空间初始化
    int i;
    //判断是不是找到了
    int judge = -1;

    metadata_entry_t* meta_en = NULL;
    meta_en = *meta_table_point;

    memset(st, 0, sizeof(struct stat));

    printf("查看文件类型\n");
    if (strcmp(path, "/") == 0){
        st->st_mode = 0755 | S_IFDIR;
    }else{
        for(i = 0; i < META_TABLE_SIZE; i++){
            //如果两个指针都是0，那就退出
            if(meta_en[i].path_name == 0 && meta_en[i].meta == 0){
                printf("遍历完毕\n");
                break;
            }else{
                //查看目录是不是一样的
                if(strcmp(path, meta_en[i].path_name)==0){
                    printf("找到了\n");
                    judge = 0;
                    *st = *(meta_en[i].meta);
                    break;
                }
            }
        }
        //保证权限与类型
        st->st_mode = 0644 | S_IFREG;
    }
    
    //搜索元数据
    if(judge == -1){
        return -errno;
    }

    return 0;
}

static struct fuse_operations oufs_ops = {
    .getattr    =   amp_getattr,
    .readdir    =   amp_readdir,
};

int main(int argc, char* argv[])
{
    //因为已经在全局有元数据表的指针了，这里我们要初始化这个表
    printf("开始运行\n");
    init_metadata_table(meta_table_point);

    return fuse_main(argc, argv, &oufs_ops, NULL);
    //printf("运行结束\n");
}