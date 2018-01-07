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