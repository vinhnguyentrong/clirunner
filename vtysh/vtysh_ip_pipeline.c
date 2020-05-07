#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h>
#include <zebra.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "command.h"
#include "vtysh.h"
#include "if.h"

#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h> 
#include "lib/conn.h"


#define MAX_SIZE (1024 * 1024)
#define PORT 8086

#define SEND_AND_RETURN(cmd_line) 		\
 	if (send_cmd(conn, cmd_line) < 0) 	\
		return CMD_WARNING; 			\
	return CMD_SUCCESS; 

struct conn *conn;

int str2i(const char *str) 
{
	int val = 0, i;
	for (i = 0; (str[i] >= '0' && str[i] <= '9'); i++) {
        val = val * 10 + (str[i] - '0'); 
    }
    return val;
}

int send_cmd(struct conn *conn, char *cmd) 
{
	char cmd_line[MAX_SIZE];
	int cmd_len, size_recv;

	if (conn == NULL) {
		printf("Connection is not established!\n");
		return CMD_WARNING;
	}
	snprintf(cmd_line, MAX_SIZE, "%s\r\n", cmd);

	cmd_len = strlen(cmd_line);

	if (write(conn->fd_client, cmd_line, cmd_len) != cmd_len) {
		perror("Send failed");
		return -1;
	}

	size_recv = readall(conn->fd_client, conn->msg_r,MAX_SIZE);
	if (size_recv > 0) {
		conn->msg_r[size_recv] = '\0';
	}
	printf("%s\n",conn->msg_r);
	return CMD_SUCCESS;
}

/* VIEW NODE CMD */

DEFUN (reconnect_server,
	   reconnect_server_cmd,
	   "reconnect (A.B.C.D|WORD) <1-65535>",
	   "reconnect to server\n"
	   "IP server address\n"
	   "Hostname\n"
	   "Port number")
{	
	pthread_t conn_handling;
	char server_addr[MAX_SIZE];
	int port = str2i(argv[1]);
	snprintf(server_addr, MAX_SIZE, "%s", argv[0]);
	conn = conn_init(server_addr, port);
	if (conn == NULL) {
		printf("Cannot establish connection!\n");
		return CMD_WARNING;
	}
	
	if (conn != NULL) 
    	pthread_create(&conn_handling, NULL, check_conn, NULL);
    printf("Connection is established!\n");
	return CMD_SUCCESS;
}

DEFUN (disconnect_server,
	   disconnect_server_cmd,
	   "disconnect",
	   "disconnect to server")
{
	if (conn == NULL) {
		printf("Connection hasn't been established\n"
			   "Cannot disconnect\n");
		return CMD_WARNING;
	}
	conn_free(conn);
	conn = NULL;
	if (conn != NULL) {
		printf("Cannot disconnect\n");
		return CMD_WARNING;
	}
	printf("Disconnect successful\n");
	return CMD_SUCCESS;
}

DEFUN (firewall_ls,
	   firewall_ls_cmd,
	   "firewall ls",
	   "Hieu thi cac rule ACL co trong Scrubber")
{
	SEND_AND_RETURN("firewall ls");
}


DEFUN (redis_status,
	   redis_status_cmd,
	   "redis status",
	   "check redis status")
{
	SEND_AND_RETURN("redis status");
}

DEFUN (link_ls, 
	   link_ls_cmd,
	  "link ls",
	  "Hien thi trang thai cac link")
{
	SEND_AND_RETURN("link ls");
}
DEFUN (corestats_show,
	   corestats_show_cmd,
	   "corestats (show|clear)",
	   "Corestats view\n"
	   "Hien thi thong ke so luong goi tin vao ra cho tat ca pipeline\n"
	   "Reset lai thong ke ve 0")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "corestats %s", argv[0]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (latency_show,
	   latency_show_cmd,
	   "latency show", 
	   "Hien thi dang latency trung binh")
{
	SEND_AND_RETURN("latency show");
}

