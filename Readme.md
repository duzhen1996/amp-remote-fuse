# 使用amp协议和FUSE实现一个远程文件系统

参考资料：https://ouonline.net/building-your-own-fs-with-fuse-1

主要实现读写功能，并且维护一个在内存中的元数据表。

## 完成ls操作

完成ls操作需要实现fuse的readdir和getattr两个接口。这样子我们就可以直接对文件系统挂载的目录实现ls操作。

```C
//定义每一个元数据表中的条目
struct __metadata_entry {
    //指向路径名称的指针，到时候可以把一个path名称拷贝到一个空间中
    char* pathName;
    //指向元数据指针
    struct stat *meta;
};
typedef struct __metadata_entry metadata_entry_t;
```

而关于`ls`相关的操作，主要要实现两个函数，一个是getattr，一个是readdir，下面是对这两个函数进行的实现

```C
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
```

<br/>

主要的过程就是遍历元数据表就好了，整体实现不涉及通信，还是比较简单的。下一步就是完成文件的创建，这个过程涉及到通信，所以我们我们先要写一个通信函数，这个函数的内容就是往服务器发一个文件名，告诉服务器要创建一个这个文件名的空文件。并且为元数据表添加一条元数据。


## 完成文件的创建

这个问题最主要的矛盾就是我们先写一个函数，用amp给服务器发一个文件名，让服务器创建这个文件这类问题显然是一条消息可以解决的问题，我们不需要用页来传输。
首先必须要定义消息的格式，我认为，在现有的功能下（文件创建，写文件、读文件），后两个要用到page，但是三个操作的共同点就是消息里面只要包含**文件名和操作类型就可以了**。在文件名的处理上，客户端与服务器端要保持一致。
消息结构体的设计如下。
```C
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
```

<br/>
现在要考虑的就是这个函数应该怎么设计，主要传入的就是上面的这个结构体和一个buf缓冲空间的指针，page的大小可以按照结构体的bytes来设定。
对于读写来说，目标就是修改buf。此外元数据的修改也要注意。