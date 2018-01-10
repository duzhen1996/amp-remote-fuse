//远程文件系统的实现
#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <string.h>
#include <fuse.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include "metadata.h"
#include "amp_kernal.h"
#include "amp_client.h"
#include <memory.h>

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
            //看看元数据里面有没有东西
            printf("pathname:%s,meta->mode:%d", meta_en[i].path_name, meta_en[i].meta->st_mode);
            //传入的东西不能有斜杠，太可怕了
            filler(buf, meta_en[i].path_name+1, meta_en[i].meta ,0);
        }
    }

    // return filler(buf, "凑数", NULL, 0);
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
        return 0;
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
    }
    
    //搜索元数据
    if(judge == -1){
        printf("并没有找到相关数据\n");
        return -ENOENT;
    }

    return 0;
}

static int amp_create(const char* path, mode_t mode, struct fuse_file_info* fi)
{
    printf("创建一个新文件");
    // struct ou_entry* o;
    // struct list_node* n;

    //首先先添加一定的元数据
    

    // if (strlen(path + 1) > MAX_NAMELEN)
    //     return -ENAMETOOLONG;

    // list_for_each (n, &entries) {
    //     o = list_entry(n, struct ou_entry, node);
    //     if (strcmp(path + 1, o->name) == 0)
    //         return -EEXIST;
    // }

    // o = malloc(sizeof(struct ou_entry));
    // strcpy(o->name, path + 1); /* skip leading '/' */
    // o->mode = mode | S_IFREG;
    // list_add_prev(&o->node, &entries);
    //打包信息
    fuse_msg_t fusemsg;
    //初始化
    memset(&fusemsg, 0, sizeof(fuse_msg_t));

    fusemsg.type = 0;
    strcpy(fusemsg.path_name, path);

    //发送消息
    send_to_server(&fusemsg, NULL);

    //获取返回的元数据
    if(!fusemsg.server_stat.st_mode){
        printf("蛤？元数据没有传回来？\n");
    }

    //修改元数据
    //查看传回的文件权限
    printf("查看传回的文件权限:%d\n", fusemsg.server_stat.st_mode);
    fusemsg.server_stat.st_mode = mode;
    fusemsg.server_stat.st_uid = getuid();
    fusemsg.server_stat.st_gid = getgid();

    //插入一条元数据
    insert_metadata_table(path, &fusemsg.server_stat);
    
    printf("创建文件处理完毕\n");

    return 0;
}


static int amp_read(const char* path, char* buf, size_t bytes, off_t offset,
                   struct fuse_file_info* fi)
{
    // int fd;
	// int res;

	// (void) fi;
	// fd = open(path, O_RDONLY);
	// if (fd == -1)
	// 	return -errno;

	// res = pread(fd, buf, size, offset);
	// if (res == -1)
	// 	res = -errno;
	// close(fd);
	// return res;

    //首先组装一下消息
    //打包信息
    fuse_msg_t fusemsg;
    struct stat old_stat;
    int res = 0;
    //初始化
    memset(&fusemsg, 0, sizeof(fuse_msg_t));

    fusemsg.type = 1;
    strcpy(fusemsg.path_name, path);
    fusemsg.bytes = bytes;
    fusemsg.offset = offset;
    
    //发送消息，直接把buf传进去，力图修改buf的值
    send_to_server(&fusemsg, buf);

    //将现有的mode取出来,
    res = get_metadata_by_pathname(path, &old_stat);
    
    if(res < 0){
        printf("找不到这个文件的元数据\n");
    }

    //更新元数据
    fusemsg.server_stat.st_mode = old_stat.st_mode;
    fusemsg.server_stat.st_uid = old_stat.st_uid;
    fusemsg.server_stat.st_gid = old_stat.st_gid;

    //将元数据更新到元数据表中
    update_metadata_by_pathname(path, &fusemsg.server_stat);

    return 0;
}

static int amp_unlink(const char* path)
{
    return 0;
}

static int amp_utimens(const char *path, const struct timespec ts[2])
{
	return 0;
}

