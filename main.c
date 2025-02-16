#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/netfilter.h>		/* for NF_ACCEPT */
#include <errno.h>
#include <libnet.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

int mutex;
char* file_name;

void usage() {
    printf("syntax: netfilter-test <host>\n");
    printf("sample: netfilter-test test.gilgil.net\n");
}

void dump(unsigned char* buf, int size) {
	int i;
	for (i = 0; i < size; i++) {
		if (i != 0 && i % 16 == 0)
			printf("\n");
		printf("%02X ", buf[i]);
	}
	printf("\n");
}

char *check_host(unsigned char* payload)
{
	char* search_ = "Host: ";
	char* tmp_host_ = strstr(payload,search_);
	if(tmp_host_ != NULL)
	{	
		tmp_host_ += 6;
		return tmp_host_;		
		      	
	}
	else	return NULL;
	//if (host_ !=NULL) 
	//{
//		//printf("print host : %s\n",host_);
//		mutex = 1;
//	}
//	else mutex = 0;

}


/* returns packet id */
static u_int32_t print_pkt (struct nfq_data *tb)
{
	
	int id = 0;
	struct nfqnl_msg_packet_hdr *ph;
	struct nfqnl_msg_packet_hw *hwph;
	u_int32_t mark,ifi;
	int ret;
	unsigned char *data;
	struct libnet_ipv4_hdr* ip_;
	struct libnet_tcp_hdr* tcp_;
	unsigned char* http_;
	ph = nfq_get_msg_packet_hdr(tb);
	if (ph) {
		id = ntohl(ph->packet_id);
		printf("hw_protocol=0x%04x hook=%u id=%u ",
			ntohs(ph->hw_protocol), ph->hook, id);
	}

	hwph = nfq_get_packet_hw(tb);
	if (hwph) {
		int i, hlen = ntohs(hwph->hw_addrlen);

		printf("hw_src_addr=");
		for (i = 0; i < hlen-1; i++)
			printf("%02x:", hwph->hw_addr[i]);
		printf("%02x ", hwph->hw_addr[hlen-1]);
	}

	mark = nfq_get_nfmark(tb);
	if (mark)
		printf("mark=%u ", mark);

	ifi = nfq_get_indev(tb);
	if (ifi)
		printf("indev=%u ", ifi);

	ifi = nfq_get_outdev(tb);
	if (ifi)
		printf("outdev=%u ", ifi);
	ifi = nfq_get_physindev(tb);
	if (ifi)
		printf("physindev=%u ", ifi);

	ifi = nfq_get_physoutdev(tb);
	if (ifi)
		printf("physoutdev=%u ", ifi);

	ret = nfq_get_payload(tb, &data);
	if (ret >= 0)
	{
		printf("payload_len=%d ", ret);
		//dump (data,ret);
		
	}
	mutex = 0;
	ip_=(struct libnet_ipv4_hdr*) data;
        if (ip_->ip_p == 6)
	{
		tcp_ = (struct libnet_tcp_hdr*)(data + (ip_->ip_hl*4));
		http_= data + (ip_->ip_hl*4) + (tcp_->th_off*4);
	//	printf("\n%s\n",http_);
		char *check_request = "GET";
		if(memcmp(check_request,http_,3)) 
		{
			FILE *file;
			file = fopen(file_name,"r");
			char *host_=check_host(http_);
			if(host_!=NULL)
			{
			char str[256];
			while(1){
				int W[256] = {0};
				fgets(str,256,file);
				printf("%s\n\n\n\n",host_);
				for (int i = 0, j = 1, status = 0; host_[j] != '\0';++j)
				{
					if (host_[i] == host_[j])
					{
						i++;
						W[j]=++status;
					}
					else if(i>0)
					{
						i=0;
						W[j]=status=0;
					}

				}

				for (unsigned s = 0, f = 0; s < strlen(str);)
				{
					
					while (str[s] == host_[f] && f < strlen(host_))
					{
			
						s++;
						f++;
					}

					if (f > strlen(host_) - 1)  // 완전한 문자열 찾았을 경우
					{
						f = W[f - 1];  // 배열값이 find의 인덱스가 됨
						mutex = 1;
						break;
					}
					else  // 찾기 전에 다른 문자가 나올 경우
					{
						if (W[f] == 0)
							f = 0;  // W값이 0이면 0으로 이동
						else if (W[f] > 0)  // W값이 0보다 크면
							f = W[f] - 1;  // 해당 W값보다 한 칸 앞으로 이동
						s++;  // 뒷글자로 이동
					}
				}
			}

		
			}
			fclose(file);
		}
	}

	fputc('\n', stdout);

	return id;
}


static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
	      struct nfq_data *nfa, void *data)
{
	u_int32_t id = print_pkt(nfa);
	printf("entering callback\n");
	if (mutex == 0)	return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
	else if (mutex == 1 ) return nfq_set_verdict(qh, id, NF_DROP, 0, NULL);

}

int main(int argc, char **argv)
{
	if (argc != 2) {
                usage();
                return -1;
        }

	struct nfq_handle *h;
	struct nfq_q_handle *qh;
	struct nfnl_handle *nh;
	int fd;
	int rv;
	char buf[4096] __attribute__ ((aligned));
	file_name = argv[1];
	printf("opening library handle\n");
	h = nfq_open();
	if (!h) {
		fprintf(stderr, "error during nfq_open()\n");
		exit(1);
	}

	printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
	if (nfq_unbind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_unbind_pf()\n");
		exit(1);
	}

	printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
	if (nfq_bind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_bind_pf()\n");
		exit(1);
	}

	printf("binding this socket to queue '0'\n");
	qh = nfq_create_queue(h,  0, &cb, NULL);
	if (!qh) {
		fprintf(stderr, "error during nfq_create_queue()\n");
		exit(1);
	}

	printf("setting copy_packet mode\n");
	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
		fprintf(stderr, "can't set packet_copy mode\n");
		exit(1);
	}

	fd = nfq_fd(h);

	for (;;) {
		if ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) {
			printf("pkt received\n");
			nfq_handle_packet(h, buf, rv);
			continue;
		}
		/* if your application is too slow to digest the packets that
		 * are sent from kernel-space, the socket buffer that we use
		 * to enqueue packets may fill up returning ENOBUFS. Depending
		 * on your application, this error may be ignored. nfq_nlmsg_verdict_putPlease, see
		 * the doxygen documentation of this library on how to improve
		 * this situation.
		 */
		if (rv < 0 && errno == ENOBUFS) {
			printf("losing packets!\n");
			continue;
		}
		perror("recv failed");
		break;
	}
	printf("unbinding from queue 0\n");
	nfq_destroy_queue(qh);

#ifdef INSANE
	/* normally, applications SHOULD NOT issue this command, since
	 * it detaches other programs/sockets from AF_INET, too ! */
	printf("unbinding from AF_INET\n");
	nfq_unbind_pf(h, AF_INET);
#endif

	printf("closing library handle\n");
	nfq_close(h);

	exit(0);
}

