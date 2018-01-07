//远程文件系统的实现
// #define FUSE_USE_VERSION 26

#include <stdio.h>
#include <string.h>
// #include <fuse.h>
#include "metadata.h"
#include "amp_kernal.h"

// static int ou_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
//                       off_t offset, struct fuse_file_info* fi)
// {
//     return filler(buf, "hello-world", NULL, 0);
// }

// static int ou_getattr(const char* path, struct stat* st)
// {
//     memset(st, 0, sizeof(struct stat));

//     if (strcmp(path, "/") == 0)
//         st->st_mode = 0755 | S_IFDIR;
//     else
//         st->st_mode = 0644 | S_IFREG;

//     return 0;
// }

// static struct fuse_operations oufs_ops = {
//     .readdir    =   ou_readdir,
//     .getattr    =   ou_getattr,
// };

int main(int argc, char* argv[])
{
    //因为已经在全局有元数据表的指针了，这里我们要初始化这个表
    printf("开始运行\n");
    init_metadata_table(meta_table_point);

    //return fuse_main(argc, argv, &oufs_ops, NULL);
    printf("运行结束\n");
}