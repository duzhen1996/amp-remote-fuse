//fuse的服务器，将客户端的文件映射到一个文件夹
//并且根据不同的请求针对这个文件夹做出不同的操作

#include "amp_kernal.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

amp_comp_context_t  *this_ctxt;
amp_lock_t          request_queue_lock;
amp_sem_t           request_queue_sem;

//注意创建目录
static const char* ROOT_PATH = "/home/zhendu/server_root/";

LIST_HEAD(request_queue);

//服务器端吉祥三宝的实现，要在连接的时候传入的函数
//这个函数处理传统信息，
int server_queue_req (amp_request_t *req)
{
	amp_lock(&request_queue_lock);
	list_add_tail(&req->req_list, &request_queue);
	amp_unlock(&request_queue_lock);
	amp_sem_up(&request_queue_sem);
	return 1;
}

//段分配函数
//我觉得直接从fuse_msg_t的byte来判断空间的分配比较好
int server_alloc_pages (void *msg, amp_u32_t *niov, amp_kiov_t **iov)
{
	
    //用来暂存函数运行状态（错误码）
    int err = 0;
    
    //指向段分配空间的头指针，最终要移交给iov
    amp_kiov_t *kiov = NULL;
	
	//要传输的东西分为message，还有一些data，这些并没有明显的界限，
    //data是可以分段处理的，是有利于数据的可靠性和效率的，test_msg_t属于我们要
    //传输的消息部分，这个结构体是在测试中新定义的，即便使用字符串也是没有问题的
    fuse_msg_t *fusemsg = NULL;
    
    //一个用来直接申请页空间的指针
    char *page_buf = NULL;

    //页的数量，返回给niov
    int page_num;

    //用以控制循环的计数变量
    int i, j;

	//如果消息本身就是空的，那就不用分配段空间了，因为消息中包含了段分配的基本信息
    if(!msg){
        //返回错误码
        err = -EINVAL;
        return err;
    }

	//通过我们自己构造的消息内容来判断需要几个页
	fusemsg = (fuse_msg_t *)msg;
	if(fusemsg->page_size_now == 0){
		//这里分别是文件的创建和truncate操作
		page_num = 0;
	}else{
		//这里计算一下所需段的数量，段大小为4096
		page_num = 1;
		// if((fusemsg->length / 4096) != 0){
		// 	page_num = page_num + 1;
		// }

		//申请空间
		kiov = (amp_kiov_t *)malloc(sizeof(amp_kiov_t) * page_num);

		//当申请不到的时候就退出
		if(!kiov){
			err = -ENOMEM;
			return err;
		}

		//为所有的段申请的大空间初始化，这是申请空间所必须附带的操作
		memset(kiov, 0, sizeof(amp_kiov_t) * page_num);
		
		size_t page_size = fusemsg->page_size_now;
		//为每个段数据存储分别分配空间，其实就是为ak_addr分配空间
		//为了简化操作，我们只申请一页
		for(i = 0 ; i < page_num ; i++){
			//申请一个页所需的空间
			page_buf = (char*)malloc(page_size);
			//如果申请不到的处理
			if(!page_buf){
				//退还所有的空间
				for(j=0; j < i; j++){
					free(kiov[j].ak_addr);
				}
				free(kiov);
				err = -ENOMEM;
				return err;
			}
			//申请的段空间初始化
			memset(page_buf, 0 , page_size);
			//为段结构体赋值
			kiov[i].ak_addr = page_buf;
			kiov[i].ak_len = page_size;
			//这个offset是基本上必须为0的，可能这里代表的是一个页中的偏移，真正传的只会是
			//这个页中offset之后的部分，之类的。。
			kiov[i].ak_offset = 0;
			kiov[i].ak_flag = 0;
		}
	}

	//以上就是申请完段空间了
	*iov = kiov;
    *niov = page_num;
	return 0;

}

//段回收
void server_free_pages (amp_u32_t num, amp_kiov_t **kiov)
{
	int i;
    amp_kiov_t * kiovt = *kiov;
	for(i=0; i<num; i++)
		free(kiovt[i].ak_addr);
	return;
}

