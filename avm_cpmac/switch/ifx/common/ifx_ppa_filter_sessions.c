#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/inet.h>
#include <linux/ktime.h>
#if __has_include(<avm/pa/pa.h>)
#include <avm/pa/pa.h>
#else
#include <linux/avm_pa.h>
#endif
#if __has_include(<avm/sammel/simple_proc.h>)
#include <avm/sammel/simple_proc.h>
#else
#include <linux/simple_proc.h>
#endif
#if __has_include(<avm/pa/ifx_multiplexer.h>)
#include <avm/pa/ifx_multiplexer.h>
#else
#include <linux/avm_pa_ifx_multiplexer.h>
#endif
#include <linux/ip.h>

#define DO_SNAT_MATCH 0
#define NR_FILTERS 16
#define ENTRY_SLEN 128
#define PROC_FILE_NAME "ppa_filter"

extern bool (*filter_session_hook)(struct avm_pa_session *session);

struct endpoint{
	u8 addr_v4[4];
	u8 prefix;
	uint16_t port;
};

struct filter_rule{
	struct endpoint src;
	struct endpoint dst;
	bool bidirectional;
	bool valid;
};

static struct filter_rule filter_whitelist[NR_FILTERS];
static struct filter_rule filter_blacklist[NR_FILTERS];

static size_t nr_active_filters(struct filter_rule* filter_list)
{
	size_t i, res=0;

	for (i = 0; i < NR_FILTERS; i++)
		if(filter_list[i].valid)
			res +=1;
	return res;
}

static char *print_filter_entry(char *buf, size_t len, struct filter_rule *r){
	snprintf(buf, len, "%u.%u.%u.%u/%u:%u %s %u.%u.%u.%u/%u:%u \n",
	       (u32)r->src.addr_v4[0],
	       (u32)r->src.addr_v4[1],
	       (u32)r->src.addr_v4[2],
	       (u32)r->src.addr_v4[3],
	       (u32)r->src.prefix,
	       (u32)r->src.port,
	       r->bidirectional?"<->":"->",
	       (u32)r->dst.addr_v4[0],
	       (u32)r->dst.addr_v4[1],
	       (u32)r->dst.addr_v4[2],
	       (u32)r->dst.addr_v4[3],
	       (u32)r->dst.prefix,
	       (u32)r->dst.port);
	return buf;
}

static void seq_print_filter_entry(struct seq_file *m, struct filter_rule *r){
	char buf[ENTRY_SLEN];
	if ( r->valid ) {
		seq_printf(m, "   %s", print_filter_entry(buf, sizeof(buf), r));
	}
}

/*
 * prefix == 32: no tolerance only exact 32-bit address matches
 * prefix == 24: only first 24-bit need to match
 */
bool ip_v4_match(u8 *addr_v4_a, u8 *addr_v4_b, u32 prefix){
	size_t i;
	u8 xored_v4[4];
	u32 fls_addr = 0;
	bool matched;

	for (i = 0; i < 4; i++){
		xored_v4[i] = addr_v4_a[i] ^ addr_v4_b[i];
	}

	for (i = 0; i < 4; i++){
		u32 byte = xored_v4[i];
		u32 fls_byte = fls( byte );

		/* 
		 * fls_byte_ = 0..8 
		 * fls_addr = 32..0
		 */
		if (fls_byte){
			fls_addr = 32 - (i+1)*8 + fls_byte;
			break;
		}
	}

	if (fls_addr <= (32 - prefix))
		matched = true;
	else
		matched = false;

	return matched;
}

bool session_matches_rule(struct avm_pa_session *session,
			  struct filter_rule *rule)
{
	size_t n;
	bool src_port_match = false;
	bool src_ip_match = false;
	bool dst_port_match = false;
	bool dst_ip_match = false;
	bool res;

	if (!rule->valid)
		return false;

