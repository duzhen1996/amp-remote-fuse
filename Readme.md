# 使用amp协议和FUSE实现一个远程文件系统

参考资料：https://ouonline.net/building-your-own-fs-with-fuse-1

主要实现读写功能，并且维护一个在内存中的元数据表。

## 完成ls操作

完成ls操作需要实现fuse的readdir和getattr两个接口。这样子我们就可以直接对文件系统挂载的目录实现ls操作。