DEFUN (malformed_status,
	   malformed_status_cmd,
	   "malformed status",
	   "Hien thi dang malformed packet ma scrubber co kha nang nhan biet"
	   "va trang thai chan cho dang do (enable hay disable)")
{
	SEND_AND_RETURN("malformed status");
}

DEFUN (synproxy_status,
	   synproxy_status_cmd,
	   "synproxy status",
	   "synproxy view node\n"
	   "Hien thi trang thai chung cua synproxy")
{
	SEND_AND_RETURN("synproxy status");
}

DEFUN (synproxy_scrip_ip_status,
	   synproxy_scrip_ip_status_cmd,
	   "synproxy scrip A.B.C.D status",
	   "synproxy view node\n"
	   "Kiem tra trang thai cua mot srcIP bat ky")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "synproxy scrip %s status", argv[0]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (ratelimit_dstip_show,
	   ratelimit_dstip_show_cmd,
	   "ratelimit dstip A.B.C.D show",
	   "Hien thi cac thong so rate limit cho <dstip>")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "ratelimit dstip %s show", argv[0]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (pcap_status, 
	   pcap_status_cmd,
	   "pcap status",
	   "pcap view node\n"
	   "Hien thi thong tin chung ve module pcap")
{
	SEND_AND_RETURN("pcap status");
}

DEFUN (pcap_dstip_ls,
	   pcap_dstip_ls_cmd,
	   "pcap dstip ls",
	   "pcap view node\n"
	   "Hien thi ca dstip dang duoc capture")
{
	SEND_AND_RETURN("pcap dstip ls");
}



DEFUN (corestats_show_pipeline_id,
	   corestats_show_pipeline_id_cmd,
	   "corestats show <0-1000>",
	   "Hien thi thong ke so luong vao ra voi ID la <ip_pipeline>")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "corestats show %s", argv[0]);
	SEND_AND_RETURN(cmd_line);
}


// ENABLE NODE

DEFUN (corestats_type,
	   corestats_type_cmd,
	   "corestats (enable|disable|send)",
	   "corestats configure\n"
	   "Enable gui corestats\n"
	   "Disable gui corestats\n"
	   "Gui thong ke len portal dong thoi reset thong ke ve 0."
	   " Khong nen chay command nay bang tay vi co the dan den hien thi sai thong tin tren portal")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "corestats %s", argv[0]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (sysinfo_type,
	   sysinfo_type_cmd,
	   "sysinfo (enable|disable)",
	   "System info\n"
	   "Enable gui sysinfo\n"
	   "Disable gui sysinfo")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "sysinfo %s", argv[0]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (latency_conf,
	   latency_conf_cmd,
	   "latency (enable|disable)",
	   "Configure tinh nang do latency\n"
	   "Enable tinh nang do latency\n"
	   "Disable tinh nang do latency")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "latency %s", argv[0]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (malformed_type,
	   malformed_type_cmd,
	   "malformed (enable|disable) WORD",
	   "Malformed configure\n"
	   "Enable tinh nang chan cac goi tin tan cong dang <atack_type>\n"
	   "Disable tinh nang chan cac goi tin co dau hieu tan cong dang <atack_type>\n"
	   "atack_type")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "malformed %s %s", argv[0], argv[1]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (synproxy_conf,
	   synproxy_conf_cmd,
	   "synproxy (enable|disable)",
	   "Synproxy configure\n"
	   "Enable tinh nang synproxy\n"
	   "Disable tinh nang synproxy")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "synproxy %s", argv[0]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (redis_conf,
	   redis_conf_cmd,
	   "redis (enable|disable) (polling|pushing)",
	   "Redis configure\n"
	   "Cho phep viec nhan command tu dashboard\n"
	   "Tam thoi ngat nhan command tu dashboard\n"
	   "Cho phep gui thong tin len dashboard\n"
	   "Tam thoi ngat gui thong tin len dashboard")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "redis %s %s", argv[0], argv[1]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (redis_auto_reconnect,
	   redis_auto_reconnect_cmd,
	   "redis (enable|disable) auto_reconnect",
	   "Redis configure\n"
	   "Enable tinh nang tu dong ket noi lai voi redis server\n"
	   "Disable tinh nang tu dong ket noi lai voi redis server\n"
	   "Redis auto reconnect\n")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "redis %s auto_reconnect", argv[0]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (redis_conn,
	   redis_conn_cmd,
	   "redis (reconnect|disconnect)",
	   "Redis configure\n"
	   "Ket noi lai toi server duoc ket noi gan nhat\n"
	   "Ngat ket noi voi redis")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "redis %s", argv[0]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (redis_connect_host,
	   redis_connect_host_cmd,
	   "redis connect (WORD|A.B.C.D) <1-65535>",
	   "Redis configure\n"
	   "redis establish Connection\n"
	   "Ket noi voi redis server thong qua hostname\n"
	   "Ket noi voi redis server thong qua ip address\n"
	   "port")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "redis connect %s %s", argv[0], argv[1]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (pcap_conf,
	   pcap_conf_cmd,
	   "pcap (enable|disable)",
	   "pcap configure\n"
	   "Enable module pcap\n"
	   "Disable module pcap")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "pcap %s", argv[0]);
	SEND_AND_RETURN(cmd_line);
}