//看着读一个文件
static int amp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
    printf("准备写一个文件\n");
	// int fd;
	// int res;

	// (void) fi;
	// fd = open(path, O_WRONLY);
	// if (fd == -1)
	// 	return -errno;

	// res = pwrite(fd, buf, size, offset);
	// if (res == -1)
	// 	res = -errno;

	// close(fd);
	// return res;

    //首先组装一下消息
    //打包信息
    fuse_msg_t fusemsg;
    struct stat old_stat;
    int res = 0;
    //初始化
    memset(&fusemsg, 0, sizeof(fuse_msg_t));

    fusemsg.type = 2;
    strcpy(fusemsg.path_name, path);
    fusemsg.bytes = size;
    fusemsg.offset = offset;
    fusemsg.page_size_now = size;

    printf("准备发送");
    //将buf的信息传进去，发送给服务器端
    send_to_server(&fusemsg, buf);

    //然后更新元数据
    //将现有的mode取出来,
    res = get_metadata_by_pathname(path, &old_stat);
    
    if(res < 0){
        printf("找不到这个文件的元数据\n");
    }

    //更新元数据
    fusemsg.server_stat.st_mode = old_stat.st_mode;
    fusemsg.server_stat.st_uid = old_stat.st_uid;
    fusemsg.server_stat.st_gid = old_stat.st_gid;

    //将元数据更新到元数据表中
    update_metadata_by_pathname(path, &fusemsg.server_stat);

    return 0;
}

//truncate文件
static int amp_truncate(const char *path, off_t size)
{
	// int res;
    printf("准备截断一个文件\n");
	// res = truncate(path, size);
	// if (res == -1)
	// 	return -errno;

	// return 0;
    //首先组装一下消息
    //打包信息
    fuse_msg_t fusemsg;
    struct stat old_stat;
    int res = 0;
    //初始化
    memset(&fusemsg, 0, sizeof(fuse_msg_t));

    fusemsg.type = 3;
    strcpy(fusemsg.path_name, path);

    //发送消息
    printf("准备发送截断消息给文件\n");
    send_to_server(&fusemsg, NULL);

    //获取返回的元数据
    if(!fusemsg.server_stat.st_mode){
        printf("蛤？元数据没有传回来？\n");
    }

    //修改元数据
    //将现有的mode取出来,
    //然后更新元数据
    //将现有的mode取出来,
    res = get_metadata_by_pathname(path, &old_stat);
    
    if(res < 0){
        printf("找不到这个文件的元数据\n");
    }

    //更新元数据
    fusemsg.server_stat.st_mode = old_stat.st_mode;
    fusemsg.server_stat.st_uid = old_stat.st_uid;
    fusemsg.server_stat.st_gid = old_stat.st_gid;

    //将元数据更新到元数据表中
    update_metadata_by_pathname(path, &fusemsg.server_stat);

    printf("文件的截断处理完成\n");

    return 0;
}

static struct fuse_operations oufs_ops = {
    .getattr    =   amp_getattr,
    .readdir    =   amp_readdir,
    .create     =   amp_create,
    .unlink     =   amp_unlink,
    .utimens	=   amp_utimens,
    .read		=   amp_read,
    .write      =   amp_write,
    .truncate	=   amp_truncate,
};

int main(int argc, char* argv[])
{
    //因为已经在全局有元数据表的指针了，这里我们要初始化这个表
    printf("开始运行\n");
    init_metadata_table(meta_table_point);

    // fuse_msg_t* fusemsg = (fuse_msg_t *)malloc(sizeof(fuse_msg_t));
    // memset(fusemsg, 0, sizeof(fuse_msg_t));

    // sprintf(fusemsg->path_name, "Hello");
    
    // void* buf = NULL;
    // send_to_server(fusemsg, buf);
    // printf("mode:%d\n", fusemsg->server_stat.st_mode);
    // insert_metadata_table(&(fusemsg->path_name), &(fusemsg->server_stat));
    // 
    // return 0;
    // return fuse_main(argc, argv, &oufs_ops, NULL);
    //printf("运行结束\n");


    //进行文件传输的单元测试
    fuse_msg_t* fusemsg = (fuse_msg_t *)malloc(sizeof(fuse_msg_t));
    memset(fusemsg, 0, sizeof(fuse_msg_t));

    char* information = "I am angry";

    sprintf(fusemsg->path_name, "Hello");
    //设置大小
    fusemsg->page_size_now = strlen(information);
    
    //在段里面加东西
    send_to_server_test(fusemsg, information);
    
}