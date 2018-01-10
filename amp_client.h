#ifndef AMP_CLIENT_H
#define AMP_CLIENT_H

//这里放着一个与服务器通信的函数
#include "amp_kernal.h"

//客户端上下文
static amp_comp_context_t *clt_ctxt = NULL;

//这里需要吉祥二宝，作为客户端首先消息的时候分配空间的依据
//和服务器端保持一致即可
//段分配函数
int client_alloc_pages (void *msg, amp_u32_t *niov, amp_kiov_t **iov)
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
	printf("成功申请了段空间\n");
	return 0;

}

//段回收
void client_free_pages (amp_u32_t num, amp_kiov_t **kiov)
{
	int i;
    amp_kiov_t * kiovt = *kiov;
	for(i=0; i<num; i++)
		free(kiovt[i].ak_addr);
	return;
}

//这里写一个函数，向服务器端发送消息和接收内容
//要将input_buf所指向的空间填满
//这个指针应该是指向一个空间的
int send_to_server(fuse_msg_t* msg, void *input_buf){
    int err = 0;
	
    //通信地址
    int addr;
	struct in_addr naddr;

    //要发送的请求
	amp_request_t *req = NULL;
	fuse_msg_t *fusemsg = NULL;
	amp_message_t *reqmsg = NULL;
	amp_message_t *replymsg = NULL;
	
    //为一个消息申请空间的大小
    int size;

    //循环计数变量
	// int i;

    //设置地址
    err = inet_aton("127.0.0.1", &naddr);
	if (!err) {
		printf("ip地址错误\n");
		exit(1);
	}

    //设置地址的网络格式
	addr = htonl(naddr.s_addr);

	//amp_lock_init(&request_queue_lock);
	//amp_sem_init_locked(request_queue_sem);
    //初始化消息上下文
	clt_ctxt = amp_sys_init(CLIENT, CLIENT_ID1);

	if (!clt_ctxt) {
		printf("客户端初始化失败\n");
		exit(1);
	}
    
    printf("客户端初始化完毕\n");

    //这里创建连接
    err = amp_create_connection(clt_ctxt, 
                                    SERVER, 
                                    SERVER_ID1, 
				    addr,
				    SERVER_PORT,
				    AMP_CONN_TYPE_TCP,
				    AMP_CONN_DIRECTION_CONNECT,
				    NULL,
				    client_alloc_pages,
				    client_free_pages);

    if (err < 0) {
		printf("与服务器连接失败, err:%d\n", err);
		amp_sys_finalize(clt_ctxt);
		exit(1);
	}

    //申请一个请求
    err = __amp_alloc_request(&req);
	if (err < 0) {
		printf("请求申请失败, err:%d\n", err);
		exit(1);
	}

    //为消息申请空间
    size = AMP_MESSAGE_HEADER_LEN + sizeof(fuse_msg_t);
	reqmsg = (amp_message_t *)malloc(size);
	if (!reqmsg) {
		printf("消息空间分配错误, err:%d\n", err);
		exit(1);
	}

    //初始化要发送的消息空间
    memset(reqmsg, 0, size);
	fusemsg = (fuse_msg_t *)((char *)reqmsg + AMP_MESSAGE_HEADER_LEN);
	
    //然后我们把消息拷贝进来
    *fusemsg = *msg;

    //然后开始发送
    req->req_msg = reqmsg;
    req->req_msglen = size;
	req->req_need_ack = 1;
	req->req_resent = 0;
	req->req_type = AMP_REQUEST|AMP_MSG;
    req->req_niov = 0;
    req->req_iov = NULL;
    

    //看看有没有段空间的申请。对于客户端来说，只有写文件需要申请并填充段空间
    if(fusemsg->page_size_now != 0){
		printf("申请段空间\n");
        err = client_alloc_pages(fusemsg, &req->req_niov, &req->req_iov);
		if (err < 0){
            printf("段空间申请失败\n");
        }
        req->req_type = AMP_REQUEST|AMP_DATA;

		//这里是要往服务器端发送的数据
		//我们将input_buf中的数据拷贝到段中
		memcpy( req->req_iov, input_buf, fusemsg->page_size_now);
    }
    
    //正式发送内容
	printf("准备发送内容\n");
    err = amp_send_sync(clt_ctxt, req, SERVER, 1, 0);
    if (err < 0) {
			printf("消息发送失败, err:%d\n", err);
			return err;
	}

    printf("消息发送完毕\n");
    //现在接受
	replymsg = req->req_reply;
	fusemsg = (fuse_msg_t *)((char *)replymsg + AMP_MESSAGE_HEADER_LEN);

	if(strcmp(fusemsg->path_name, "yes") == 0){
		//这里说明操作成功了
		printf("操作成功6666\n");
		//将收到的数据放回去
		*msg = *fusemsg;
		
		//这里查看发回来的东西
		if(fusemsg->page_size_now != 0){
			printf("有文件发回来了！\n");
			//将input_buf所指向的空间填满
			//好了，这样子就接收到发过来的数据了
			memcpy(input_buf, req->req_iov, fusemsg->page_size_now);
		}

		//空间回收，因为都是使用值拷贝，所以这样子直接析构应该问题不大
		//如果段中有类型就清空段
		if (req->req_iov) {
			client_free_pages(req->req_niov, &req->req_iov);
			free(req->req_iov);
			req->req_iov = NULL;
			req->req_niov = 0;
		}

		//回收空间
		amp_free(req->req_reply, req->req_replylen);
		__amp_free_request(req);

		return 0;
	} else if(strcmp(fusemsg->path_name, "no")==0){
		printf("操作失败。。。。。\n");
		//将收到的数据放回去
		*msg = *fusemsg;

		//空间回收，因为都是使用值拷贝，所以这样子直接析构应该问题不大
		//如果段中有类型就清空段
		if (req->req_iov) {
			client_free_pages(req->req_niov, &req->req_iov);
			free(req->req_iov);
			req->req_iov = NULL;
			req->req_niov = 0;
		}

		//回收空间
		amp_free(req->req_reply, req->req_replylen);
		__amp_free_request(req);

		return -1;
	} else{

		printf("传回来的是什么鬼\n");
	}

	//将收到的数据放回去
	*msg = *fusemsg;

	//空间回收，因为都是使用值拷贝，所以这样子直接析构应该问题不大
	//如果段中有类型就清空段
	if (req->req_iov) {
		client_free_pages(req->req_niov, &req->req_iov);
		free(req->req_iov);
		req->req_iov = NULL;
		req->req_niov = 0;
	}

	//回收空间
	amp_free(req->req_reply, req->req_replylen);
	__amp_free_request(req);
	
    return 0;
}





#endif