/* CONFIUGRE NOED */

DEFUN (firewall_del_bgpfs_rule,
	   firewall_del_bgpfs_rule_cmd,
	   "firewall del bgpfs rule <0-1000>",
	   "firewall configure\n"
	   "Xoa mot rule ACL trong Scrubber")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "firewall del bgpfs rule %s", argv[0]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (firewall_add_priority_10_bgpfs,
	   firewall_add_priority_10_bgpfs_cmd,
	   "firewall add priority 10 bgpfs .LINE",
	   "firewall configure\n"
	   "Them rule vao whilelist hoac blacklist")
{
	char cmd_line[MAX_SIZE] = "firewall add priority 10 bgpfs";
	for (int i = 0; i < argc; i++) {
		strcat(cmd_line, " ");
		strcat(cmd_line, argv[i]);
	}
	SEND_AND_RETURN(cmd_line);
}

DEFUN (firewall_add_default,
	   firewall_add_default_cmd,
	   "firewall add default <0-1000>",
	   "firewall configure\n"
	   "Them rule mac dinh")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "firewall add default %s", argv[0]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (firewall_del_default,
	   firewall_del_default_cmd,
	   "firewall del default",
	   "firewall configure\n"
	   "Xoa rule mac dinh")
{
	SEND_AND_RETURN("firewall del default");
}

DEFUN (synproxy_scrip_ip_del,
	   synproxy_scrip_ip_del_cmd,
	   "synproxy scrip A.B.C.D del",
	   "Xoa mot srcIP ra khoi mot danh sach cac IP tracking")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "synproxy scrip %s del", argv[0]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (synproxy_syn_ack_ratio_thres,
	   synproxy_syn_ack_ratio_thres_cmd,
	   "synproxy syn_ack_ratio_thres <0-1000>",
	   "Cau hinh nguong chan tren ti le goi ack")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "synproxy syn_ack_ratio_thres %s", argv[0]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (ratelimit_dstip_proto,
	   ratelimit_dstip_proto_cmd,
	   "ratelimit dstip A.B.C.D proto (udp|tcp|icmp) <0-10000000> (mbps|kbps)",
	   "Cau hinh nguong cho dstip va protocol")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "ratelimit dstip %s proto %s %s %s", 
									argv[0], argv[1], argv[2], argv[3]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (redis_channel_name,
	   redis_channel_name_cmd,
	   "redis (cmdqueue|netflowq|sysinfoq|corestatsq|bwstatsq) WORD",
	   "Thay doi hang doi nhan gui thong tin voi redis")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "redis %s %s", argv[0], argv[1]);
	SEND_AND_RETURN(cmd_line);
}