	for (n=0; n < session->ingress.nmatch; n++) {
		u8 tmp_v4[4];
		u16 tmp_port;
		struct avm_pa_match_info *p = &session->ingress.match[n];
		void *hdr = &session->ingress.hdrcopy[p->offset + session->ingress.hdroff];
		switch (p->type) {

		case AVM_PA_IPV4: {
			struct iphdr *iph = hdr;
			// src match
			if (DO_SNAT_MATCH && (session->mod.v4_mod.flags & AVM_PA_V4_MOD_SADDR))
				memcpy(tmp_v4, &session->mod.v4_mod.saddr, 4);
			else
				memcpy(tmp_v4, &iph->saddr, 4);

			if ( ip_v4_match(tmp_v4, rule->src.addr_v4,
					 rule->src.prefix))
				src_ip_match = true;


			// dest match
			if (session->mod.v4_mod.flags & AVM_PA_V4_MOD_DADDR)
				memcpy(tmp_v4, &session->mod.v4_mod.daddr, 4);
			else
				memcpy(tmp_v4, &iph->daddr, 4);

			if ( ip_v4_match(tmp_v4, rule->dst.addr_v4,
					 rule->dst.prefix))
				dst_ip_match = true;

			break;
		}

		case AVM_PA_PORTS: {
			__be16 *ports = hdr;
			// src port match
			if (DO_SNAT_MATCH && (session->mod.v4_mod.flags & AVM_PA_V4_MOD_SPORT))
				tmp_port = session->mod.v4_mod.sport;
			else
				tmp_port = ntohs(ports[0]);

			if ((rule->src.port == 0) || (rule->src.port == tmp_port)){
				src_port_match = true;
				pr_debug("src_port_match: %d/%d\n", rule->src.port, tmp_port );
			}

			// dst port match
			if (session->mod.v4_mod.flags & AVM_PA_V4_MOD_DPORT)
				tmp_port = session->mod.v4_mod.dport;
			else
				tmp_port = ntohs(ports[1]);

			if ((rule->dst.port == 0) || (rule->dst.port == tmp_port)){
				dst_port_match = true;
				pr_debug("dst_port_match: %d/%d\n", rule->dst.port, tmp_port );
			}

			break;
		}
		}
	}


	res = src_port_match && src_ip_match && dst_port_match && dst_ip_match;
	pr_debug("src_port_match=%d, src_ip_match=%d, dst_port_match=%d, dst_ip_match=%d %s\n",
		src_port_match , src_ip_match , dst_port_match , dst_ip_match, res?"--> match":"");

	return res;
}

struct filter_rule *session_matches_list(struct avm_pa_session *session,
			  struct filter_rule *filter_list)
{
	size_t i;
	for (i = 0; i < NR_FILTERS; i++){
		if(session_matches_rule(session, &filter_list[i]))
			return &filter_list[i];
	}
	return NULL;
}

/*
 * returns true, if session shall not be programmed
 */
bool filter_session(struct avm_pa_session *session)
{
	char buf[ENTRY_SLEN];
	bool res = false; /* not filtered */
	struct filter_rule *match;

	if ( nr_active_filters(filter_whitelist) > 0 ){
		match = session_matches_list(session, filter_whitelist);
		if( match ){
			pr_debug("whitelist match: %s\n",
				 print_filter_entry(buf, sizeof(buf), match));
			res = false;
		}
		else {
			res = true;
		}
	}

	if ( nr_active_filters(filter_blacklist) > 0 ){
		match = session_matches_list(session, filter_blacklist);
		if(match){
			pr_debug("blacklist match: %s\n",
				 print_filter_entry(buf, sizeof(buf), match));
			res = true;
		}
		else {
			res = false;
		}
	}
	return res;
}


static void flush_filter_list(struct filter_rule *list){
	size_t i;
	for (i = 0; i < NR_FILTERS; i++)
		memset(&list[i], 0, sizeof(struct filter_rule));
}

static void swap_rule(struct filter_rule *rule){

	struct endpoint tmp;
	tmp = rule->src;
	rule->src = rule->dst;
	rule->dst = tmp;
}

static int insert_rule(struct filter_rule *filter_list,
			struct filter_rule *new_entry)
{
	size_t i;
	for (i = 0; i < NR_FILTERS; i++){
		if ( !filter_list[i].valid ){
			memcpy(&filter_list[i], new_entry,
			       sizeof(struct filter_rule));
			return 0;
		}
	}
	return 1;
}


