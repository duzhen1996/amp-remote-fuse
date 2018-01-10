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
```

<br/>

主要的过程就是遍历元数据表就好了，整体实现不涉及通信，还是比较简单的。下一步就是完成文件的创建，这个过程涉及到通信，所以我们我们先要写一个通信函数，这个函数的内容就是往服务器发一个文件名，告诉服务器要创建一个这个文件名的空文件。并且为元数据表添加一条元数据。
在这个函数里面有一个非常重要的坑，那就是`filler(buf, meta_en[i].path_name+1, meta_en[i].meta ,0);`这个的路径名是不能带斜杠`/`的，带了就会出错。


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
    //返回值
    int return_num;
    
    struct stat server_stat;
};
typedef struct __fuse_msg  fuse_msg_t;
```
然后我们设计一下客户端的传输接口，先进行message的传输。真个touch的过程是这样的，首先遍历当前目录下所有的文件，然后查看要touch的文件是不是存在了，如果存在，那就直接就该文件的时间，如果不存在，那就新建一个文件。对于查看一个文件是不是存在的amp_getattr来说，我们要给FUSE一个他看得懂的返回值来搞定，就像下面这样。

```C
    //搜索元数据
    if(judge == -1){
        printf("并没有找到相关数据\n");
        return -ENOENT;
    }
    return 0;
```

<br/>
现在要考虑的就是这个函数应该怎么设计，主要传入的就是上面的这个结构体和一个buf缓冲空间的指针，page的大小可以按照结构体的bytes来设定。
对于读写来说，目标就是修改buf。此外元数据的修改也要注意。

实际的文件创建是在服务器端的，我们告诉服务器要创建的文件名就好了。

## 完成的文件的读写

文件的读写完成起来的难度其实并不大，但是因为涉及到要预申请段空间等等内容，所以在编写过程中bug出现的数量和麻烦程度会高很多。首先就是段申请的时机。实际上，段申请的函数有两种使用的场景。一种是自动的（接收文件时），一种是手动的（发送文件时）。注意统一逻辑，客户端和服务器端申请段空间的时机并不一样。

还有就是返回值的管理，之前一直都把经历放在了处理传入形参上面，但是实际上FUSE接口的返回值规范也是要严格遵守的，需要从服务器端返回。

在FUSE这里我们看到了一些比较有意思的现象，实际上，上层应用程序访问文件的时候是按照4096字节为单位进行读和写的，而文件操作pread与pwrite的返回值是基于真实文件大小的，这是我比较意外的。

## 服务器的设计
我们做一个单线程服务器即可，多线程会有临界变量的问题，需要处理，所以单线程是最为保险的。我们将amp段的大小设成客户端一次所需的大小：
```C
size_t page_size = fusemsg->bytes;
```
段数量固定为一个。服务器的工作分为三块。
首先就是接受信息，通过消息中的type来获取操作的类型，利用本地接口完成对应操作之后操作的结果以及对于对应文件操作之后那个文件新的元数据。在服务器端的文件都会被开放到最大的权限来保证服务器端进程的有效访问。真正的权限信息（mode，gid，uid）放到在客户端上。当元数据传回客户端的时候客户端才会依照实际的情况来搞定权限信息的问题。