DEFUN (pcap_duration_or_samp,
	   pcap_duration_or_samp_cmd,
	   "pcap dstip add A.B.C.D {duration WORD |sampling_rate WORD}",
	   "pca configure node config\n"
	   "key\n"
	   "key\n"
	   "Destination IP\n"
	   "Viec capture goi ket thuc sau time_out giay\n"
	   "Thoi gian time_out (s)\n"
	   "Cu moi N goi tin lay mau mot goi\n"
	   "So goi tin N")
{
	char cmd_line[MAX_SIZE];
	if (argv[1] == NULL) {
		snprintf(cmd_line, MAX_SIZE, "pcap dstip add %s", argv[0]);
	} else if (argv[1] != NULL && argv[2] == NULL) {
		snprintf(cmd_line, MAX_SIZE, "pcap dstip add %s duration %s", argv[0], argv[1]);
	} else {
		snprintf(cmd_line, MAX_SIZE, "pcap dstip add %s duration %s sampling_rate %s", 
													argv[0], argv[1], argv[2]);
	}
	SEND_AND_RETURN(cmd_line);
}

DEFUN (pcap_packet_per_file,
	   pcap_packet_per_file_cmd,
	   "pcap packet_per_file WORD",
	   "pcap configure node config\n"
	   "Cau hinh So luong goi tin trong moi file pcap. Mac dinh 500\n"
	   "So luong goi tin")
{
	char cmd_line[MAX_SIZE];
	snprintf(cmd_line, MAX_SIZE, "pcap packet_per_file %s", argv[0]);
	SEND_AND_RETURN(cmd_line);
}



void vtysh_ippp_init (void)
{
    install_element (VIEW_NODE, &reconnect_server_cmd);
    install_element (VIEW_NODE, &disconnect_server_cmd);
    install_element (VIEW_NODE, &redis_status_cmd);
    install_element (VIEW_NODE, &link_ls_cmd);
    install_element (VIEW_NODE, &corestats_show_cmd);
    install_element (VIEW_NODE, &latency_show_cmd);
    install_element (VIEW_NODE, &malformed_status_cmd);
    install_element (VIEW_NODE, &synproxy_status_cmd);
    install_element (VIEW_NODE, &synproxy_scrip_ip_status_cmd);
    install_element (VIEW_NODE, &ratelimit_dstip_show_cmd);
    install_element (VIEW_NODE, &pcap_status_cmd);
    install_element (VIEW_NODE, &pcap_dstip_ls_cmd);
    install_element (VIEW_NODE, &corestats_show_pipeline_id_cmd);
    install_element (VIEW_NODE, &firewall_ls_cmd);

    install_element (ENABLE_NODE, &malformed_type_cmd);
    install_element (ENABLE_NODE, &corestats_type_cmd);
    install_element (ENABLE_NODE, &sysinfo_type_cmd);
    install_element (ENABLE_NODE, &latency_conf_cmd);
    install_element (ENABLE_NODE, &synproxy_conf_cmd);
    install_element (ENABLE_NODE, &redis_conf_cmd);
    install_element (ENABLE_NODE, &redis_auto_reconnect_cmd);
    install_element (ENABLE_NODE, &redis_conn_cmd);
    install_element (ENABLE_NODE, &redis_connect_host_cmd);
    install_element (ENABLE_NODE, &pcap_conf_cmd);

    install_element (CONFIG_NODE, &firewall_del_bgpfs_rule_cmd);
    install_element (CONFIG_NODE, &firewall_add_default_cmd);
    install_element (CONFIG_NODE, &firewall_del_default_cmd); 
    install_element (CONFIG_NODE, &firewall_add_priority_10_bgpfs_cmd);
    install_element (CONFIG_NODE, &synproxy_scrip_ip_del_cmd);
    install_element (CONFIG_NODE, &synproxy_syn_ack_ratio_thres_cmd);
    install_element (CONFIG_NODE, &ratelimit_dstip_proto_cmd);
    install_element (CONFIG_NODE, &redis_channel_name_cmd);
    install_element (CONFIG_NODE, &pcap_duration_or_samp_cmd);
    install_element (CONFIG_NODE, &pcap_packet_per_file_cmd);
}