static int parse_endpoint(struct endpoint *e, char *str)
{
	int total_len;
	char *mask_pos, *port_pos;

	total_len = strlen(str);
	mask_pos = strchr(str, '/') + 1 ;
	port_pos = strchr(str, ':') + 1 ;

	pr_debug("len=%d, str=%s, mask_pos=%s, port_pos=%s\n", total_len, str,
		 mask_pos, port_pos);

	if (!mask_pos || ((mask_pos - str) > total_len))
		return -1;

	if (!port_pos || ((port_pos - str) > total_len))
		return -1;

	if (!in4_pton(str, (mask_pos - str - 1), e->addr_v4, '/', NULL )) {
		pr_err("faild to parse ip '%s', len=%d \n",
		       str, (mask_pos - str - 1));
		return -1;
	}

	e->prefix = simple_strtoul(mask_pos, NULL, 0);

	e->port = simple_strtoul(port_pos, NULL, 0);

	return 0;
}


static int proc_ppa_filter_in(char *line, void *data __maybe_unused)
{
	unsigned int parse_level = 0;
	struct filter_rule *filter_list = NULL;
	struct filter_rule new_entry;

	memset(&new_entry, 0, sizeof(struct filter_rule));

	if ( line ){
		char *tok, *parse_pos;
		parse_pos = line;
		while ((tok = strsep( &parse_pos, " "))){
			if (parse_level == 0){
				if (strncmp("blacklist", tok, strlen("blacklist")) == 0) {
					filter_list = filter_blacklist;
				}
				else if (strncmp("whitelist", tok, strlen("whitelist")) == 0) {
					filter_list = filter_whitelist;
				}
				else {
					pr_err("cannot parse list, usage examples:\n"
					       " blacklist add 214.12.42.1/32:88 -> 192.168.178.44/24:0\n"
					       "    0 == all ports\n"
					       " blacklist clear\n"
					       " whitelist clear\n");
					return 1;
				}
			}

			if (parse_level == 1) {
				if (strncmp("add", tok, strlen("add")) == 0) {
					pr_debug("add filter\n");
				}
				else if (strncmp("clear", tok, strlen("clear")) == 0) {
					flush_filter_list(filter_list);
				}
				else {
					pr_err("cannot parse cmd\n");
					return 1;
				}
			}

			if (parse_level == 2) {
				if (parse_endpoint(&new_entry.src, tok) < 0){
					pr_err("cannot parse src endpoint\n");
					return 1;
				}
			}

			if (parse_level == 3) {
				if (strncmp("->", tok, strlen("->")) == 0) {
					new_entry.bidirectional = false;
				}
				else if (strncmp("<->", tok, strlen("<->")) == 0) {
					new_entry.bidirectional = true;
				}
				else {
					pr_err("cannot parse direction\n");
					return 1;
				}
			}

			if (parse_level == 4) {
				if (parse_endpoint(&new_entry.dst, tok) < 0){
					pr_err("cannot parse dst endpoint\n");
					return 1;
				}
				new_entry.valid = true;
			}

			parse_level += 1;
		}

		if (new_entry.valid){
			size_t entries = 1;

			if( new_entry.bidirectional){
				entries = 2;
				new_entry.bidirectional = false;
			}
			do {
				if(insert_rule(filter_list, &new_entry) ){
					pr_err("no free filter slot\n");
					return 1;
				}

				if(--entries){
					swap_rule(&new_entry);
				}

			} while(entries > 0);
		}
	}

	return 0;
}


static void proc_ppa_filter_out(struct seq_file *m, void *data __maybe_unused){
	size_t i;
	seq_printf(m, "whitelist (lower prio):\n");
	for (i = 0; i < NR_FILTERS; i++){
			seq_print_filter_entry(m, &filter_whitelist[i]);
	}

	seq_printf(m, "\nblacklist (higher prio):\n");
	for (i = 0; i < NR_FILTERS; i++){
			seq_print_filter_entry(m, &filter_blacklist[i]);
	}
}

int __init ifx_ppa_filter_session_init(void) {
	filter_session_hook = filter_session;
	add_simple_proc_file( PROC_FILE_NAME, proc_ppa_filter_in, proc_ppa_filter_out, NULL );
	return 0;
}

void __exit ifx_ppa_filter_session_exit(void) {
	filter_session_hook = NULL;
	remove_simple_proc_file( PROC_FILE_NAME );
}

module_init(ifx_ppa_filter_session_init);
module_exit(ifx_ppa_filter_session_exit);

MODULE_LICENSE("GPL");