//向客户端发送结果，有必要的话需要返回一个buf，result = 1代表发送成功，=0代表发送失败
int send_to_client(amp_request_t *req, int result, char* buf){
	//根据结果修改返回请求的值
	fuse_msg_t* fusemsg = NULL;
	fusemsg = (fuse_msg_t *)((char *)req->req_msg + AMP_MESSAGE_HEADER_LEN);

	//先修改消息，返回yes
	memset(fusemsg,0,sizeof(fuse_msg_t));
	if(result == 1){
		sprintf(fusemsg->path_name, "yes");
	}else if(result == 0){
		sprintf(fusemsg->path_name, "no");
	}
	
	//然后修改包头，把信息的位置换一下，等等操作
	req->req_reply = req->req_msg;
	req->req_replylen = req->req_msglen;
	req->req_msg=NULL;
	req->req_msglen=0;
	req->req_need_ack = 0;
	req->req_resent = 0;
	req->req_type = AMP_REPLY|AMP_MSG;
    req->req_niov = 0;
    req->req_iov = NULL;

	//然后看看要不要进行段填充
	if(!buf){
		//有东西要传输
	}

	//然后把东西发回去
	amp_send_sync(this_ctxt, req, req->req_remote_type, req->req_remote_id,0);

	//回收空间
	amp_free(req->req_reply, req->req_replylen);
    __amp_free_request(req);

	return 0;
}

//创建一个函数来处理请求
int slove_request(amp_request_t *req){
	
	int res;

	char* tmp_path_name = NULL;
	
	//根据请求类型处理
	//这里处理新增文件的请求
	//首先先把自定义消息那里拿出来
	fuse_msg_t* fusemsg = NULL;
	
	fusemsg = (fuse_msg_t *)((char *)req->req_msg + AMP_MESSAGE_HEADER_LEN);
	
	//指向路径名字符串的指针
	tmp_path_name = fusemsg->path_name;
	if(fusemsg->path_name[0] == '/'){
		tmp_path_name = tmp_path_name + 1;
	}

	//用一个指针来存储真正的路径
	char dest_path[512];
	strcpy(dest_path, ROOT_PATH);
	strcat(dest_path, tmp_path_name);

	if(fusemsg->type == 0){
		printf("处理一个文件创建请求\n");

		//创建一个权限完全开放的文件
		//注意是0777来表示这个是8进制数字
		res = open(dest_path, O_CREAT | O_EXCL | O_WRONLY, 0777);
		if (res >= 0){
			res = close(res);
			send_to_client(req,0,NULL);
		}else{
			send_to_client(req,-1,NULL);
		}
	}
	return 0;
}

//main函数
int main(){
	int err = 0;
	int addr;
	struct in_addr naddr;
	amp_request_t  *req = NULL;
	fuse_msg_t   *msg_get = NULL;

	err = inet_aton("127.0.0.1", &naddr);
	if (!err) {
		printf("[main] wrong ip address\n");
		exit(1);
	}
	addr = ntohl(naddr.s_addr);

	amp_lock_init(&request_queue_lock);
	amp_sem_init_locked(&request_queue_sem);
	this_ctxt = amp_sys_init(SERVER, SERVER_ID1);
	if (!this_ctxt) {
		printf("服务器初始化失败\n");
		exit(1);
	}

	err = amp_create_connection(this_ctxt, 
                                    CLIENT, 
                                    0, 
				    INADDR_ANY,
				    SERVER_PORT,
				    AMP_CONN_TYPE_TCP,
				    AMP_CONN_DIRECTION_LISTEN,
				    server_queue_req,
				    server_alloc_pages,
				    server_free_pages);
	if (err < 0) {
		printf("连接初始化失败, err:%d\n", err);
		amp_sys_finalize(this_ctxt);
		exit(1);
	}
	
	printf("---------监听，滴~~~~~---------\n");

	while(1) {
		amp_sem_down(&request_queue_sem);
		printf("发现了传来的请求\n");
		amp_lock(&request_queue_lock);

		if (list_empty(&request_queue)) {
			//没有东西
			amp_unlock(&request_queue_lock);
			continue;
		}

		//取出请求
		req = list_entry(request_queue.next, amp_request_t, req_list);
		list_del_init(&req->req_list);
		amp_unlock(&request_queue_lock);
		
		//取出消息
		msg_get = (fuse_msg_t *)((char *)req->req_msg + AMP_MESSAGE_HEADER_LEN);
		printf("收到了消息[main]type:%d, len:%d, msg:%s\n", msg_get->type, msg_get->bytes, msg_get->path_name);
		amp_free(req->req_msg, req->req_msglen);
		__amp_free_request(req);
		//根据消息，分别处理
		//收到消息之后不回复，先看看可以走通吗

	}

	return 0;

}

